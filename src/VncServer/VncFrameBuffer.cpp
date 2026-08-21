/**
* If not stated otherwise in this file or this component's LICENSE
* file the following copyright and licenses apply:
*
* Copyright 2024 RDK Management
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
**/

#include <netdb.h>
#include <cstring>

#include "VncClient.h"
#include "VncServer.h"
#include "VncFrameBuffer.h"
#include "framebuffer.h"
#include "framebufferrenderer.h"
#include "logger.h"
#include "MemFd.h"

#include <EGL/egl.h>
#include <GLES3/gl3.h>

#ifdef ENABLE_RDKWINDOWMANAGER_VNCSERVER2
#include "VncBridgeServer.h"
#endif

#define ROUND_UP(N, S)      ((((N) + (S) - 1) / (S)) * (S))
std::mutex mVNCFrameBufferContextLock;

namespace RdkWindowManager
{
    VncFrameBuffer::VncFrameBuffer(uint32_t width, uint32_t height)
        : mWidth(width),
          mHeight(height),
          mMatrix(),
          mOpacity(1.0),
          mVncFrameBufferPtr(nullptr),
          mVncFrameBufferSize(0),
          mRGBAData(width * height * 4),
          mPboIds{0, 0},
          mPboWriteIndex(0),
          mPboInitialized(false)
    {
        Logger::log(LogLevel::Information, "In %s Constructor width: %d height: %d", __func__, width, height);
        mFrameBuffer = std::make_shared<FrameBuffer>(mWidth, mHeight);
        float* matrixPointer = mMatrix;
        float matrix[16] =
        {
            1.0, 0.0, 0.0, 0.0,
            0.0, 1.0, 0.0, 0.0,
            0.0, 0.0, 1.0, 0.0,
            0.0, 0.0, 0.0, 1.0
        };
        memcpy(matrixPointer, matrix, sizeof(matrix));

        if (!initVncFrameBuffer())
        {
            Logger::log(LogLevel::Error, "%s: initVncFrameBuffer Failed", __func__);
            return;
        }

        // Install the callback that the VncCaptureThread will invoke on each
        // successfully mapped PBO frame.  All heavy work (pixel copy, format
        // conversion, socket send) runs here on the capture thread, keeping
        // the GL render thread free.
        mCaptureThread.setFrameReadyCallback(
            [this](const uint8_t* pixels, uint32_t pboW, uint32_t pboH, bool bridgeMode)
            {
                this->onFrameReady(pixels, pboW, pboH, bridgeMode);
            });
    }

    VncFrameBuffer::~VncFrameBuffer()
    {
        Logger::log(LogLevel::Information, "%s VncFrameBuffer Destructor", __func__);

        // Stop the capture thread first so it releases any PBO it may be
        // mapping before we delete the GL buffer objects.
        mCaptureThread.stop();

        destroyPBOs();

        // Explicitly release the capture FBO while the GL context is still active
        mCaptureFbo.reset();

        if (mVncFrameBufferPtr != nullptr)
        {
            munmap(mVncFrameBufferPtr, mVncFrameBufferSize);
            mVncFrameBufferPtr  = nullptr;
            mVncFrameBufferSize = 0;
        }
    }

    bool VncFrameBuffer::initVncFrameBuffer()
    {
        size_t size = (mWidth * mHeight * 4) + 8192; // Round up to page multiple
        size = ROUND_UP(size, 4096);

        char memName[32];
        sprintf(memName, "/vncbuffer-%08x", rand());

        Logger::log(LogLevel::Information, "In %s", __func__);

        int memFd = RdkWindowManager::memfd_create(memName, MFD_CLOEXEC);
        if (memFd < 0)
        {
            Logger::log(LogLevel::Information, "%s: failed to create memfd for buffer (%d - %s)", __func__, errno, g_strerror(errno));
            return false;
        }
        if (ftruncate(memFd, size) != 0)
        {
            Logger::log(LogLevel::Information, "%s: failed to resize memfd for buffer (%d - %s)", __func__, errno, g_strerror(errno));
            close(memFd);
            return false;
        }

        void *memPtr = mmap(nullptr, size, PROT_WRITE | PROT_READ, MAP_SHARED, memFd, 0);
        if (memPtr == MAP_FAILED)
        {
            Logger::log(LogLevel::Information, "%s: failed to overlap memfd buffer (%d - %s)", __func__, errno, g_strerror(errno));
            close(memFd);
            return false;
        }

        memset(memPtr, 0x00, size);
        if (close(memFd) != 0)
        {
            Logger::log(LogLevel::Information, "%s: failed to close memfd (%d - %s)", __func__, errno, g_strerror(errno));
        }
        mVncFrameBufferPtr = static_cast<uint8_t*>(memPtr);
        mVncFrameBufferSize = size;

        return true;
    }

    void VncFrameBuffer::begin()
    {
        mFrameBuffer->bind();
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);
    }

    void VncFrameBuffer::end()
    {
        mFrameBuffer->unbind();
    }

    void VncFrameBuffer::draw()
    {
        FrameBufferRenderer::instance()->draw(mFrameBuffer, mWidth, mHeight, mMatrix,
                                              0, 0, mWidth, mHeight,
                                              0, 0, mWidth, mHeight, mOpacity);
    }

    // -------------------------------------------------------------------------
    // publish() – called from the GL render thread on every compositor frame.
    //
    // The function is intentionally non-blocking: it only initiates a GPU→PBO
    // DMA transfer (glReadPixels with a nullptr destination) and posts the PBO
    // handle to VncCaptureThread.  All CPU-heavy work (waiting for the fence
    // sync, mapping the PBO, pixel conversion, socket send) happens on the
    // dedicated capture thread so the render pipeline is not stalled.
    // -------------------------------------------------------------------------
    void VncFrameBuffer::publish()
    {
#ifdef ENABLE_RDKWINDOWMANAGER_VNCSERVER2
        if (VncBridgeServer::getInstance().isBridgeFrameUpdatePending())
        {
            startAsyncCapture(true /* bridgeMode */);
            return;
        }

        // In VNCServer2 bridge mode, internal socket-based frame publishing
        // is not used.  Clear the stale update flag and return.
        if (VncBridgeServer::getInstance().isRunning())
        {
            VncServer::getInstance().setVncFrameUpdateRequestFlag(false);
            return;
        }
#endif

        if (VncServer::getInstance().getVncFrameUpdateRequestFlag())
        {
            if (VncServer::getInstance().getVncFrameBufferProgressState())
            {
                auto socket = VncServer::getInstance().getVncSocket();
                int socketState = (nullptr != socket) ? static_cast<int>(socket->state()) : -1;
                Logger::log(LogLevel::Information, "%s is in progress SKIP VncSocket state %d",
                            __func__, socketState);
                return;
            }

            if (mCaptureThread.isBusy())
            {
                Logger::log(LogLevel::Information, "%s: capture thread busy, skipping frame", __func__);
                return;
            }

            startAsyncCapture(false /* bridgeMode */);
        }
    }

    // -------------------------------------------------------------------------
    // startAsyncCapture() – GL thread, non-blocking.
    //
    // 1. Lazy-starts VncCaptureThread (creates a shared EGL context once).
    // 2. Ensures PBOs are allocated at the correct resolution.
    // 3. Binds a PBO and issues glReadPixels(nullptr) → GPU DMA begins.
    // 4. Creates a glFenceSync and hands both the PBO id and the sync to the
    //    capture thread (postFrame is a non-blocking call).
    // 5. Advances the ping-pong PBO index and returns immediately.
    // -------------------------------------------------------------------------
    void VncFrameBuffer::startAsyncCapture(bool bridgeMode)
    {
        // Lazy-start the capture thread using the GL thread's current EGL context
        if (!mCaptureThread.isRunning())
        {
            EGLDisplay display = eglGetCurrentDisplay();
            EGLContext context  = eglGetCurrentContext();

            if ((display == EGL_NO_DISPLAY) || (context == EGL_NO_CONTEXT))
            {
                Logger::log(LogLevel::Error, "%s: no active EGL context – cannot start capture thread",
                            __func__);
                return;
            }

            if (!mCaptureThread.start(display, context))
            {
                Logger::log(LogLevel::Error, "%s: VncCaptureThread::start() failed", __func__);
                return;
            }
        }

        // Snapshot the currently bound FBO – this is the source for the GPU blit.
        //   Bridge mode   : compositor has left its display FBO bound (typically 0).
        //   Non-bridge    : begin() has bound mFrameBuffer (the VNC render target).
        GLint srcFboId = 0;
        glGetIntegerv(GL_FRAMEBUFFER_BINDING, &srcFboId);

        // Determine source dimensions.
        //   Bridge : full display viewport (e.g. 1920×1080).
        //   Non-bridge : the VNC render target is already at mWidth×mHeight.
        uint32_t srcWidth  = mWidth;
        uint32_t srcHeight = mHeight;

#ifdef ENABLE_RDKWINDOWMANAGER_VNCSERVER2
        if (bridgeMode)
        {
            GLint viewport[4] = {0, 0, 0, 0};
            glGetIntegerv(GL_VIEWPORT, viewport);
            srcWidth  = static_cast<uint32_t>(viewport[2]);
            srcHeight = static_cast<uint32_t>(viewport[3]);

            if ((0 == srcWidth) || (0 == srcHeight))
            {
                Logger::log(LogLevel::Error, "%s: invalid viewport size for bridge capture %u x %u",
                            __func__, srcWidth, srcHeight);
                return;
            }
        }
#endif

        // Lazy-create the intermediate capture FBO at VNC output dimensions.
        // This mirrors AI1.0 (WesterosWindowManager) mCaptureFrameBuffer.
        // glBlitFramebuffer will GPU-scale the source into this FBO so that
        // glReadPixels only DMA's mWidth×mHeight pixels instead of the full
        // source resolution (e.g. 1920×1080 → 960×540 = 4× less data).
        if (!mCaptureFbo)
        {
            mCaptureFbo = std::make_shared<FrameBuffer>(static_cast<int>(mWidth),
                                                        static_cast<int>(mHeight));
            Logger::log(LogLevel::Information, "%s: created capture FBO %u x %u",
                        __func__, mWidth, mHeight);
        }

        // ── GPU blit ───────────────────────────────────────────────────────────
        // Bind the capture FBO as the draw target and the source FBO as the
        // read source, then let the GPU scale the frame (hardware-accelerated,
        // ~1 ms).  This is the same pattern as AI1.0's captureScreen():
        //   glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
        //   mCaptureFbo->bind(GL_DRAW_FRAMEBUFFER);
        //   glBlitFramebuffer(full-res, vnc-res, GL_COLOR_BUFFER_BIT, GL_LINEAR);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, mCaptureFbo->fboId());
        glBindFramebuffer(GL_READ_FRAMEBUFFER, static_cast<GLuint>(srcFboId));
        glBlitFramebuffer(0, 0, static_cast<GLint>(srcWidth),  static_cast<GLint>(srcHeight),
                          0, 0, static_cast<GLint>(mWidth),    static_cast<GLint>(mHeight),
                          GL_COLOR_BUFFER_BIT, GL_LINEAR);

        // Switch read source to the (now populated) capture FBO
        glBindFramebuffer(GL_READ_FRAMEBUFFER, mCaptureFbo->fboId());

        // Init PBOs at VNC output size on first use.  The GPU blit above has
        // already resolved any source/destination dimension mismatch, so the
        // PBO is always mWidth × mHeight regardless of the source resolution.
        if (!mPboInitialized)
        {
            if (!initPBOs())
            {
                Logger::log(LogLevel::Error, "%s: PBO initialisation failed", __func__);
                glBindFramebuffer(GL_FRAMEBUFFER, 0);
                return;
            }
        }

        GLenum fbStatus = glCheckFramebufferStatus(GL_READ_FRAMEBUFFER);
        if (fbStatus != GL_FRAMEBUFFER_COMPLETE)
        {
            Logger::log(LogLevel::Error, "%s: capture FBO not complete (0x%X)", __func__, fbStatus);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            return;
        }

        const int writeIdx = mPboWriteIndex;

        // Non-blocking async GPU→PBO DMA at VNC output size.
        // nullptr destination routes the transfer into the bound PBO.
        glBindBuffer(GL_PIXEL_PACK_BUFFER, mPboIds[writeIdx]);
        glReadPixels(0, 0, static_cast<GLsizei>(mWidth), static_cast<GLsizei>(mHeight),
                     GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

        // Fence sync: capture thread waits on this before mapping the PBO
        GLsync sync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);

        glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

        // Restore default framebuffer so subsequent GL operations are unaffected
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        if (!sync)
        {
            Logger::log(LogLevel::Error, "%s: glFenceSync failed", __func__);
            return;
        }

        GLenum err = glGetError();
        if (err != GL_NO_ERROR)
        {
            Logger::log(LogLevel::Error, "%s: GL error after glReadPixels (0x%X)", __func__, err);
            glDeleteSync(sync);
            return;
        }

        // Hand PBO + sync to the capture thread (non-blocking).
        // The capture thread calls glClientWaitSync, maps the PBO, and delivers
        // the frame – all without stalling the GL render loop.
        mCaptureThread.postFrame(mPboIds[writeIdx], sync, mWidth, mHeight, bridgeMode);

        // Advance the write index for the next call (ping-pong)
        mPboWriteIndex = (mPboWriteIndex + 1) % kPboCount;
    }

    // -------------------------------------------------------------------------
    // onFrameReady() – called on VncCaptureThread after the PBO has been
    // mapped.  Copies / scales the raw RGBA pixel data into mRGBAData, then
    // dispatches to the correct send path.
    // -------------------------------------------------------------------------
    void VncFrameBuffer::onFrameReady(const uint8_t* pixels,
                                       uint32_t pboWidth,
                                       uint32_t pboHeight,
                                       bool bridgeMode)
    {
        if (!pixels)
        {
            // Capture failure – signal bridge if applicable, otherwise skip
#ifdef ENABLE_RDKWINDOWMANAGER_VNCSERVER2
            if (bridgeMode)
            {
                Logger::log(LogLevel::Error, "%s: pixel capture failed, signalling bridge", __func__);
                VncBridgeServer::getInstance().deliverFrame(nullptr, 0, 0);
            }
#endif
            return;
        }

        // The GPU blit in startAsyncCapture() has already scaled the source frame
        // to mWidth × mHeight, so pboWidth == mWidth and pboHeight == mHeight always.
        // A simple memcpy is sufficient – no CPU scaling required.
        std::memcpy(mRGBAData.data(), pixels, mWidth * mHeight * 4);

        if (bridgeMode)
        {
#ifdef ENABLE_RDKWINDOWMANAGER_VNCSERVER2
            VncBridgeServer::getInstance().deliverFrame(mRGBAData.data(), mWidth, mHeight);
#endif
        }
        else
        {
            sendFrameBufferToVNCClient();
        }
    }

    bool VncFrameBuffer::sendFrameBufferToVNCClient()
    {
        bool status = false;
        constexpr size_t headerSize = sizeof(VncFrameBufferUpdate) + sizeof(VncFrameBufferRectangle);
        const size_t frameOffset = (headerSize + 63) & ~63; // align to a 64-byte align address
        const size_t headerOffset = frameOffset - headerSize;
        uint32_t frameWritten = 0;
        static bool frameSkipLogged = false;

        if (VncServer::getInstance().getVncFrameBufferProgressState())
        {
            return status;
        }

        std::lock_guard<std::mutex> contextLock(mVNCFrameBufferContextLock);
        VncServer::getInstance().setVncFrameBufferProgressState(true);

        VncFrameBufferUpdate *update = reinterpret_cast<VncFrameBufferUpdate*>(mVncFrameBufferPtr + headerOffset);
        update->messageType = 0x00;
        update->numberOfRectangles = htons(1);
        update->rectangles[0].xPosition = htons(0);
        update->rectangles[0].yPosition = htons(0);
        update->rectangles[0].width = htons(mWidth);
        update->rectangles[0].height = htons(mHeight);
        update->rectangles[0].encodingType = htonl(static_cast<int32_t>(VncEncoding::Raw));

        frameWritten = readAndConvertPixelData(frameOffset);
        if(frameWritten == 0)
        {
            if (false == frameSkipLogged)
            {
                Logger::log(LogLevel::Warn, "%s frameWritten is zero, Nothing to send SKIP", __func__);
                frameSkipLogged = true;
            }
            VncServer::getInstance().setVncFrameBufferProgressState(false);
            return status;
        }
        else
        {
            // Reset one-time warning when frame production recovers.
            frameSkipLogged = false;
        }

        auto socket = VncServer::getInstance().getVncSocket();
        if ((nullptr == socket) || (IVncSocket::State::Open != socket->state()))
        {
            Logger::log(LogLevel::Warn, "%s: VNC socket unavailable while sending frame", __func__);
            VncServer::getInstance().setVncFrameBufferProgressState(false);
            return status;
        }

        GBytes *data = g_bytes_new_with_free_func(  mVncFrameBufferPtr + headerOffset,
                                                    headerSize + frameWritten,
                                                    &VncFrameBuffer::onVncFrameSent, this);
        socket->write(data);

        status = true;
        return status;
    }

    void VncFrameBuffer::onVncFrameSent(gpointer userData)
    {
        auto self = reinterpret_cast<VncFrameBuffer*>(userData);
        // Free is skipped here as the same buffer will be reused again
        VncServer::getInstance().setVncFrameBufferProgressState(false);
        VncServer::getInstance().setVncFrameUpdateRequestFlag(false); // Dont send again until asked by client
    }

    uint32_t VncFrameBuffer::readAndConvertPixelData(const size_t frameOffset)
    {
        uint32_t noOfPixelBytes = 0;
        uint16_t redMax = 0;
        uint16_t greenMax = 0;
        uint16_t blueMax = 0;
        uint8_t redShift = 0;
        uint8_t greenShift = 0;
        uint8_t blueShift = 0;
        uint8_t bitsPerPixel = 0;
        uint8_t* updateBuffer = nullptr;

        switch(VncServer::getInstance().getVncFrameUpdatePixelFormat())
        {
            case VncClient::ClientCaptureFormat::RGB_2_2_2:
                redMax = greenMax = blueMax = 3;
                redShift = 4;
                greenShift = 2;
                blueShift = 0;
                bitsPerPixel = 8;
                break;
            case VncClient::ClientCaptureFormat::RGB0_8_8_8_8:
                redMax = greenMax = blueMax = 255;
                redShift = 0;
                greenShift = 8;
                blueShift = 16;
                bitsPerPixel = 32;
                break;
            case VncClient::ClientCaptureFormat::BGR0_8_8_8_8:
                redMax = greenMax = blueMax = 255;
                redShift = 16;
                greenShift = 8;
                blueShift = 0;
                bitsPerPixel = 32;
                break;
            default:
                Logger::log(LogLevel::Error, "%s: Invalid PixelFormat :%d", __func__, VncServer::getInstance().getVncFrameUpdatePixelFormat());
                break;
        }

        if(bitsPerPixel == 0)
        {
            return noOfPixelBytes;
        }
        noOfPixelBytes = mWidth * mHeight * (bitsPerPixel/8);

        updateBuffer = mVncFrameBufferPtr + frameOffset;

        for (int i = 0; i < (mWidth * mHeight); ++i)
        {
            // Extract the color components, Ignore the Alpha component
            uint8_t r = mRGBAData[i * 4 + 0];
            uint8_t g = mRGBAData[i * 4 + 1];
            uint8_t b = mRGBAData[i * 4 + 2];

            // Scale color components to max values
            uint32_t red   = (r * redMax) / 255;
            uint32_t green = (g * greenMax) / 255;
            uint32_t blue  = (b * blueMax) / 255;

            uint32_t pixel = (red << redShift) | (green << greenShift) | (blue << blueShift); // Pack the pixel according to the client

            if (bitsPerPixel == 32)
            {
                std::memcpy(updateBuffer + i * 4, &pixel, 4);
            }
	    // Note: Currently unreachable (only 8 and 32 bit formats supported)
	    /*
            else if (bitsPerPixel == 16)
                uint16_t outPixel = static_cast<uint16_t>(pixel);
                std::memcpy(updateBuffer + i * 2, &outPixel, 2);
            }*/
            else if (bitsPerPixel == 8)
            {
                updateBuffer[i] = static_cast<uint8_t>(pixel);
            }
        }

        int rowStride = mWidth * (bitsPerPixel/8); // Length of the complete row
        std::vector<unsigned char> tempRow(rowStride);
        // Exchange the top row image data with the bottom row
        for (int y = 0; y < mHeight / 2; ++y)
        {
            unsigned char* rowTop = &updateBuffer[y * rowStride];
            unsigned char* rowBottom = &updateBuffer[(mHeight - 1 - y) * rowStride];

            std::memcpy(tempRow.data(), rowTop, rowStride);
            std::memcpy(rowTop, rowBottom, rowStride);
            std::memcpy(rowBottom, tempRow.data(), rowStride);
        }

        Logger::log(LogLevel::Debug, "%s: Image Flipping Done!", __func__);
        return noOfPixelBytes;
    }

    // initPBOs() – allocates both ping-pong PBOs at the VNC output size (mWidth × mHeight).
    // The GPU blit in startAsyncCapture() always downscales the source to this fixed size
    // before the readback, so PBOs never need to be reallocated between frames.
    bool VncFrameBuffer::initPBOs()
    {
        destroyPBOs();

        const GLsizeiptr bufSize = static_cast<GLsizeiptr>(mWidth) * mHeight * 4;

        glGenBuffers(kPboCount, mPboIds);
        for (int i = 0; i < kPboCount; ++i)
        {
            glBindBuffer(GL_PIXEL_PACK_BUFFER, mPboIds[i]);
            // GL_DYNAMIC_READ: driver places the buffer in memory optimal for
            // GPU writes and CPU reads (DMA-friendly).
            glBufferData(GL_PIXEL_PACK_BUFFER, bufSize, nullptr, GL_DYNAMIC_READ);
        }
        glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

        GLenum error = glGetError();
        if (error != GL_NO_ERROR)
        {
            Logger::log(LogLevel::Error, "%s: failed to create PBOs, glGetError()=0x%X", __func__, error);
            glDeleteBuffers(kPboCount, mPboIds);
            mPboIds[0] = mPboIds[1] = 0;
            return false;
        }

        mPboWriteIndex  = 0;
        mPboInitialized = true;
        Logger::log(LogLevel::Information, "%s: PBOs initialised (%u x %u, %zu bytes each)",
                    __func__, mWidth, mHeight, static_cast<size_t>(bufSize));
        return true;
    }

    void VncFrameBuffer::destroyPBOs()
    {
        if (!mPboInitialized)
            return;

        glDeleteBuffers(kPboCount, mPboIds);
        mPboIds[0] = mPboIds[1] = 0;
        mPboInitialized = false;
    }

} // namespace RdkWindowManager

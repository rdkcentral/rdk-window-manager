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

#include "VncClient.h"
#include "VncServer.h"
#include "VncFrameBuffer.h"
#include "framebuffer.h"
#include "framebufferrenderer.h"
#include "logger.h"
#include "MemFd.h"

#define ROUND_UP(N, S)      ((((N) + (S) - 1) / (S)) * (S))
std::mutex mVNCFrameBufferContextLock;

namespace RdkWindowManager
{
    VncFrameBuffer::VncFrameBuffer(uint32_t width, uint32_t height)
        : mPixelsProcessingInProgress(false),
          mWidth(width),
          mHeight(height),
          mMatrix(),
          mOpacity(1.0),
          mVncFrameBufferPtr(nullptr),
          mVncFrameBufferSize(0),
          mRGBAData(width * height * 4)
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
    }

    VncFrameBuffer::~VncFrameBuffer()
    {
        Logger::log(LogLevel::Information, "%s VncFrameBuffer Destructor", __func__);

        mPixelsProcessingInProgress = false;
        if (mVncFrameBufferPtr != nullptr)
        {
            munmap(mVncFrameBufferPtr, mVncFrameBufferSize);
        }

        mVncFrameBufferPtr = nullptr;
        mVncFrameBufferSize = 0;
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

    void VncFrameBuffer::publish()
    {
        if (VncServer::getInstance().getVncFrameUpdateRequestFlag())
        {
            if (VncServer::getInstance().getVncFrameBufferProgressState())
            {
                Logger::log(LogLevel::Information, "%s is in progress SKIP VncSocket state %d", __func__, VncServer::getInstance().getVncSocket()->state());
                return;
            }
            if (!readPixel())
            {
                Logger::log(LogLevel::Information, "%s - readPixel failed", __func__);
                return;
            }

            mPixelsProcessingInProgress = true;
            std::thread pixelProcessThread([this] {
                this->sendFrameBufferToVNCClient();
                this->notifyPixelProcessDone();
            });
            pixelProcessThread.detach();
        }
    }

    void VncFrameBuffer::notifyPixelProcessDone()
    {
        Logger::log(LogLevel::Information, "%s - All processing done!", __func__);
        mPixelsProcessingInProgress = false;
    }

    bool VncFrameBuffer::sendFrameBufferToVNCClient()
    {
        bool status = false;
        constexpr size_t headerSize = sizeof(VncFrameBufferUpdate) + sizeof(VncFrameBufferRectangle);
        const size_t frameOffset = (headerSize + 63) & ~63; // align to a 64-byte align address
        const size_t headerOffset = frameOffset - headerSize;
        uint32_t frameWritten = 0;

        if (VncServer::getInstance().getVncFrameBufferProgressState())
        {
            Logger::log(LogLevel::Information, "%s is in progress SKIP VncSocket state %d", __func__, VncServer::getInstance().getVncSocket()->state());
            return status;
        }

        Logger::log(LogLevel::Information, "%s calculated offsets : headerSize %zu : frameOffset %zu : headerOffset %zu", __func__, headerSize, frameOffset, headerOffset);

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
            Logger::log(LogLevel::Error, "%s frameWritten is zero, Nothing to send SKIP", __func__);
            VncServer::getInstance().setVncFrameBufferProgressState(false);
            return status;
        }

        GBytes *data = g_bytes_new_with_free_func(  mVncFrameBufferPtr + headerOffset,
                                                    headerSize + frameWritten,
                                                    &VncFrameBuffer::onVncFrameSent, this);
        VncServer::getInstance().getVncSocket()->write(data);

        Logger::log(LogLevel::Information, "%s frameWritten %d", __func__, frameWritten);

        status = true;
        return status;
    }

    void VncFrameBuffer::onVncFrameSent(gpointer userData)
    {
        Logger::log(LogLevel::Information, "In %s", __func__);
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

        Logger::log(LogLevel::Information, "%s: PixelFormat :%d", __func__, VncServer::getInstance().getVncFrameUpdatePixelFormat());

        switch(VncServer::getInstance().getVncFrameUpdatePixelFormat())
        {
            case VncClient::ClientCaptureFormat::RGB_2_2_2:
                redMax = greenMax = blueMax = 3;
                redShift = 4;
                greenShift = 2;
                blueShift = 0;
                bitsPerPixel = 8;
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
            // coverity[DEADCODE : FALSE]
            // Note: Currently unreachable (only 8 and 32 bit formats supported)
            // Kept for future 16-bit format support (e.g., RGB_5_6_5)
            else if (bitsPerPixel == 16)
            {
                uint16_t outPixel = static_cast<uint16_t>(pixel);
                std::memcpy(updateBuffer + i * 2, &outPixel, 2);
            }
            else if (bitsPerPixel == 8)
            {
                updateBuffer[i] = static_cast<uint8_t>(pixel);
            }
        }

        Logger::log(LogLevel::Information, "%s: W:%d H:%d BPP:%d FW:%d", __func__, mWidth, mHeight, (bitsPerPixel/8), noOfPixelBytes);

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

        Logger::log(LogLevel::Information, "%s: Image Flipping Done!", __func__);
        return noOfPixelBytes;
    }

    bool VncFrameBuffer::readPixel()
    {
        bool status = true;
        GLenum valid = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (valid != GL_FRAMEBUFFER_COMPLETE)
        {
            Logger::log(LogLevel::Error, "%s: glCheckFramebufferStatus() = %X", __func__, valid);
        }

        Logger::log(LogLevel::Information, "%s: Read pixels start", __func__);
        std::lock_guard<std::mutex> contextLock(mVNCFrameBufferContextLock);

        glReadPixels(0, 0, mWidth, mHeight, GL_RGBA, GL_UNSIGNED_BYTE, mRGBAData.data());
        GLenum error = glGetError();
        if (error != GL_NO_ERROR)
        {
            Logger::log(LogLevel::Error, "%s: glGetError() = %X\n", __func__, error);
            status = false;
        }
        Logger::log(LogLevel::Information, "%s: Read pixels done", __func__);

        return status;
    }

}



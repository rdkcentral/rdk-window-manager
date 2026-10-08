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

#include "VncCaptureThread.h"
#include "logger.h"

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>

namespace RdkWindowManager
{

VncCaptureThread::VncCaptureThread() = default;

VncCaptureThread::~VncCaptureThread()
{
    stop();
}

void VncCaptureThread::setFrameReadyCallback(FrameReadyCallback cb)
{
    mFrameReadyCallback = std::move(cb);
}

bool VncCaptureThread::start(EGLDisplay display, EGLContext parentContext)
{
    if (mRunning.load())
    {
        Logger::log(LogLevel::Warn, "%s: VncCaptureThread already running", __func__);
        return true;
    }

    mRunning = true;
    mThread  = std::thread(&VncCaptureThread::captureLoop, this, display, parentContext);

    if (!mThread.joinable())
    {
        Logger::log(LogLevel::Error, "%s: failed to start capture thread", __func__);
        mRunning = false;
        return false;
    }

    return true;
}

void VncCaptureThread::stop()
{
    {
        std::lock_guard<std::mutex> lock(mLock);
        mRunning = false;
        mCond.notify_all();
    }

    if (mThread.joinable())
        mThread.join();
}

bool VncCaptureThread::isRunning() const
{
    return mRunning.load();
}

bool VncCaptureThread::isBusy() const
{
    return mBusy.load();
}

void VncCaptureThread::postFrame(GLuint   pboId,
                                  GLsync   sync,
                                  uint32_t pboWidth,
                                  uint32_t pboHeight,
                                  bool     bridgeMode)
{
    std::lock_guard<std::mutex> lock(mLock);

    // Drop any unconsumed previous frame's sync to avoid leaking GL objects
    if (mPending.valid && mPending.sync)
    {
        glDeleteSync(mPending.sync);
    }

    mPending.pboId      = pboId;
    mPending.sync       = sync;
    mPending.pboWidth   = pboWidth;
    mPending.pboHeight  = pboHeight;
    mPending.bridgeMode = bridgeMode;
    mPending.valid      = true;
    mBusy               = true;

    mCond.notify_one();
}

void VncCaptureThread::captureLoop(EGLDisplay display, EGLContext parentContext)
{
    // ------------------------------------------------------------------
    // Create a shared GLES3 context so PBO objects are accessible here
    // ------------------------------------------------------------------
    EGLint configId = EGL_DONT_CARE;
    if (eglQueryContext(display, parentContext, EGL_CONFIG_ID, &configId) == EGL_FALSE)
    {
        Logger::log(LogLevel::Error, "%s: eglQueryContext failed (err=0x%X)", __func__, eglGetError());
        mRunning = false;
        return;
    }

    const EGLint filterAttribs[] = { EGL_CONFIG_ID, configId, EGL_NONE };
    EGLConfig eglConfigs[8];
    EGLint    configCount = 0;

    if (!eglChooseConfig(display, filterAttribs, eglConfigs, 8, &configCount) || configCount <= 0)
    {
        Logger::log(LogLevel::Error, "%s: eglChooseConfig failed (err=0x%X)", __func__, eglGetError());
        mRunning = false;
        return;
    }

    const EGLint ctxAttribs[] = { EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE };
    EGLContext sharedCtx = eglCreateContext(display, eglConfigs[0], parentContext, ctxAttribs);
    if (sharedCtx == EGL_NO_CONTEXT)
    {
        Logger::log(LogLevel::Error, "%s: eglCreateContext failed (err=0x%X)", __func__, eglGetError());
        mRunning = false;
        return;
    }

    // Surfaceless context: no window surface required for PBO operations
    if (eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, sharedCtx) == EGL_FALSE)
    {
        Logger::log(LogLevel::Error, "%s: eglMakeCurrent failed (err=0x%X)", __func__, eglGetError());
        eglDestroyContext(display, sharedCtx);
        mRunning = false;
        return;
    }

    Logger::log(LogLevel::Information, "%s: VncCaptureThread started with shared EGL context", __func__);

    // ------------------------------------------------------------------
    // Main capture loop
    // ------------------------------------------------------------------
    std::unique_lock<std::mutex> lock(mLock);

    while (mRunning.load())
    {
        // Wait until a frame is posted or we are asked to stop
        mCond.wait(lock, [this] { return !mRunning.load() || mPending.valid; });

        if (!mRunning.load())
            break;

        // Take ownership of the pending frame descriptor
        PboFrame frame   = mPending;
        mPending.valid   = false;
        mPending.sync    = nullptr; // capture thread now owns it

        lock.unlock();

        // ------------------------------------------------------------------
        // 1. Wait for the GPU DMA transfer to complete (fence sync)
        //    glClientWaitSync is valid on this shared context because sync
        //    objects are shared across contexts in the same share group.
        // ------------------------------------------------------------------
        const GLenum waitResult = glClientWaitSync(frame.sync,
                                                    GL_SYNC_FLUSH_COMMANDS_BIT,
                                                    250000000ULL /* 250 ms in ns */);
        glDeleteSync(frame.sync);
        frame.sync = nullptr;

        if ((waitResult != GL_ALREADY_SIGNALED) && (waitResult != GL_CONDITION_SATISFIED))
        {
            Logger::log(LogLevel::Error, "%s: glClientWaitSync timed out or failed (result=0x%X)",
                        __func__, waitResult);

            // Notify caller of failure so bridge mode can signal an error
            if (mFrameReadyCallback)
                mFrameReadyCallback(nullptr, frame.pboWidth, frame.pboHeight, frame.bridgeMode);

            mBusy = false;
            lock.lock();
            continue;
        }

        // ------------------------------------------------------------------
        // 2. Map the PBO into CPU-visible RAM and invoke the callback.
        //    glMapBufferRange is valid here because the PBO (by GLuint) is
        //    shared with the render context.
        // ------------------------------------------------------------------
        glBindBuffer(GL_PIXEL_PACK_BUFFER, frame.pboId);

        const uint8_t* pixels = static_cast<const uint8_t*>(
            glMapBufferRange(GL_PIXEL_PACK_BUFFER,
                             0,
                             static_cast<GLsizeiptr>(frame.pboWidth) * frame.pboHeight * 4,
                             GL_MAP_READ_BIT));

        if (pixels)
        {
            if (mFrameReadyCallback)
                mFrameReadyCallback(pixels, frame.pboWidth, frame.pboHeight, frame.bridgeMode);

            glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
        }
        else
        {
            Logger::log(LogLevel::Error, "%s: glMapBufferRange failed (GL error=0x%X)",
                        __func__, glGetError());

            if (mFrameReadyCallback)
                mFrameReadyCallback(nullptr, frame.pboWidth, frame.pboHeight, frame.bridgeMode);
        }

        glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

        mBusy = false;
        lock.lock();
    }

    // ------------------------------------------------------------------
    // Clean up EGL resources
    // ------------------------------------------------------------------
    eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroyContext(display, sharedCtx);

    Logger::log(LogLevel::Information, "%s: VncCaptureThread terminated", __func__);
}

} // namespace RdkWindowManager

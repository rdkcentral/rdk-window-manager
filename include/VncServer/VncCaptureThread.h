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
#pragma once

#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <functional>
#include <cstdint>

#include <EGL/egl.h>
#include <GLES3/gl3.h>

namespace RdkWindowManager
{

/**
 * Dedicated thread that owns all post-DMA GPU work for VNC pixel capture.
 *
 * Pattern (mirrors WesterosWindowManager / ScreenCaptureThread from the
 * RDK AppManager reference):
 *
 *  GL render thread (VncFrameBuffer::publish):
 *    1. glReadPixels(... nullptr)  – non-blocking, starts GPU→PBO DMA
 *    2. glFenceSync(...)           – non-blocking fence
 *    3. postFrame(pboId, sync, …) – hands ownership of sync to this thread
 *    4. returns immediately
 *
 *  VncCaptureThread:
 *    1. Waits for a posted frame via condition variable
 *    2. glClientWaitSync(sync, …) – waits for DMA completion (shared ctx)
 *    3. glMapBufferRange(…)       – maps PBO into CPU RAM
 *    4. Invokes FrameReadyCallback with raw RGBA pixel pointer
 *    5. glUnmapBuffer / glDeleteSync, loops back to wait
 *
 * The thread creates its own shared GLES3 EGL context so that PBO
 * operations can run concurrently with the render thread without
 * stalling the GPU pipeline.
 */
class VncCaptureThread
{
public:
    /**
     * Callback invoked on the capture thread when pixel data is available
     * (or when a capture attempt failed).
     *
     * @param pixels     Pointer to raw RGBA data (height×width×4 bytes,
     *                   bottom-to-top row order as returned by GL).
     *                   nullptr indicates a capture failure.
     * @param pboWidth   Width of the PBO capture buffer.
     * @param pboHeight  Height of the PBO capture buffer.
     * @param bridgeMode true when the frame was requested by VncBridgeServer.
     */
    using FrameReadyCallback = std::function<void(const uint8_t* pixels,
                                                   uint32_t pboWidth,
                                                   uint32_t pboHeight,
                                                   bool bridgeMode)>;

    VncCaptureThread();
    ~VncCaptureThread();

    VncCaptureThread(const VncCaptureThread&)            = delete;
    VncCaptureThread& operator=(const VncCaptureThread&) = delete;

    /**
     * Set the callback invoked when frame pixels are ready.
     * Must be called before start().
     */
    void setFrameReadyCallback(FrameReadyCallback cb);

    /**
     * Start the capture thread.  Must be called from the GL render thread
     * while that thread's EGL context is current so that a shared context
     * can be derived from it.
     *
     * @return true on success.
     */
    bool start(EGLDisplay display, EGLContext parentContext);

    /** Stop the capture thread and join. */
    void stop();

    /** Returns true if the thread was successfully started. */
    bool isRunning() const;

    /**
     * Called from the GL thread (non-blocking).
     *
     * Posts a filled PBO for the capture thread to map and deliver.
     * Ownership of @a sync is transferred to this class; it will be
     * deleted by the capture thread after waiting.
     *
     * If a previous frame has not yet been processed its sync is deleted
     * and the new frame takes priority (latest-frame-wins policy).
     *
     * @param pboId      GL buffer object id (shared between both contexts).
     * @param sync       glFenceSync created just after glReadPixels.
     * @param pboWidth   Width of the data stored in the PBO.
     * @param pboHeight  Height of the data stored in the PBO.
     * @param bridgeMode Forwarded verbatim to FrameReadyCallback.
     */
    void postFrame(GLuint   pboId,
                   GLsync   sync,
                   uint32_t pboWidth,
                   uint32_t pboHeight,
                   bool     bridgeMode);

    /**
     * Returns true if the capture thread is still processing the last
     * posted frame (i.e. it has not yet invoked the FrameReadyCallback).
     */
    bool isBusy() const;

private:
    void captureLoop(EGLDisplay display, EGLContext parentContext);

    struct PboFrame
    {
        GLuint   pboId      = 0;
        GLsync   sync       = nullptr;
        uint32_t pboWidth   = 0;
        uint32_t pboHeight  = 0;
        bool     bridgeMode = false;
        bool     valid      = false;
    };

    std::thread             mThread;
    std::atomic<bool>       mRunning { false };
    std::atomic<bool>       mBusy    { false };

    std::mutex              mLock;
    std::condition_variable mCond;
    PboFrame                mPending;

    FrameReadyCallback      mFrameReadyCallback;
};

} // namespace RdkWindowManager

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

class VncCaptureThread
{
public:
    using FrameReadyCallback = std::function<void(const uint8_t* pixels,
                                                   uint32_t pboWidth,
                                                   uint32_t pboHeight,
                                                   bool bridgeMode)>;

    VncCaptureThread();
    ~VncCaptureThread();

    VncCaptureThread(const VncCaptureThread&)            = delete;
    VncCaptureThread& operator=(const VncCaptureThread&) = delete;

    void setFrameReadyCallback(FrameReadyCallback cb);

    bool start(EGLDisplay display, EGLContext parentContext);

    void stop();

    bool isRunning() const;

    void postFrame(GLuint   pboId,
                   GLsync   sync,
                   uint32_t pboWidth,
                   uint32_t pboHeight,
                   bool     bridgeMode);

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

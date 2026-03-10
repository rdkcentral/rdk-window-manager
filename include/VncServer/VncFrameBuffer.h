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
#ifndef RDK_WINDOW_MANAGER_VNCFRAMEBUFFER_H
#define RDK_WINDOW_MANAGER_VNCFRAMEBUFFER_H

#include <thread>
#include <glib.h>
#include <memory>
#include <atomic>
#include "framebuffer.h"

namespace RdkWindowManager {
    class VncFrameBuffer {

    public:
        VncFrameBuffer(uint32_t width, uint32_t height);
        ~VncFrameBuffer();

        void begin();
        void end();
        void draw();
        void publish();
    private:
        bool sendFrameBufferToVNCClient();
        bool initVncFrameBuffer();
        static void onVncFrameSent(gpointer userData);
        bool readPixel();
        uint32_t readAndConvertPixelData(const size_t frameOffset);
        void notifyPixelProcessDone();

        std::atomic<bool> mPixelsProcessingInProgress;
        std::shared_ptr<FrameBuffer> mFrameBuffer;
        uint32_t    mWidth;
        uint32_t    mHeight;
        float       mMatrix[16];
        double      mOpacity;
        uint8_t*    mVncFrameBufferPtr;
        size_t      mVncFrameBufferSize;
        std::vector<uint8_t> mRGBAData;
    };
}
#endif // RDK_WINDOW_MANAGER_VNCFRAMEBUFFER_H



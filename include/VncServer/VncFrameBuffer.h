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

#include <glib.h>
#include <memory>
#include <vector>
#include <cstdint>
#include "framebuffer.h"
#include "VncCaptureThread.h"

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
        // ---- Frame delivery (called on the VncCaptureThread) ----
        bool sendFrameBufferToVNCClient();
        uint32_t readAndConvertPixelData(const size_t frameOffset);
        static void onVncFrameSent(gpointer userData);

        // ---- Shared-memory VNC output buffer ----
        bool initVncFrameBuffer();

        // ---- PBO lifecycle (called on the GL render thread) ----
        bool initPBOs();
        void destroyPBOs();

        /**
         * Called from publish() – non-blocking on the GL thread.
         * Kicks off glReadPixels into a PBO and posts the PBO to
         * VncCaptureThread for async mapping and delivery.
         *
         * @param bridgeMode true when the frame is for VncBridgeServer.
         */
        void startAsyncCapture(bool bridgeMode);

        /**
         * Callback installed into VncCaptureThread.
         * Invoked on the capture thread once PBO mapping succeeds.
         * Copies/scales pixels into mRGBAData then calls the appropriate
         * send path (VNC socket or bridge server).
         */
        void onFrameReady(const uint8_t* pixels,
                          uint32_t pboWidth,
                          uint32_t pboHeight,
                          bool bridgeMode);

        // ---- Members ----
        std::shared_ptr<FrameBuffer> mFrameBuffer;
        // Intermediate VNC-sized FBO used as the GPU blit destination.
        // glBlitFramebuffer downscales the source (e.g. 1920x1080) to this
        // FBO (mWidth x mHeight) so glReadPixels only DMA's the smaller buffer.
        std::shared_ptr<FrameBuffer> mCaptureFbo;
        uint32_t    mWidth;
        uint32_t    mHeight;
        float       mMatrix[16];
        double      mOpacity;

        uint8_t*    mVncFrameBufferPtr;
        size_t      mVncFrameBufferSize;

        // Intermediate RGBA buffer written by VncCaptureThread,
        // consumed by readAndConvertPixelData on the same thread.
        std::vector<uint8_t> mRGBAData;

        // Ping-pong PBOs – allocated on the GL thread, shared with the
        // capture thread's EGL context.
        static constexpr int kPboCount = 2;
        GLuint   mPboIds[kPboCount];
        int      mPboWriteIndex;
        bool     mPboInitialized;

        // Dedicated thread that owns glClientWaitSync + glMapBufferRange
        VncCaptureThread mCaptureThread;
    };

} // namespace RdkWindowManager

#endif // RDK_WINDOW_MANAGER_VNCFRAMEBUFFER_H



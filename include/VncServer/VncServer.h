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
#ifndef RDK_WINDOW_MANAGER_VNCSERVER_H
#define RDK_WINDOW_MANAGER_VNCSERVER_H

#include <thread>
#include <atomic>
#include <mutex>
#include <glib.h>
#include "VncSoupTcpServer.h"

namespace RdkWindowManager {
    class VncServer {

    public:
        static VncServer& getInstance();
        bool start(uint32_t width, uint32_t height);
        void stop();
        uint32_t getFrameBufferWidth();
        uint32_t getFrameBufferHeight();
        std::string getFriendlyName();

    private:
        VncServer();
        ~VncServer();

        VncServer(const VncServer&) = delete;
        VncServer& operator=(const VncServer&) = delete;

        static void mainLoopThread(GMainLoop* loop);
        bool applyIptableRule();
        bool deleteIptableRule();

        uint32_t mPort;
        uint32_t mWidth;
        uint32_t mHeight;
        std::atomic<bool> mIsRunning;
        VncSoupTcpServer* mVncSoupTcpServer;
        GMainLoop* mGMainLoop;
        std::thread mGMainLoopThread;
    };
}
#endif // RDK_WINDOW_MANAGER_VNCSERVER_H

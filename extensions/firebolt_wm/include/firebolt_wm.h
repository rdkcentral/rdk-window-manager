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
 */

#ifndef FIREBOLT_WM_H
#define FIREBOLT_WM_H
#include <map>
#include <queue>
#include <condition_variable>
#include <mutex>
#include <thread>
#include "westeros-compositor.h"
#include "firebolt_wm_protocol_server.h"
#include "compositorcontroller.h"


typedef struct
{
    uint32_t            clientId;
    std::string         clientName;
    struct wl_display*  display;
    wl_resource*        resource;
} FireboltWmClientInfo;

struct FireboltWmEventMessage {
    std::string clientName;
    std::string eventName;
};

class FireboltWindowManager
{
    public:
        FireboltWindowManager();
        ~FireboltWindowManager();
        FireboltWmClientInfo* getFireboltWmClientInfo(wl_resource *resource);

        typedef std::map<wl_resource*, FireboltWmClientInfo*> ClientListMap;
        ClientListMap                 mClientListMap;

        FireboltWindowManager        *mInstance;
        static std::mutex             mContextLock;

        wl_global                    *mWlGlobal;
        WstCompositor                *mWstCompositor;
        struct wl_display            *mWlDisplay;
        std::string                   mWstDisplayName;


        std::queue<FireboltWmEventMessage> mEventQueue;
        std::mutex mQueueMutex;
        std::condition_variable mQueueCV;
        std::thread mWorkerThread;
        bool mThreadRunning = false;

        void fireboltWMEventWorkerThread (void);

        void createFireboltWMEventWorker(void);
        void deleteFireboltWMEventWorker(void);

        bool notify_client_event(const char* clientName,
                            const std::string& eventName,
                            void (*fbWindowManagerEventCallback)(wl_resource*, const char*));

        /* Firebolt windowmanager Listener */
        static std::shared_ptr<RdkWindowManager::FireboltExtensionEventListener> mFireboltWindowManagerEventListener;
        class FireboltWindowManagerListener : public RdkWindowManager::FireboltExtensionEventListener
        {
            public:
                FireboltWindowManagerListener() = default;
                ~FireboltWindowManagerListener() = default;

                /* Events listeners */
                void client_connected(const char* clientName) override;
                void client_disconnected(const char* clientName) override;

                void postEventToWorker(const char* clientName,const std::string& eventName);

        };
};
#endif

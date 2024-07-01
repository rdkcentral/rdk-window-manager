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
#include "westeros-compositor.h"
#include "firebolt_wm_protocol_server.h"

typedef struct
{
    uint32_t            clientId;
    std::string         clientName;
    struct wl_display*  display;
    wl_resource*        resource;
} FireboltWmClientInfo;

class FireboltWindowManager
{
    public:
        FireboltWindowManager();
        ~FireboltWindowManager();
        FireboltWmClientInfo* getFireboltWmClientInfo(wl_resource *resource);

        typedef std::map<wl_resource*, FireboltWmClientInfo*> ClientListMap;
        ClientListMap                 mClientListMap;

        static FireboltWindowManager *mInstance;
        static std::mutex             mContextLock;

        wl_global                    *mWlGlobal;
        WstCompositor                *mWstCompositor;
        struct wl_display            *mWlDisplay;
        std::string                   mWstDisplayName;
};
#endif

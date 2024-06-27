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

#ifndef FIREBOLT_SHELL_H
#define FIREBOLT_SHELL_H
#include <map>
#include "westeros-compositor.h"
#include "firebolt_shell_protocol_server.h"


class FireboltShell
{
    public:
        FireboltShell();
        ~FireboltShell();
        bool getClientNameByResource(wl_resource *resource, std::string& clientName);

        static FireboltShell *mInstance;
        static std::mutex mContextLock;

        WstCompositor     *mWstCompositor;
        struct wl_display *mWlDisplay;
        wl_resource       *mWlResource;
        wl_global         *mWlGlobal;
        std::string        mWstDisplayName;

        typedef std::map<wl_resource*, std::string> ClientNamesMap;
        ClientNamesMap mClientNamesMap;
};
#endif

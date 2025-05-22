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

#include <typeindex>
#include <string>
#include <map>
#include <vector>
#include "rdkwindowmanagerdata.h"

namespace RdkWindowManager
{
    class RdkWindowManagerEventListener
    {
        public:
          virtual void onApplicationConnected(const std::string& client) {}
          virtual void onApplicationDisconnected(const std::string& client)  {}
          virtual void onApplicationTerminated(const std::string& client) {}
          virtual void onReady(const std::string& client) {}
          virtual void onUserInactive(const double minutes) {}
          virtual void onKeyEvent(const uint32_t keyCode, const uint32_t flags, const bool keyDown) {}
          virtual void onSizeChangeComplete(const std::string& client) {}
    };

    const std::string RDK_WINDOW_MANAGER_EVENT_APPLICATION_CONNECTED = "onApplicationConnected";
    const std::string RDK_WINDOW_MANAGER_EVENT_APPLICATION_DISCONNECTED = "onApplicationDisconnected";
    const std::string RDK_WINDOW_MANAGER_EVENT_APPLICATION_TERMINATED = "onApplicationTerminated";
    const std::string RDK_WINDOW_MANAGER_EVENT_APPLICATION_FIRST_FRAME = "onApplicationFirstFrame";
    const std::string RDK_WINDOW_MANAGER_EVENT_USER_INACTIVE = "onUserInactive";
    const std::string RDK_WINDOW_MANAGER_EVENT_KEY = "onKeyEvent";
    const std::string RDK_WINDOW_MANAGER_EVENT_SIZE_CHANGE_COMPLETE = "onSizeChangeComplete";
}

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
          virtual void onApplicationLaunched(const std::string& client) {}
          virtual void onApplicationConnected(const std::string& client) {}
          virtual void onApplicationDisconnected(const std::string& client)  {}
          virtual void onApplicationTerminated(const std::string& client) {}
          virtual void onApplicationFirstFrame(const std::string& client) {}
          virtual void onApplicationSuspended(const std::string& client) {}
          virtual void onApplicationResumed(const std::string& client) {}
          virtual void onApplicationActivated(const std::string& client) {}
          virtual void onUserInactive(const double minutes) {}
          virtual void onDeviceLowRamWarning(const int32_t freeKb, const int32_t availableKb, const int32_t usedSwapKb) {}
          virtual void onDeviceCriticallyLowRamWarning(const int32_t freeKb, const int32_t availableKb, const int32_t usedSwapKb) {}
          virtual void onDeviceLowRamWarningCleared(const int32_t freeKb, const int32_t availableKb, const int32_t usedSwapKb) {}
          virtual void onDeviceCriticallyLowRamWarningCleared(const int32_t freeKb, const int32_t availableKb, const int32_t usedSwapKb) {}
          virtual void onAnimation(std::vector<std::map<std::string, RdkWindowManagerData>>& animationData) {}
          virtual void onEasterEgg(const std::string& name, const std::string& actionJson) {}
          virtual void onPowerKey() {}
          virtual void onKeyEvent(const uint32_t keyCode, const uint32_t flags, const bool keyDown) {}
          virtual void onSizeChangeComplete(const std::string& client) {}
    };

    const std::string RDK_WINDOW_MANAGER_EVENT_APPLICATION_LAUNCHED = "onApplicationLaunched";
    const std::string RDK_WINDOW_MANAGER_EVENT_APPLICATION_CONNECTED = "onApplicationConnected";
    const std::string RDK_WINDOW_MANAGER_EVENT_APPLICATION_DISCONNECTED = "onApplicationDisconnected";
    const std::string RDK_WINDOW_MANAGER_EVENT_APPLICATION_TERMINATED = "onApplicationTerminated";
    const std::string RDK_WINDOW_MANAGER_EVENT_APPLICATION_FIRST_FRAME = "onApplicationFirstFrame";
    const std::string RDK_WINDOW_MANAGER_EVENT_APPLICATION_SUSPENDED = "onApplicationSuspended";
    const std::string RDK_WINDOW_MANAGER_EVENT_APPLICATION_RESUMED = "onApplicationResumed";
    const std::string RDK_WINDOW_MANAGER_EVENT_APPLICATION_ACTIVATED = "onApplicationActivated";
    const std::string RDK_WINDOW_MANAGER_EVENT_USER_INACTIVE = "onUserInactive";
    const std::string RDK_WINDOW_MANAGER_EVENT_DEVICE_LOW_RAM_WARNING = "onDeviceLowRamWarning";
    const std::string RDK_WINDOW_MANAGER_EVENT_DEVICE_CRITICALLY_LOW_RAM_WARNING = "onDeviceCriticallyLowRamWarning";
    const std::string RDK_WINDOW_MANAGER_EVENT_DEVICE_LOW_RAM_WARNING_CLEARED = "onDeviceLowRamWarningCleared";
    const std::string RDK_WINDOW_MANAGER_EVENT_DEVICE_CRITICALLY_LOW_RAM_WARNING_CLEARED = "onDeviceCriticallyLowRamWarningCleared";
    const std::string RDK_WINDOW_MANAGER_EVENT_ANIMATION = "onAnimation";
    const std::string RDK_WINDOW_MANAGER_EVENT_EASTER_EGG = "onEasterEgg";
    const std::string RDK_WINDOW_MANAGER_EVENT_POWER_KEY = "onPowerKey";
    const std::string RDK_WINDOW_MANAGER_EVENT_KEY = "onKeyEvent";
    const std::string RDK_WINDOW_MANAGER_EVENT_SIZE_CHANGE_COMPLETE = "onSizeChangeComplete";

    class FireboltExtensionEventListener
    {
        public:
          virtual void on_focus(const char* clientName) {}
          virtual void on_blur(const char* clientName) {}
          virtual void client_connected(const char* clientName) {}
          virtual void client_disconnected(const char* clientName) {}
    };

    class ExtensionEventListener
    {
        public:
          virtual ~ExtensionEventListener() = default;
          virtual void onClientConfigChanged(const std::string& clientName,
                                             bool visible,
                                             int zOrder,
                                             double opacity,
                                             int x,
                                             int y,
                                             unsigned width,
                                             unsigned height) {}
          virtual void onOwnerChanged(int ownerId, const std::string& clientName) {}
    };


const std::string RDK_WINDOW_MANAGER_FIREBOLT_EXTENTION_EVENT_ON_FOCUS = "on_focus";
const std::string RDK_WINDOW_MANAGER_FIREBOLT_EXTENTION_EVENT_ON_BLUR = "on_blur";
const std::string RDK_WINDOW_MANAGER_FIREBOLT_EXTENSION_EVENT_CLIENT_CONNECTED      = "client_connected";
const std::string RDK_WINDOW_MANAGER_FIREBOLT_EXTENSION_EVENT_CLIENT_DISCONNECTED = "client_disconnected";
const std::string RDK_WINDOW_MANAGER_EXTENSION_EVENT_CLIENT_CONFIG_CHANGED = "onClientConfigChanged";
const std::string RDK_WINDOW_MANAGER_EXTENSION_EVENT_OWNER_CHANGED = "onOwnerChanged";
}

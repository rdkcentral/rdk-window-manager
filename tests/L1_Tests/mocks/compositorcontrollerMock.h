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

#include <gmock/gmock.h>
#include "compositorcontrollerImpl.h"
namespace RdkWindowManager
{
class MockCompositorControllerImpl : public CompositorControllerImpl
{
public:
    MOCK_METHOD(bool, enableVirtualDisplay, (const std::string& client, const bool enable), (override));
    MOCK_METHOD(bool, getClientInfo, (const std::string& client, ClientInfo& ci), (override));
    MOCK_METHOD(bool, setClientInfo, (const std::string& client, const ClientInfo& ci), (override));
    MOCK_METHOD(bool, setVirtualResolution, (const std::string& client, uint32_t virtualWidth, uint32_t virtualHeight), (override));
    MOCK_METHOD(bool, getScreenResolution, (uint32_t &width, uint32_t &height), (override));
    MOCK_METHOD(bool, createDisplay, (const std::string& client, const std::string& displayName, uint32_t displayWidth, uint32_t displayHeight, bool virtualDisplayEnabled, uint32_t virtualWidth, uint32_t virtualHeight, bool topmost, bool focus), (override));
    MOCK_METHOD(bool, getTopmost, (std::string& client), (override));
    MOCK_METHOD(bool, getFocused, (std::string& client), (override));
    MOCK_METHOD(bool, kill, (const std::string& client), (override));
    MOCK_METHOD(bool, setFocus, (const std::string& client), (override));
    MOCK_METHOD(bool, setBounds, (const std::string& client, uint32_t x, uint32_t y, uint32_t width, uint32_t height), (override));
    MOCK_METHOD(bool, getVirtualDisplayEnabled, (const std::string& client, bool &enabled), (override));
    MOCK_METHOD(bool, getClients, (std::vector<std::string>& clients), (override));
    MOCK_METHOD(bool, getClientName, (WstCompositor* compositor, std::string& clientName), (override));
    MOCK_METHOD(bool, getFireboltSurface, (const std::string& client, int surfaceId, uint32_t type), (override));
	MOCK_METHOD(bool, setFireboltSurfaceZorder, (const std::string& client, int surfaceId, int zOrder), (override));
    MOCK_METHOD(bool, setFireboltSurfaceName, (const std::string& client, int surfaceId, const std::string& surfaceName), (override));
    MOCK_METHOD(bool, setFireboltSurfaceOpacity, (const std::string& client, int surfaceId, double opacity), (override));
    MOCK_METHOD(bool, setFireboltSurfaceBounds, (const std::string& client, int surfaceId, int32_t x, int32_t y, uint32_t width, uint32_t height), (override));
    MOCK_METHOD(bool, setFireboltSurfaceCrop, (const std::string& client, int surfaceId, int32_t sx, int32_t sy, uint32_t swidth, uint32_t sheight), (override));
    MOCK_METHOD(bool, setFireboltSurfaceVisibility, (const std::string& client, int surfaceId, bool visible), (override));
    MOCK_METHOD(bool, fireboltSurfaceDestroy, (const std::string& client, int surfaceId), (override));
    MOCK_METHOD(bool, getSurfaceInfo, (const std::string& client, int surfaceId, FireboltSurfaceInfo& si),(override));
    MOCK_METHOD(bool, addFireboltExtensionListener, (const std::string& client, std::shared_ptr<FireboltExtensionEventListener> listener),(override));
    MOCK_METHOD(bool, removeFireboltExtensionListener, (const std::string& client, std::shared_ptr<FireboltExtensionEventListener> listener),(override));
    MOCK_METHOD(int, addExtensionEventListener, (std::shared_ptr<class ExtensionEventListener> listener),(override));
    MOCK_METHOD(bool, removeExtensionEventListener, (int listenerTag),(override));
};
}

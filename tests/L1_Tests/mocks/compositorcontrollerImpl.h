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

#ifndef RDK_WINDOW_MANAGER_COMPOSITOR_CONTROLLERIMPL_H
#define RDK_WINDOW_MANAGER_COMPOSITOR_CONTROLLERIMPL_H

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <algorithm>

namespace RdkWindowManager
{
	
struct ClientInfo;

class CompositorControllerImpl {
public:
    virtual ~CompositorControllerImpl() = default;

    virtual bool enableVirtualDisplay(const std::string& client, bool enable) = 0;
    virtual bool getClientInfo(const std::string& client, ClientInfo& ci) = 0;
    virtual bool setClientInfo(const std::string& client, const ClientInfo& ci) = 0;
    virtual bool setVirtualResolution(const std::string& client, uint32_t virtualWidth, uint32_t virtualHeight) = 0;
    virtual bool getScreenResolution(uint32_t& width, uint32_t& height) = 0;
    virtual bool createDisplay(const std::string& client, const std::string& displayName,
    uint32_t displayWidth, uint32_t displayHeight, bool virtualDisplayEnabled, uint32_t virtualWidth, uint32_t virtualHeight,
    bool topmost, bool focus, bool autodestroy) = 0;
    virtual bool getTopmost(std::string& client) = 0;
    virtual bool getFocused(std::string& client) = 0;
    virtual bool kill(const std::string& client) = 0;
    virtual bool setFocus(const std::string& client) = 0;
    virtual bool setBounds(const std::string& client, uint32_t x, uint32_t y, uint32_t width, uint32_t height) = 0;
    virtual bool getVirtualDisplayEnabled(const std::string& client, bool& enabled) = 0;
    virtual bool getClients(std::vector<std::string>& clients) = 0;
    virtual bool getClientName(WstCompositor* compositor, std::string& clientName) = 0;
    virtual bool getFireboltSurface(const std::string& client, int surfaceId, uint32_t type) = 0;
    virtual bool setFireboltSurfaceZorder(const std::string& client, int surfaceId, int zOrder) = 0;
    virtual bool setFireboltSurfaceName(const std::string& client, int surfaceId, const std::string& surfaceName) = 0;
    virtual bool setFireboltSurfaceOpacity(const std::string& client, int surfaceId, double opacity) = 0;
    virtual bool setFireboltSurfaceBounds(const std::string& client, int surfaceId, int32_t x, int32_t y, uint32_t width, uint32_t height) = 0;
    virtual bool setFireboltSurfaceCrop(const std::string& client, int surfaceId, int32_t sx, int32_t sy, uint32_t swidth, uint32_t sheight) = 0;
    virtual bool setFireboltSurfaceVisibility(const std::string& client, int surfaceId, bool visible) = 0;
    virtual bool fireboltSurfaceDestroy(const std::string& client, int surfaceId) = 0;
    virtual bool getSurfaceInfo(const std::string& client, int surfaceId, FireboltSurfaceInfo& si) = 0;
};

}
#endif //RDK_WINDOW_MANAGER_COMPOSITOR_CONTROLLERIMPL_H

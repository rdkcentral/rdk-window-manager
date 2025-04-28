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

#include <gmock/gmock.h>
#include "compositorcontroller.h"
#include "compositorcontrollerImpl.h" 

namespace RdkWindowManager
{
        
    CompositorControllerImpl* CompositorController::impl = nullptr;

    CompositorController::CompositorController() {}

    void CompositorController::setImpl(CompositorControllerImpl* newImpl)
    {
       ASSERT_TRUE(((nullptr == impl) && (nullptr != newImpl)) || ((nullptr != impl) && (nullptr == newImpl)));
       impl = newImpl;
    }
        
    bool CompositorController::enableVirtualDisplay(const std::string& client, const bool enable)
    {

       assert(nullptr != impl);
       return impl->enableVirtualDisplay(client, enable);
    }
        
    bool CompositorController::getClientInfo(const std::string& client, ClientInfo& ci)
    {
        assert(nullptr != impl);
        return impl->getClientInfo(client, ci);
    }
    bool CompositorController::setClientInfo(const std::string& client, const ClientInfo& ci)
    {
                
        assert(nullptr != impl);
        return impl->setClientInfo(client, ci);
    }
    bool CompositorController::setVirtualResolution(const std::string& client, const uint32_t virtualWidth, const uint32_t virtualHeight)
    {
                
         assert(nullptr != impl);
         return impl->setVirtualResolution(client,virtualWidth,virtualHeight);
    }
    bool CompositorController::getScreenResolution(uint32_t &width, uint32_t &height)
    {
         assert(nullptr != impl);
         return impl->getScreenResolution(width,height);
    }
    bool CompositorController::createDisplay(const std::string& client, const std::string& displayName,
                                              uint32_t displayWidth, uint32_t displayHeight, bool virtualDisplayEnabled, uint32_t virtualWidth,
                                              uint32_t virtualHeight,bool topmost, bool focus , bool autodestroy)
    {
          assert(nullptr != impl);
          return impl->createDisplay(client,displayName,displayWidth,displayHeight,virtualDisplayEnabled,virtualWidth,
                                     virtualHeight,topmost,focus,autodestroy);
    }
        
    bool CompositorController::getTopmost(std::string& client)
    {
           assert(nullptr != impl);
           return impl->getTopmost(client);
    }
    bool CompositorController::getFocused(std::string& client)
    {
           assert(nullptr != impl);
           return impl->getFocused(client);
    }
    bool CompositorController::kill(const std::string& client)
    {
           assert(nullptr != impl);
           return impl->kill(client);
    }
    bool CompositorController::setFocus(const std::string& client)
    {
           assert(nullptr != impl);
           return impl->setFocus(client);
    }
    bool CompositorController::setBounds(const std::string& client, const uint32_t x, const uint32_t y, const uint32_t width, const uint32_t height)
    {
           assert(nullptr != impl);
           return impl->setBounds(client,x,y,width,height);
    }
    bool CompositorController::getVirtualDisplayEnabled(const std::string& client, bool &enabled)
    {
           assert(nullptr != impl);
           return impl->getVirtualDisplayEnabled(client,enabled);
    }
    bool CompositorController::getClients(std::vector<std::string>& clients)
    {
           assert(nullptr != impl);
           return impl->getClients(clients);
    }
    bool CompositorController::getClientName(WstCompositor* compositor, std::string& clientName)
    {
           assert(nullptr != impl);
           return impl->getClientName(compositor,clientName);
    }
    bool CompositorController::getFireboltSurface(const std::string& client, int surfaceId, uint32_t type)
    {
           assert(nullptr != impl);
           return impl->getFireboltSurface(client,surfaceId,type);
    }
	
	bool CompositorController::setFireboltSurfaceZorder(const std::string& client, int surfaceId, int zOrder)
    {
        assert(nullptr != impl);
        return impl->setFireboltSurfaceZorder(client, surfaceId, zOrder);
    }

    bool CompositorController::setFireboltSurfaceName(const std::string& client, int surfaceId, const std::string& surfaceName)
    {
        assert(nullptr != impl);
        return impl->setFireboltSurfaceName(client, surfaceId, surfaceName);
    }

    bool CompositorController::setFireboltSurfaceOpacity(const std::string& client, int surfaceId, double opacity)
    {
        assert(nullptr != impl);
        return impl->setFireboltSurfaceOpacity(client, surfaceId, opacity);
    }

    bool CompositorController::setFireboltSurfaceBounds(const std::string& client, int surfaceId, int32_t x, int32_t y, uint32_t width, uint32_t height)
    {
        assert(nullptr != impl);
        return impl->setFireboltSurfaceBounds(client, surfaceId, x, y, width, height);
    }

    bool CompositorController::setFireboltSurfaceCrop(const std::string& client, int surfaceId, int32_t sx, int32_t sy, uint32_t swidth, uint32_t sheight)
    {
        assert(nullptr != impl);
        return impl->setFireboltSurfaceCrop(client, surfaceId, sx, sy, swidth, sheight);
    }

    bool CompositorController::setFireboltSurfaceVisibility(const std::string& client, int surfaceId, bool visible)
    {
        assert(nullptr != impl);
        return impl->setFireboltSurfaceVisibility(client, surfaceId, visible);
    }

    bool CompositorController::fireboltSurfaceDestroy(const std::string& client, int surfaceId)
    {
        assert(nullptr != impl);
        return impl->fireboltSurfaceDestroy(client, surfaceId);
    }
    bool CompositorController::getSurfaceInfo(const std::string& client, int surfaceId, FireboltSurfaceInfo& si)
    {
        assert(nullptr != impl);
        return impl->getSurfaceInfo(client,surfaceId,si);
    }


    bool (*enableVirtualDisplay)(const std::string&, const bool) = &CompositorController::enableVirtualDisplay;
    bool (*getClientInfo)(const std::string&, ClientInfo&) = &CompositorController::getClientInfo;
    bool (*setClientInfo)(const std::string&, const ClientInfo&) = &CompositorController::setClientInfo;
    bool (*setVirtualResolution)(const std::string&, uint32_t, uint32_t) = &CompositorController::setVirtualResolution;
    bool (*getScreenResolution)(uint32_t&, uint32_t&) = &CompositorController::getScreenResolution;
    bool (*createDisplay)(const std::string&, const std::string&, uint32_t, uint32_t, bool, uint32_t, uint32_t, bool, bool, bool) = &CompositorController::createDisplay;
    bool (*getTopmost)(std::string&) = &CompositorController::getTopmost;
    bool (*getFocused)(std::string&) = &CompositorController::getFocused;
    bool (*kill)(const std::string&) = &CompositorController::kill;
    bool (*setFocus)(const std::string&) = &CompositorController::setFocus;
    bool (*setBounds)(const std::string&, uint32_t, uint32_t, uint32_t, uint32_t) = &CompositorController::setBounds;
    bool (*getVirtualDisplayEnabled)(const std::string&, bool&) = &CompositorController::getVirtualDisplayEnabled;
    bool (*getClients)(std::vector<std::string>&) = &CompositorController::getClients;
    bool (*getClientName)(WstCompositor*, std::string&) = &CompositorController::getClientName;
    bool (*getFireboltSurface)(const std::string&,int,uint32_t) = &CompositorController::getFireboltSurface;
    bool (*setFireboltSurfaceZorder)(const std::string&, int, int) = &CompositorController::setFireboltSurfaceZorder;
    bool (*setFireboltSurfaceName)(const std::string&, int, const std::string&) = &CompositorController::setFireboltSurfaceName;
    bool (*setFireboltSurfaceOpacity)(const std::string&, int, double) = &CompositorController::setFireboltSurfaceOpacity;
    bool (*setFireboltSurfaceBounds)(const std::string&, int, int32_t, int32_t, uint32_t, uint32_t) = &CompositorController::setFireboltSurfaceBounds;
    bool (*setFireboltSurfaceCrop)(const std::string&, int, int32_t, int32_t, uint32_t, uint32_t) = &CompositorController::setFireboltSurfaceCrop;
    bool (*setFireboltSurfaceVisibility)(const std::string&, int, bool) = &CompositorController::setFireboltSurfaceVisibility;
    bool (*fireboltSurfaceDestroy)(const std::string&, int) = &CompositorController::fireboltSurfaceDestroy;
    bool (*getSurfaceInfo)(const std::string&, int, FireboltSurfaceInfo& )=&CompositorController::getSurfaceInfo;

}

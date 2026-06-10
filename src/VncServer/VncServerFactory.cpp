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

#include "VncServerFactory.h"
#include "VncServer.h"
#include "VncSoupTcpServer.h"
#include "logger.h"

#ifdef ENABLE_RDKWINDOWMANAGER_VNCSERVER2
#include "VncBridgeServer.h"
#endif

namespace RdkWindowManager
{

VncServerFactory& VncServerFactory::getInstance()
{
    static VncServerFactory instance;
    return instance;
}

bool VncServerFactory::initializeVncServer(uint32_t width, uint32_t height)
{
    if (mInitialized)
    {
        Logger::log(LogLevel::Warning, "%s: VNC server already initialized", __func__);
        return true;
    }

    if (width == 0 || height == 0)
    {
        Logger::log(LogLevel::Error, "%s: Invalid framebuffer dimensions: %ux%u", __func__, width, height);
        return false;
    }

    // Keep VncServer context (width/height/flags) valid in all modes.
    VncServer::getInstance().initializeContext(width, height);

#ifdef ENABLE_RDKWINDOWMANAGER_VNCSERVER2
    Logger::log(LogLevel::Information, "%s: ENABLE_RDKWINDOWMANAGER_VNCSERVER2 is set, initializing bridge mode", __func__);
    if (initializeBridgeMode(width, height))
    {
        mInitialized = true;
        return true;
    }
    else
    {
        Logger::log(LogLevel::Error, "%s: Failed to initialize bridge mode", __func__);
        return false;
    }
#else
    Logger::log(LogLevel::Information, "%s: Initializing internal VNC server mode", __func__);
    if (initializeInternalServerMode(width, height))
    {
        mInitialized = true;
        return true;
    }
    else
    {
        Logger::log(LogLevel::Error, "%s: Failed to initialize internal server mode", __func__);
        return false;
    }
#endif
}

void VncServerFactory::stopVncServer()
{
    if (!mInitialized)
    {
        Logger::log(LogLevel::Information, "%s: VNC server not initialized, nothing to stop", __func__);
        return;
    }

#ifdef ENABLE_RDKWINDOWMANAGER_VNCSERVER2
    Logger::log(LogLevel::Information, "%s: Stopping bridge mode", __func__);
    VncBridgeServer::getInstance().stop();
    VncServer::getInstance().resetContext();
#else
    Logger::log(LogLevel::Information, "%s: Stopping internal VNC server", __func__);
    VncServer::getInstance().stop();
#endif

    mInitialized = false;
}

#ifdef ENABLE_RDKWINDOWMANAGER_VNCSERVER2

bool VncServerFactory::initializeBridgeMode(uint32_t width, uint32_t height)
{
    Logger::log(LogLevel::Information, "%s: Starting VNC bridge server (VNCServer2 integration)", __func__);

    // Start the bridge server
    if (!VncBridgeServer::getInstance().start())
    {
        Logger::log(LogLevel::Error, "%s: Failed to start VncBridgeServer", __func__);
        return false;
    }

    Logger::log(LogLevel::Information, "%s: VNC bridge server started successfully", __func__);
    return true;
}

#else

bool VncServerFactory::initializeInternalServerMode(uint32_t width, uint32_t height)
{
    Logger::log(LogLevel::Information, "%s: Starting internal VNC server (listening on port 5900)", __func__);

    // Start internal VNC server via VncServer singleton
    VncServer& vncServer = VncServer::getInstance();
    if (!vncServer.start(width, height))
    {
        Logger::log(LogLevel::Error, "%s: Failed to start internal VNC server", __func__);
        return false;
    }

    Logger::log(LogLevel::Information, "%s: Internal VNC server started successfully", __func__);
    return true;
}

#endif

} // namespace RdkWindowManager

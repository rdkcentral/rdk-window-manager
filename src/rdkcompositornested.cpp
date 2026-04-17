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

#include "rdkcompositornested.h"
#include "compositorcontroller.h"

#include <iostream>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include "linuxkeys.h"
#include "rdkwindowmanager.h"
#include "logger.h"

namespace RdkWindowManager
{
    bool RdkCompositorNested::createDisplay(const std::string& displayName, const std::string& clientName,
        uint32_t width, uint32_t height, bool virtualDisplayEnabled, uint32_t virtualWidth, uint32_t virtualHeight, int32_t ownerId, int32_t groupId)
    {
        double tFunc = RdkWindowManager::seconds();
        Logger::log(LogLevel::Information, "[createDisplay] START displayName:%s clientName:%s %dx%d", displayName.c_str(), clientName.c_str(), width, height);

        if (width > 0 && height > 0)
        {
            mWidth = width;
            mHeight = height;
        }

        double t0 = RdkWindowManager::seconds();
        mWstContext = WstCompositorCreate();
        Logger::log(LogLevel::Information, "[createDisplay] WstCompositorCreate took %.3f ms, mWstContext:%p", (RdkWindowManager::seconds() - t0) * 1000.0, mWstContext);

        bool error = false;

        if (mWstContext)
        {
            t0 = RdkWindowManager::seconds();
            error = !loadExtensions(mWstContext, clientName);
            Logger::log(LogLevel::Information, "[createDisplay] loadExtensions took %.3f ms, error:%d", (RdkWindowManager::seconds() - t0) * 1000.0, error);

            t0 = RdkWindowManager::seconds();
            if (!error && !WstCompositorSetIsEmbedded(mWstContext, true))
            {
                error = true;
            }
            Logger::log(LogLevel::Information, "[createDisplay] WstCompositorSetIsEmbedded took %.3f ms, error:%d", (RdkWindowManager::seconds() - t0) * 1000.0, error);

            t0 = RdkWindowManager::seconds();
            if (!error && !WstCompositorSetOutputSize(mWstContext, mWidth, mHeight))
            {
                error = true;
            }
            Logger::log(LogLevel::Information, "[createDisplay] WstCompositorSetOutputSize(%d,%d) took %.3f ms, error:%d", mWidth, mHeight, (RdkWindowManager::seconds() - t0) * 1000.0, error);

            t0 = RdkWindowManager::seconds();
            if (!error && !WstCompositorSetInvalidateCallback( mWstContext, invalidate, this))
            {
                error = true;
            }
            Logger::log(LogLevel::Information, "[createDisplay] WstCompositorSetInvalidateCallback took %.3f ms, error:%d", (RdkWindowManager::seconds() - t0) * 1000.0, error);

            t0 = RdkWindowManager::seconds();
            if (!error && !WstCompositorSetClientStatusCallback( mWstContext, clientStatus, this))
            {
                error = true;
            }
            Logger::log(LogLevel::Information, "[createDisplay] WstCompositorSetClientStatusCallback took %.3f ms, error:%d", (RdkWindowManager::seconds() - t0) * 1000.0, error);

            t0 = RdkWindowManager::seconds();
            if (!error && !WstCompositorSetDispatchCallback( mWstContext, dispatch, this))
            {
                error = true;
            }
            Logger::log(LogLevel::Information, "[createDisplay] WstCompositorSetDispatchCallback took %.3f ms, error:%d", (RdkWindowManager::seconds() - t0) * 1000.0, error);

            if (!error)
            {
                if (!displayName.empty())
                {
                    t0 = RdkWindowManager::seconds();
                    if (!WstCompositorSetDisplayName( mWstContext, displayName.c_str()))
                    {
                        error = true;
                    }
                    Logger::log(LogLevel::Information, "[createDisplay] WstCompositorSetDisplayName(%s) took %.3f ms, error:%d", displayName.c_str(), (RdkWindowManager::seconds() - t0) * 1000.0, error);
                }
                if (mDisplayName.empty())
                {
                    t0 = RdkWindowManager::seconds();
                    mDisplayName = WstCompositorGetDisplayName(mWstContext);
                    Logger::log(LogLevel::Information, "[createDisplay] WstCompositorGetDisplayName took %.3f ms, name:%s", (RdkWindowManager::seconds() - t0) * 1000.0, mDisplayName.c_str());
                }
                Logger::log(LogLevel::Information,  "The display name is: %s", mDisplayName.c_str());

                /* Load Westeros extensions for WM firebolt interfaces */
                t0 = RdkWindowManager::seconds();
                loadfireboltExtensions(mWstContext);
                Logger::log(LogLevel::Information, "[createDisplay] loadfireboltExtensions took %.3f ms", (RdkWindowManager::seconds() - t0) * 1000.0);

                t0 = RdkWindowManager::seconds();
                if (!error && !WstCompositorStart(mWstContext))
                {
                    error= true;
                }
                Logger::log(LogLevel::Information, "[createDisplay] WstCompositorStart took %.3f ms, error:%d", (RdkWindowManager::seconds() - t0) * 1000.0, error);

                if (!mApplicationName.empty())
                {
                    Logger::log(LogLevel::Information,  "RDKWindowManager is launching %s", mApplicationName.c_str());
                    t0 = RdkWindowManager::seconds();
                    launchApplicationInBackground();
                    Logger::log(LogLevel::Information, "[createDisplay] launchApplicationInBackground took %.3f ms", (RdkWindowManager::seconds() - t0) * 1000.0);
                }
            }
        }

        t0 = RdkWindowManager::seconds();
        enableVirtualDisplay(virtualDisplayEnabled);
        Logger::log(LogLevel::Information, "[createDisplay] enableVirtualDisplay(%d) took %.3f ms", virtualDisplayEnabled, (RdkWindowManager::seconds() - t0) * 1000.0);

        t0 = RdkWindowManager::seconds();
        setVirtualResolution(virtualWidth, virtualHeight);
        Logger::log(LogLevel::Information, "[createDisplay] setVirtualResolution(%d,%d) took %.3f ms", virtualWidth, virtualHeight, (RdkWindowManager::seconds() - t0) * 1000.0);

        t0 = RdkWindowManager::seconds();
        if (!setOwner(ownerId, groupId))
        {
            Logger::log(LogLevel::Error,  "error setting the ownerID: %d", ownerId);
            error= true;
        }
        Logger::log(LogLevel::Information, "[createDisplay] setOwner(%d,%d) took %.3f ms, error:%d", ownerId, groupId, (RdkWindowManager::seconds() - t0) * 1000.0, error);

        if (error)
        {
            const char *detail= WstCompositorGetLastErrorDetail( mWstContext );
            Logger::log(LogLevel::Information,  "error setting up the compositor: %s", detail);
            Logger::log(LogLevel::Information, "[createDisplay] TOTAL time: %.3f ms (FAILED)", (RdkWindowManager::seconds() - tFunc) * 1000.0);
            return false;
        }
        Logger::log(LogLevel::Information, "[createDisplay] TOTAL time: %.3f ms (SUCCESS)", (RdkWindowManager::seconds() - tFunc) * 1000.0);
        return true;
    }
}

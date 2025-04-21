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
        if (width > 0 && height > 0)
        {
            mWidth = width;
            mHeight = height;
        }
        mWstContext = WstCompositorCreate();

        bool error = false;

        if (mWstContext)
        {
            error = !loadExtensions(mWstContext, clientName);

            if (!error && !WstCompositorSetIsEmbedded(mWstContext, true))
            {
                error = true;
            }

            if (!error && !WstCompositorSetOutputSize(mWstContext, mWidth, mHeight))
            {
                error = true;
            }

            if (!error && !WstCompositorSetInvalidateCallback( mWstContext, invalidate, this))
            {
                error = true;
            }

            if (!error && !WstCompositorSetClientStatusCallback( mWstContext, clientStatus, this))
            {
                error = true;
            }

            if (!error && !WstCompositorSetDispatchCallback( mWstContext, dispatch, this))
            {
                error = true;
            }

            if (!error)
            {
                if (!displayName.empty())
                {
                    if (!WstCompositorSetDisplayName( mWstContext, displayName.c_str()))
                    {
                        error = true;
                    }
                }
                if (mDisplayName.empty())
                {
                    mDisplayName = WstCompositorGetDisplayName(mWstContext);
                }
                Logger::log(LogLevel::Information,  "The display name is: %s", mDisplayName.c_str());
                
                /* Load Westeros extensions for WM firebolt interfaces */
                loadfireboltExtensions(mWstContext);           

                if (!error && !WstCompositorStart(mWstContext))
                {
                    error= true;
                }

                if (!mApplicationName.empty())
                {
                    Logger::log(LogLevel::Information,  "RDKWindowManager is launching %s", mApplicationName.c_str());
                    launchApplicationInBackground();
                }
            }
        }

        enableVirtualDisplay(virtualDisplayEnabled);
        setVirtualResolution(virtualWidth, virtualHeight);

        if ((!displayName.empty()) && (0 < ownerId))
        {
            const char* runtimeDir = getenv("XDG_RUNTIME_DIR");

            if(NULL != runtimeDir)
            {
                std::string displaySocket = std::string(runtimeDir) + "/" + displayName;

                Logger::log(LogLevel::Information,"change owner of %s for ownerId : %d", displaySocket.c_str(), ownerId);
                if(0 != chown(displaySocket.c_str(), ownerId, (groupId>0)?groupId:0))
                {
                    Logger::log(LogLevel::Error,"failed to change ownership for ownerId : %d", ownerId);
                    error= true;
                }
            }
            else
            {
                Logger::log(LogLevel::Error,"failed to get runtime directory");
                error= true;
            }
        }

        if (error)
        {
            const char *detail= WstCompositorGetLastErrorDetail( mWstContext );
            Logger::log(LogLevel::Information,  "error setting up the compositor: %s", detail);
            return false;
        }
        return true;
    }
}

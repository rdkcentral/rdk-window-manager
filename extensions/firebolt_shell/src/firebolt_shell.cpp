/**
* If not stated otherwise in this file or this component's LICENSE
* file the following copyright and licenses apply:
*
* Copyright 2020 RDK Management
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

#include "logger.h"
#include "firebolt_shell.h"

static void firebolt_shell_get_firebolt_surface(struct wl_client *client,
                            struct wl_resource *resource,
                            uint32_t id,
                            struct wl_resource *surface,
                            firebolt_shell_surface_type type);

static const struct firebolt_shell_interface fireboltshellinterface_Impl = {
    firebolt_shell_get_firebolt_surface
};

static void firebolt_shell_get_firebolt_surface(struct wl_client *client,
                                    struct wl_resource *resource,
                                    uint32_t id,
                                    struct wl_resource *surface,
                                    firebolt_shell_surface_type type);

void fireboltShell::fireboltShell()
{
   RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information, "FireboltShell constructor"); 
}

void fireboltShell::~fireboltShell()
{
    RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information, "FireboltShell Desstructor");
}

void fireboltShellBind( struct wl_client *client, void *data, uint32_t version, uint32_t id)
{
    fireboltShell *shellData = (WstContext*)data;

    RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information, "FireboltShell Bind");

    shellData->shellResource = wl_resource_create(client,&firebolt_shell_interface,std::min<int>MIN(version, 1), id);

    if (!shellData->shellResource) 
    {
        wl_client_post_no_memory(client);
    }
    else
    {
        wl_resource_set_implementation(shellData->shellResource, &fireboltshellinterface_Impl, shellData->ctx, NULL);
    }

    return;
}


static void firebolt_shell_get_firebolt_surface(struct wl_client *client,
                                    struct wl_resource *resource,
                                    uint32_t id,
                                    struct wl_resource *surface,
                                    firebolt_shell_surface_type type)
{
    std::string hdwre_video_surface_id = "1111"; /*dummy input for now */
    RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information, "fireboltShell Interface getFireboltSurface");
    type = FIREBOLT_SHELL_SURFACE_TYPE_VIDEO ; /* dummy input for now */

    if( type == FIREBOLT_SHELL_SURFACE_TYPE_VIDEO )
    {
        firebolt_shell_send_firebolt_video_surface_id(resource,hdwre_video_surface_id);
        RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information, "fireboltShell video id %s",hdwre_video_surface_id);
    }
    RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information, "fireboltShell getFireboltSurface End videoId %s",hdwre_video_surface_id);
}


bool fireboltShell::initialise()
{
    bool status = true;
    RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information, "initialiseFireboltShell called for firebolt shell module");

    /*Need to connect with the actual window manager display*/
    ctx.display = wl_display_create();

    /* register our firebolt_shell interface with wayland */
    shellGlobal = wl_global_create(display, &firebolt_shell_interface,
                                               1, this, fireboltShellBind);
    if (!shellGlobal)
    {
        RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information, "Error: failed to register firebolt_interface shell interface");
        status = false;
    }
    return status;
}

bool fireboltShell::destroy(void)
{
    RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information, "destroyFireboltShell called for firebolt shell module");
    return true;
}

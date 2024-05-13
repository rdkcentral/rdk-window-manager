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

#include "logger.h"
#include "firebolt_shell.h"
#include "firebolt_shell_protocol_server.h"

typedef struct fireboltShellContext
{
    struct wl_display *display;
    struct wl_surface *firebolt_surface;
    struct wl_resource *shellResource;
    struct wl_global *shellGlobal;
}fireboltShellCtx;

static fireboltShellCtx *ctx;
static bool fireboltshell_initialised = false;

static void firebolt_shell_get_firebolt_surface(struct wl_client *client,
                            struct wl_resource *resource,
                            uint32_t id,
                            struct wl_resource *surface,
                            uint32_t type);

static const struct firebolt_shell_interface fireboltshellinterface_Impl = {
    firebolt_shell_get_firebolt_surface
};

void fireboltShellBind( struct wl_client *client, void *data, uint32_t version, uint32_t id)
{
    fireboltShellCtx *shellData = (fireboltShellCtx*)data;

    RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information, "FireboltShell Bind");

    shellData->shellResource = wl_resource_create(client,&firebolt_shell_interface,std::min<int>(version, 1), id);

    if (!shellData->shellResource) 
    {
        wl_client_post_no_memory(client);
    }
    else
    {
        wl_resource_set_implementation(shellData->shellResource, &fireboltshellinterface_Impl, shellData, NULL);
    }

    return;
}


static void firebolt_shell_get_firebolt_surface(struct wl_client *client,
                                    struct wl_resource *resource,
                                    uint32_t id,
                                    struct wl_resource *surface,
                                    uint32_t type)
{
    const char * hdwre_video_surface_id = "1111"; /*dummy input for now */
    RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information, "fireboltShell Interface getFireboltSurface");
    type = FIREBOLT_SHELL_FIREBOLT_SURFACE_TYPE_VIDEO ; /* dummy input for now */

    if( type == FIREBOLT_SHELL_FIREBOLT_SURFACE_TYPE_VIDEO )
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

    /* If firebolt shell not initialised then only go ahead with initialisation */
    if( fireboltshell_initialised ==  false )
    {
        ctx = (fireboltShellCtx*)calloc( 1, sizeof(fireboltShellCtx));

        /* Connecting to the window manager display */
        ctx->display = wl_display_create();

        /* register our firebolt_shell interface with wayland */
        ctx->shellGlobal = wl_global_create(ctx->display , &firebolt_shell_interface,1, ctx, fireboltShellBind);
        if (!ctx->shellGlobal)
        {
            RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information, "Error: failed to register firebolt_interface shell interface");
            status = false;
        }
        else
        {
            fireboltshell_initialised = true;
        }
    }
    return status;
}

bool fireboltShell::destroy(void)
{
    if (ctx->shellResource)
    {
        wl_resource_destroy(ctx->shellResource);
        ctx->shellResource = 0;
    }
    
    if ( ctx->display )
    {
        wl_display_destroy(ctx->display);
        ctx->display= 0;
    }
    
    fireboltshell_initialised = false;
    RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information, "destroyFireboltShell called for firebolt shell module");
    return true;
}

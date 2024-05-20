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
#include <cstring>
#include "westeros-compositor.h"
#include "firebolt_shell.h"
#include "firebolt_shell_protocol_server.h"

struct fireboltShellCtx
{
    WstCompositor       *wstComp;
    struct wl_display   *wlDisplay;
    struct wl_surface   *wlSurface;
    wl_resource         *wlResource;
    wl_global           *wlGlobal;
};

static fireboltShellCtx   *f_fbShellCtx = NULL;
static bool                 bfbShellInitialized = false;

static void firebolt_shell_get_firebolt_surface(struct wl_client *client,
                            struct wl_resource *resource,
                            uint32_t id,
                            struct wl_resource *surface,
                            uint32_t type);

/* struct for shell interface implementation*/
static const struct firebolt_shell_interface fireboltShellInterfaceImpl = {
    firebolt_shell_get_firebolt_surface
};

/*firebolt_shell bind function*/
void firebolt_shell_bind( struct wl_client *client, void *data, uint32_t version, uint32_t id)
{
    fireboltShellCtx *f_fbShellCtx = (fireboltShellCtx*)data;

    RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information, "FireboltShell Bind");

    if (NULL == f_fbShellCtx->wlResource)
    {
        RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information, " firebolt_shell id:%d wl_resource_create", id);
        f_fbShellCtx->wlResource= wl_resource_create(client,
                                                        &firebolt_shell_interface,
                                                        std::min<int>(version, 1), id);
        if (!f_fbShellCtx->wlResource)
        {
            RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Error, " firebolt_shell id:%d wl_resource_create - no memory", id);
            wl_client_post_no_memory(client);
        }
        else
        {
            RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information, " firebolt_shell id:%d wl_resource_set_implementation", id);
            wl_resource_set_implementation(f_fbShellCtx->wlResource, &fireboltShellInterfaceImpl, f_fbShellCtx, NULL);
        }
    }
    else
    {
        RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information, " firebolt_shell_bind id:%d already bound!", id);
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

extern "C"
{
    bool moduleInit(WstCompositor *wstComp, struct wl_display *display)
    {
        bool ret = true;

        RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
                                        " moduleInit: firebolt_shell extension wstComp@%p wlDisplay@%p initializing",
                                        wstComp, display);
        if (!bfbShellInitialized)
        {
            if (NULL == f_fbShellCtx)
            {
                f_fbShellCtx = (fireboltShellCtx *)malloc(sizeof(fireboltShellCtx));
                if (NULL == f_fbShellCtx)
                {
                    RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Error,
                                        " moduleInit: Failed to create memory for fireboltShellCtx");
                    ret = false;
                }
                else
                {
                    memset(f_fbShellCtx, 0, sizeof(fireboltShellCtx));
                    f_fbShellCtx->wstComp = wstComp;
                    f_fbShellCtx->wlDisplay = display;
                    f_fbShellCtx->wlGlobal = wl_global_create(display,
                                                                &firebolt_shell_interface,
                                                                1, f_fbShellCtx,
                                                                firebolt_shell_bind);
                    if (NULL == f_fbShellCtx->wlGlobal)
                    {
                        RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Error,
                                        " moduleInit: Failed to wl_global_create interface:firebolt_shell");
                        ret = false;
                    }
                    else
                    {
                        RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
                                        " moduleInit: firebolt_shell extension wstComp@%p wlDisplay@%p wlGlobal@%p initialized",
                                        f_fbShellCtx->wstComp,
                                        f_fbShellCtx->wlDisplay,
                                        f_fbShellCtx->wlGlobal);
                    }
                    bfbShellInitialized = true;
                }
            }
        }
        else
        {
            RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
                                        " moduleInit: firebolt_shell extension already initialized");
        }

        return ret;
    }

    void moduleTerm(WstCompositor *wstComp)
    {
        RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information, " moduleTerm: firebolt_shell extension dummy");
        bfbShellInitialized = false;
    }
}


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
 */

#include <cstring>
#include "logger.h"
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

/* vtable of firebolt_shell interface implementation */
static const struct firebolt_shell_interface fireboltShellInterfaceImpl = {
                        .get_firebolt_surface = firebolt_shell_get_firebolt_surface
                    };


/**
 * create a firebolt shell surface from a given wayland surface
 *
 * Create a firebolt_surface wrapper around wl_surfaces, sent in reply
 * to a get_firebolt_surface request for video surfaces
 *
 * @param id        : id of the firebolt_surface interface
 * @param surface   : Object of the wayland surface to be converted as firebolt_surface
 * @param type      : firebolt_shell_firebolt_surface_type FIREBOLT_SHELL_FIREBOLT_SURFACE_TYPE_*
 */
static void firebolt_shell_get_firebolt_surface(struct wl_client *client,
                                    struct wl_resource *resource,
                                    uint32_t id,
                                    struct wl_surface *surface,
                                    uint32_t type)
{
    const char *hwVideoSurfaceId = "1"; /* hardcorded for testing purpose */

    RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
                    " firebolt_shell@.get_firebolt_surface: client@%p resource@%p id:%u surface:%p type:%u",
                    client, resource, resource, id, surface, type);

    /* TODO: To be implemented */

    /* Notifying event for video surfaces */
    if (type == FIREBOLT_SHELL_FIREBOLT_SURFACE_TYPE_VIDEO)
    {
        RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Warn,
                        " firebolt_shell@.event: firebolt_video_surface_id %s", hwVideoSurfaceId);

        /* Sending hardcoded hw firebolt_video_surface_id for testing purpose */
        firebolt_shell_send_firebolt_video_surface_id(resource, hwVideoSurfaceId);
    }
    return;
}

/**
 * firebolt_shell interfaces bind operation with wayland extension
 * for the given interface version and id
 *
 * @param client  : wayland client object of the firebolt_shell interface
 * @param data    : user defined data object of the firebolt_shell interface
 * @param version : version of the firebolt_shell interface
 * @param id      : id of the firebolt_shell interface
 */
void firebolt_shell_bind(struct wl_client *client, void *data, uint32_t version, uint32_t id)
{
    fireboltShellCtx *f_fbShellCtx = reinterpret_cast<fireboltShellCtx*>(data);
    RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
                            " firebolt_shell@.bind: client@%p data:%p version:%u, id:%u",
                            client, data, version, id);

    if (NULL == f_fbShellCtx)
    {
        RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Error,
                        " firebolt_shell interface instance not valid");
        goto ret_fail;
    }

    if (NULL == f_fbShellCtx->wlResource)
    {
        RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
                        " firebolt_shell id:%d wl_resource_create", id);
        f_fbShellCtx->wlResource = wl_resource_create(client,
                                                    &firebolt_shell_interface,
                                                    std::min<int>(version, 1), id);
        if (!f_fbShellCtx->wlResource)
        {
            RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Error,
                            " firebolt_shell id:%d wl_resource_create - no memory", id);
            wl_client_post_no_memory(client);
            goto ret_fail;
        }
        else
        {
            RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
                            " firebolt_shell id:%d wl_resource_set_implementation", id);
            wl_resource_set_implementation(f_fbShellCtx->wlResource,
                                            &fireboltShellInterfaceImpl,
                                            f_fbShellCtx,
                                            NULL);
        }
    }
    else
    {
        RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Warn,
                                " firebolt_shell@.bind: interface version:%u, id:%u already bound!");
    }

ret_fail:
    return;
}

extern "C"
{
    /**
     * moduleInit of firebolt_shell westeros extension plugin
     *
     * @param wstComp   : Object of westeros compositor instance
     * @param display   : Object of the wayland display
     */
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
                                        " moduleInit: firebolt_shell no memory for context object");
                    ret = false;
                    goto ret_fail;
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
                        goto ret_fail;
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

    ret_fail:
        return ret;
    }

    /**
     * moduleTerm of firebolt_shell westeros extension plugin
     *
     * @param wstComp : Object of westeros compositor instance
     */
    void moduleTerm(WstCompositor *wstComp)
    {
        RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
                    " moduleTerm: firebolt_shell extension dummy!");

        /* TODO: To be implemented */

        bfbShellInitialized = false;
    }
}


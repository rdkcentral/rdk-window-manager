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

#include <cstring>
#include "logger.h"
#include "westeros-compositor.h"
#include "firebolt_surface.h"
#include "firebolt_surface_protocol_server.h"

struct fireboltSurfaceCtx
{
    WstCompositor       *wstComp;
    struct wl_display   *wlDisplay;
    struct wl_surface   *wlSurface;
    wl_resource         *wlResource;
    wl_global           *wlGlobal;
};

static fireboltSurfaceCtx   *f_fbSurfaceCtx = NULL;
static bool                 bfbSurfaceInitialized = false;

static void firebolt_surface_destroy(struct wl_client *client, struct wl_resource *resource);
static void firebolt_surface_setName(struct wl_client *client, struct wl_resource *resource, const char *name);
static void firebolt_surface_setVisible(struct wl_client *client, struct wl_resource *resource, uint32_t visible);
static void firebolt_surface_setBounds(struct wl_client *client, struct wl_resource *resource,
                                        int32_t x, int32_t y, int32_t width, int32_t height);
static void firebolt_surface_setCrop(struct wl_client *client, struct wl_resource *resource,
                                        wl_fixed_t sx, wl_fixed_t sy, wl_fixed_t swidth, wl_fixed_t  sheight);
static void firebolt_surface_setZOrder(struct wl_client *client, struct wl_resource *resource, wl_fixed_t zorder);
static void firebolt_surface_setOpacity(struct wl_client *client, struct wl_resource *resource, wl_fixed_t opacity);

/* vtable of firebot_surface interfaces implementation */
static const struct firebolt_surface_interface fireboltSurfaceInterfaceImpl = {
                        .destroy        = firebolt_surface_destroy,
                        .set_name       = firebolt_surface_setName,
                        .set_visible    = firebolt_surface_setVisible,
                        .set_bounds     = firebolt_surface_setBounds,
                        .set_crop       = firebolt_surface_setCrop,
                        .set_zorder     = firebolt_surface_setZOrder,
                        .set_opacity    = firebolt_surface_setOpacity
                    };

/**
 * destroy the firebolt_surface
 *
 * Destroy the firebolt_surface object. This removes the
 * association with the underlying wl_surface or hardware video
 * surface and removes the surface from the composition.
 */
static void firebolt_surface_destroy(struct wl_client *client, struct wl_resource *resource)
{
    RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
                    " firebolt_surface@.destroy client@%p resource@%p",
                    client, resource);

    /* TODO: To be implemented */
    return;
}

/**
 * set the name of the firebolt_surface
 *
 * Sets the name of the firebolt surface
 *
 */
static void firebolt_surface_setName(struct wl_client *client, struct wl_resource *resource,
                                    const char *name)
{
    RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
                    " firebolt_surface@.set_name client@%p resource@%p name:%s",
                    client, resource, name);

    /* TODO: To be implemented */
    return;
}


/*
 * set the visibility of the surface
 * Setting to 0 makes the surface not visible.
 *
 * @param visible : type usigned int of enum firebolt_surface_visibility
 */
static void firebolt_surface_setVisible(struct wl_client *client, struct wl_resource *resource,
                                        uint32_t visible)
{
    RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
                    " firebolt_surface@.set_visible: client@%p resource@%p visible:%u",
                    client, resource, visible);

    /* TODO: To be implemented */
    return;
}

/**
 * set the surface bounds
 *
 * The surface bounds of a surface is its composited pixel
 * position and dimensions. If unset then the surface is set to the
 * top left, with width and height that match the underlying
 * surface dimensions.
 *
 * The width and height of the effective surface bounds must be
 * greater than zero. Setting an invalid size will raise an invalid_size error.
 *
 * @param x         : the left position of the surface
 * @param y         : the top position of the surface
 * @param width     : the width of the surface
 * @param height    : the height of the surface
 */
static void firebolt_surface_setBounds(struct wl_client *client,
                                        struct wl_resource *resource, int32_t x, int32_t y, int32_t width, int32_t height)
{
    RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
                    " firebolt_surface@.set_bounds: client@%p resource@%p x:%d y:%d width:%d height:%d",
                    client, resource, x, y, width, height);

    /* TODO: To be implemented */
    return;
}

/**
 * set the cropping of the surface within the surface
 *
 * Sets the cropping of a given surface.
 *
 * Cropping works on an arbitrary scale, not on pixels. The input
 * surface is defined as a rectangle with width and height of 1.0
 * and therefore all cropping values should be fixed point values
 * between 0.0 and 1.0 inclusive.
 *
 * For example to crop the top right quarter of the video then set
 * (x, y, width, height) to (0.5, 0.0, 0.5, 0.5).
 *
 * @param sx        : the left position of the surface, fixed point number
 * @param sy        : the top position of the surface, fixed point number
 * @param swidth    : the width of the surface, fixed point number
 * @param sheight   : the height of the surface, fixed point number representing
 */
static void firebolt_surface_setCrop(struct wl_client *client, struct wl_resource *resource,
                                        wl_fixed_t sx, wl_fixed_t sy, wl_fixed_t swidth, wl_fixed_t sheight)
{
    RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
                    " firebolt_surface@.set_crop: client@%p resource@%p x:%f y:%f width:%f height:%f",
                    client, resource, wl_fixed_to_double(sx), wl_fixed_to_double(sy),
                    wl_fixed_to_double(swidth), wl_fixed_to_double(sheight));

    /* TODO: To be implemented */
    return;
}

/**
 * set the relative z-order of the surface
 *
 * Sets the z-order of the surface relative to other surfaces
 * within the clients display.
 *
 * The z-order should be in the range of 0.0 - 1.0 inclusive.
 *
 * @param zorder : z-order of the surface relative to other surface
 */
static void firebolt_surface_setZOrder(struct wl_client *client, struct wl_resource *resource, wl_fixed_t zorder)
{
    RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
                    " firebolt_surface@.set_zorder: client@%p resource@%p zorder:%f",
                    client, resource, wl_fixed_to_double(zorder));

    /* TODO: To be implemented */
    return;
}

/**
 * set surface opacity
 *
 * Sets the opacity of the surface. It may not be possible to set
 * the opacity on hardware video surfaces.
 *
 * The opacity should be in the range of 0.0 - 1.0 inclusive.
 *
 * @param opacity : opacity value of the surface
 */
static void firebolt_surface_setOpacity(struct wl_client *client, struct wl_resource *resource, wl_fixed_t opacity)
{
    RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
                    " firebolt_surface@.set_opacity: client@%p resource@%p opacity :%f",
                    client, resource, wl_fixed_to_double(opacity));
    /* TODO: To be implemented */
    return;
}

/**
 * firebolt_surface interfaces bind operation with wayland extension
 * for the given interface version and id
 *
 * @param client  : wayland client object of the firebolt_surface interface
 * @param data    : user defined data object of the firebolt_surface interface
 * @param version : version of the firebolt_surface interface
 * @param id      : id of the firebolt_surface interface
 */
void firebolt_surface_bind(struct wl_client *client, void *data, uint32_t version, uint32_t id)
{
    fireboltSurfaceCtx *f_fbSurfaceCtx = reinterpret_cast<fireboltSurfaceCtx*>(data);
    RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
                            " firebolt_surface@.bind: client@%p data:%p version:%u, id:%u",
                            client, data, version, id);

    if (NULL == f_fbSurfaceCtx)
    {
        RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Error,
                        " firebolt_surface interface instance not valid");
        goto ret_fail;
    }

    if (NULL == f_fbSurfaceCtx->wlResource)
    {
        RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
                        " firebolt_surface id:%d wl_resource_create", id);
        f_fbSurfaceCtx->wlResource = wl_resource_create(client,
                                                        &firebolt_surface_interface,
                                                        std::min<int>(version, 1), id);
        if (!f_fbSurfaceCtx->wlResource)
        {
            RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Error,
                            " firebolt_surface id:%d wl_resource_create - no memory", id);
            wl_client_post_no_memory(client);
            goto ret_fail;
        }
        else
        {
            RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
                            " firebolt_surface id:%d wl_resource_set_implementation", id);
            wl_resource_set_implementation(f_fbSurfaceCtx->wlResource,
                                            &fireboltSurfaceInterfaceImpl,
                                            f_fbSurfaceCtx,
                                            NULL);
        }
    }
    else
    {
        RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Warn,
                                " firebolt_surface@.bind: interface version:%u, id:%u already bound!");
    }

ret_fail:
    return;
}

extern "C"
{
    /**
     * moduleInit of firebolt_surface westeros extension plugin
     *
     * @param wstComp   : Object of westeros compositor instance
     * @param display   : Object of the wayland display
     */
    bool moduleInit(WstCompositor *wstComp, struct wl_display *display)
    {
        bool ret = true;
        RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
                                        " moduleInit: firebolt_surface extension wstComp@%p wlDisplay@%p initializing",
                                        wstComp, display);
        if (!bfbSurfaceInitialized)
        {
            if (NULL == f_fbSurfaceCtx)
            {
                f_fbSurfaceCtx = (fireboltSurfaceCtx *)malloc(sizeof(fireboltSurfaceCtx));
                if (NULL == f_fbSurfaceCtx)
                {
                    RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Error,
                                        " moduleInit: firebolt_surface no memory for context object");
                    ret = false;
                    goto ret_fail;
                }
                else
                {
                    memset(f_fbSurfaceCtx, 0, sizeof(fireboltSurfaceCtx));
                    f_fbSurfaceCtx->wstComp = wstComp;
                    f_fbSurfaceCtx->wlDisplay = display;
                    f_fbSurfaceCtx->wlGlobal = wl_global_create(display,
                                                                &firebolt_surface_interface,
                                                                1, f_fbSurfaceCtx,
                                                                firebolt_surface_bind);
                    if (NULL == f_fbSurfaceCtx->wlGlobal)
                    {
                        RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Error,
                                        " moduleInit: Failed to wl_global_create interface:firebolt_surface");
                        ret = false;
                        goto ret_fail;
                    }
                    else
                    {
                        RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
                                        " moduleInit: firebolt_surface extension wstComp@%p wlDisplay@%p wlGlobal@%p initialized",
                                        f_fbSurfaceCtx->wstComp,
                                        f_fbSurfaceCtx->wlDisplay,
                                        f_fbSurfaceCtx->wlGlobal);
                    }
                    bfbSurfaceInitialized = true;
                }
            }
        }
        else
        {
            RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
                                        " moduleInit: firebolt_surface extension already initialized");
        }

    ret_fail:
        return ret;
    }

    /**
     * moduleTerm of firebolt_surface westeros extension plugin
     *
     * @param wstComp : Object of westeros compositor instance
     */
    void moduleTerm(WstCompositor *wstComp)
    {
        RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
                        " moduleTerm: firebolt_surface extension dummy");

        /* TODO: To be implemented */

        bfbSurfaceInitialized = false;
    }
}


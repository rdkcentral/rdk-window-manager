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
#include "firebolt_wm.h"
#include "firebolt_wm_protocol_server.h"

struct fireboltWmContext
{
    WstCompositor       *wstComp;
    struct wl_display   *wlDisplay;
    struct wl_surface   *wlSurface;
    wl_resource         *wlResource;
    wl_global           *wlGlobal;
};

static fireboltWmContext    *f_fbWmCtx = NULL;
static bool                 bfbWMInitialized = false;

static void firebolt_wm_set_properties(struct wl_client *client,
                                        struct wl_resource *resource,
                                        const char *id,
                                        int32_t x,
                                        int32_t y,
                                        uint32_t width,
                                        uint32_t height,
                                        uint32_t render_width,
                                        uint32_t render_height,
                                        wl_fixed_t opacity,
                                        int32_t zorder,
                                        int32_t visible,
                                        wl_fixed_t crop_x,
                                        wl_fixed_t crop_y,
                                        wl_fixed_t crop_width,
                                        wl_fixed_t crop_height);
static void firebolt_wm_create(struct wl_client *client, struct wl_resource *resource, const char *id);
static void firebolt_wm_create_with_bounds(struct wl_client *client,
                                        struct wl_resource *resource,
                                        const char *id,
                                        int32_t x,
                                        int32_t y,
                                        uint32_t width,
                                        uint32_t height);
static void firebolt_wm_create_with_properties(struct wl_client *client,
                                        struct wl_resource *resource,
                                        const char *id,
                                        int32_t x,
                                        int32_t y,
                                        uint32_t width,
                                        uint32_t height,
                                        uint32_t display_width,
                                        uint32_t display_height,
                                        wl_fixed_t opacity,
                                        int32_t zorder,
                                        int32_t visible,
                                        wl_fixed_t crop_x,
                                        wl_fixed_t crop_y,
                                        wl_fixed_t crop_width,
                                        wl_fixed_t crop_height,
                                        int32_t focused);
static void firebolt_wm_destroy(struct wl_client *client, struct wl_resource *resource, const char *id);
static void firebolt_wm_set_client_bounds(struct wl_client *client,
                                        struct wl_resource *resource,
                                        const char *id,
                                        int32_t x,
                                        int32_t y,
                                        uint32_t width,
                                        uint32_t height);
static void firebolt_wm_set_client_display_bounds(struct wl_client *client,
                                         struct wl_resource *resource,
                                         const char *id,
                                         uint32_t width,
                                         uint32_t height);
static void firebolt_wm_set_client_focus(struct wl_client *client, struct wl_resource *resource, const char *id);
static void firebolt_wm_get_properties(struct wl_client *client, struct wl_resource *resource, const char *id);
static void firebolt_wm_get_focused_client(struct wl_client *client, struct wl_resource *resource);
static void firebolt_wm_get_clients(struct wl_client *client, struct wl_resource *resource);

/* vtable of firebolt_wm interfaces implementation */
static const struct firebolt_wm_interface fireboltWindowManagerImpl = {
                                        .set_properties             = firebolt_wm_set_properties,
                                        .create                     = firebolt_wm_create,
                                        .create_with_bounds         = firebolt_wm_create_with_bounds,
                                        .create_with_properties     = firebolt_wm_create_with_properties,
                                        .destroy                    = firebolt_wm_destroy,
                                        .set_client_bounds          = firebolt_wm_set_client_bounds,
                                        .set_client_display_bounds  = firebolt_wm_set_client_display_bounds,
                                        .set_client_focus           = firebolt_wm_set_client_focus,
                                        .get_properties             = firebolt_wm_get_properties,
                                        .get_focused_client         = firebolt_wm_get_focused_client,
                                        .get_clients                = firebolt_wm_get_clients
                                    };

/**
 * Set the properties of the firebolt window manager 
 *
 * @param id            : id of the app or group
 * @param x             : the left position of the surface in pixel screen coordinates
 * @param y             : the top position of the surface in pixel screen coordinates
 * @param width         : the width of the graphics surface in pixel screen coordinates
 * @param height        : the height of the graphics surface in pixel screen coordinates
 * @param render_width  : the width of the graphics rendering
 * @param render_height : the height of the graphics surface in pixel screen coordinates
 * @param opacity       : opacity factor
 * @param zorder        : location in the z-order
 * @param visible       : the visibility of the surface
 * @param crop_x        : the cropping left insert (before scale?)
 * @param crop_y        : the cropping top insert (before scale?)
 * @param crop_width    : the cropping width (before scale?)
 * @param crop_height   : the cropping height (before scale?)
 */
static void firebolt_wm_set_properties(struct wl_client *client,
                             struct wl_resource *resource,
                             const char *id,
                             int32_t x,
                             int32_t y,
                             uint32_t width,
                             uint32_t height,
                             uint32_t render_width,
                             uint32_t render_height,
                             wl_fixed_t opacity,
                             int32_t zorder,
                             int32_t visible,
                             wl_fixed_t crop_x,
                             wl_fixed_t crop_y,
                             wl_fixed_t crop_width,
                             wl_fixed_t crop_height)
{
    RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
                    "firebolt_wm@.set_properties client@%p resource@%p "
                    "id:%s surface{x:%d y:%d width:%u height:%u} graphics render{width:%u height:%u} "
                    "opacity:%f zorder:%u visible:%u crop{x:%f y:%f width:%f height:%f}",
                    client, resource, id, x, y, width, height, render_width, render_height,
                    wl_fixed_to_double(opacity), zorder, visible, wl_fixed_to_double(crop_x),
                    wl_fixed_to_double(crop_y), wl_fixed_to_double(crop_width),
                    wl_fixed_to_double(crop_height));

    /* TODO: To be implemented */
    return;
}

/**
 * set surface opacity
 *
 * Defaults: x, y = 0 Width, height, display width, display
 * height = device resolution Opacity = 1.0 Visible = false Z-order
 * = topmost + 1 Crop_x, crop_y = 0 Crop_width, crop_height = 1.0
 *
 * @param id : id of the app or group
 */
static void firebolt_wm_create (struct wl_client *client,
                             struct wl_resource *resource,
                             const char *id )
{
    RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
                    "firebolt_wm@.create client@%p resource@%p id:%s",
                    client, resource, id);

    /* TODO: To be implemented */
    return;
}

/**
 * Create window manager for the app or group with boundary.
 *
 * @param id    : id of the app or group
 * @param x     : the left position of the surface in pixel screen coordinates
 * @param y     : the top position of the surface in pixel screen coordinates
 * @param width : the width of the graphics surface in pixel screen coordinates
 * @param height: the height of the graphics surface in pixel screen coordinates
 */
static void firebolt_wm_create_with_bounds (struct wl_client *client,
                             struct wl_resource *resource,
                             const char *id,
                             int32_t x,
                             int32_t y,
                             uint32_t width,
                             uint32_t height)
{
    RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
                    "firebolt_wm@.create_with_bounds client@%p resource@%p "
                    "id:%s surface{x:%d y:%d width:%u height:%u}",
                    client, resource, id, x, y, width, height);

    /* TODO: To be implemented */
    return;
}

/**
 * Create window manager for the app or group with properties.
 *
 * @param id            : id of the app or group
 * @param x             : the left position of the surface in pixel screen coordinates
 * @param y             : the top position of the surface in pixel screen coordinates
 * @param width         : the width of the graphics surface in pixel screen coordinates
 * @param height        : the height of the graphics surface in pixel screen coordinates
 * @param display_width : the width of the Waylands display
 * @param display_height: the height of the Waylands display 
 * @param opacity       : opacity factor
 * @param zorder        : location in the z-order
 * @param visible       : the visibility of the surface
 * @param crop_x        : the cropping left insert
 * @param crop_y        : the cropping top insert
 * @param crop_width    : the cropping width
 * @param crop_height   : the cropping height
 * @param focused       : focused
 */
static void firebolt_wm_create_with_properties(struct wl_client *client,
                                    struct wl_resource *resource,
                                    const char *id,
                                    int32_t x,
                                    int32_t y,
                                    uint32_t width,
                                    uint32_t height,
                                    uint32_t display_width,
                                    uint32_t display_height,
                                    wl_fixed_t opacity,
                                    int32_t zorder,
                                    int32_t visible,
                                    wl_fixed_t crop_x,
                                    wl_fixed_t crop_y,
                                    wl_fixed_t crop_width,
                                    wl_fixed_t crop_height,
                                    int32_t focused)
{
    RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
                    "firebolt_wm@.create_with_properties client@%p resource@%p "
                    "id:%s surface{x:%d y:%d width:%u height:%u} wayland display{width:%u height:%u} "
                    "opacity:%f zorder:%u visible:%u crop{x:%f y:%f width:%f height:%f focused:%d}",
                    client, resource, id, x, y, width, height, display_width, display_height,
                    wl_fixed_to_double(opacity), zorder, visible, wl_fixed_to_double(crop_x),
                    wl_fixed_to_double(crop_y), wl_fixed_to_double(crop_width),
                    wl_fixed_to_double(crop_height), focused);

    /* TODO: To be implemented */
    return;
}

/**
 * destroy window manager for given id of the app or group.
 *
 * @param id id of the app or group
 */
static void firebolt_wm_destroy(struct wl_client *client,
                                struct wl_resource *resource,
                                const char *id)
{
    RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
                    "firebolt_wm@.destroy client@%p resource@%p id:%s",
                    client, resource, id);

    /* TODO: To be implemented */
    return;
}

/**
 * sets client window bounds
 *
 * Sets client window bounds for rendering. The client will be
 * rendered inside these bounds without a change to its Wayland
 * display size
 *
 * @param id    : Id of the app
 * @param x     : the left position of the surface in pixel screen coordinates
 * @param y     : the top position of the surface in pixel screen coordinates
 * @param width : the width of the graphics surface in pixel screen coordinates
 * @param height: the height of the graphics surface in pixel screen coordinates
 */
static void firebolt_wm_set_client_bounds(struct wl_client *client,
                                 struct wl_resource *resource,
                                 const char *id,
                                 int32_t x,
                                 int32_t y,
                                 uint32_t width,
                                 uint32_t height)
{
    RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
                    "firebolt_wm@.set_client_bounds client@%p resource@%p "
                    "id:%s surface{x:%d y:%d width:%u height:%u}",
                    client, resource, id, x, y, width, height);

    /* TODO: To be implemented */
    return;
}

/**
 * sets client display window bounds
 *
 * Sets client display window bounds. This will change the size of a
 * clients Wayland display
 *
 * @param id    : Id of the app
 * @param width : the width of an apps Wayland display
 * @param height: the height of an apps Wayland display
 */
static void firebolt_wm_set_client_display_bounds(struct wl_client *client,
                                 struct wl_resource *resource,
                                 const char *id,
                                 uint32_t width,
                                 uint32_t height)
{
    RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
                    "firebolt_wm@.set_client_display_bounds client@%p resource@%p "
                    "id:%s surface{width:%u height:%u}",
                    client, resource, id, width, height);

    /* TODO: To be implemented */
    return;
}

/**
 * sets a client to be the focused app
 *
 *
 * @param id : Id of the app to focus
 */
static void firebolt_wm_set_client_focus (struct wl_client *client,
                                 struct wl_resource *resource,
                                 const char *id)
{
    RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
                    "firebolt_wm@.set_client_focus client@%p resource@%p id:%s",
                    client, resource, id);

    /* TODO: To be implemented */
    return;
}

/**
 * get the app or client properties uing the app or group id
 *
 * @param id : id of the client or group
 */
static void firebolt_wm_get_properties (struct wl_client *client,
                                 struct wl_resource *resource,
                                 const char *id)
{
    const char *appId = "1"; /* hardcorded for testing purpose */

    RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
                    "firebolt_wm@.get_properties client@%p resource@%p id:%s",
                    client, resource, appId);

    /* TODO: To be implemented */

    /* Notifying event using hardcorded value for testing purpose */
    RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
                    "firebolt_wm@.event: send_client_properties resource@%p id:%s "
                    "id:%s surface{x:%d y:%d width:%u height:%u} display{width:%u height:%u} "
                    "opacity:%f zorder:%u visible:%u crop{x:%f y:%f width:%f height:%f focused:%d}",
                    resource, appId, 0, 0, 640, 480,
                    0, 0, 0, 0, 0, 0, 0, 0);
    firebolt_wm_send_client_properties(resource, appId, 
                                    0, 0, 640, 480,
                                    0, 0, 0, 0,
                                    0, 0, 0, 0);
    return;
}

/**
 * get the focused client details and sends an focused_client
 * event to the client owning the resource.
 *
 * @param id : id of the client or group
 */
static void firebolt_wm_get_focused_client (struct wl_client *client, struct wl_resource *resource)
{
    const char *focusedClientID = "1"; /* hardcorded for testing purpose */

    RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
                    "firebolt_wm@.get_focused_client: client@%p resource@%p",
                    client, resource);

    /* TODO: To be implemented */

    /* Notifying event using hardcorded value for testing purpose */
    RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
                    "firebolt_wm@.event: get_focused_client client@%p resource@%p focusedClientID:%s",
                    client, resource, focusedClientID);
    firebolt_wm_send_focused_client(resource, focusedClientID);

    return;
}

/**
 * get the list of client details and Sends an clients
 * event to the client owning the resource.
 *
 * @param id : id of the client or group
 */
static void firebolt_wm_get_clients (struct wl_client *client, struct wl_resource *resource)
{
    const char *focusedClientID = "1"; /* hardcorded for testing purpose */

    RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
                    "firebolt_wm@.get_clients: client@%p resource@%p",
                    client, resource);

    /* TODO: To be implemented */

    /* Notifying event using hardcorded value for testing purpose */
    RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
                    "firebolt_wm@.event: send_clients resource@%p focusedClientID:%s",
                    resource, focusedClientID);
    firebolt_wm_send_clients(resource, focusedClientID);

    return;
}

/**
 * firebolt_wm interfaces bind operation with wayland extension
 * for the given interface version and id
 *
 * @param client  : wayland client object of the firebolt_wm interface
 * @param data    : user defined data object of the firebolt_wm interface
 * @param version : version of the firebolt_wm interface
 * @param id      : id of the firebolt_wm interface
 */
void firebolt_wm_bind(struct wl_client *client, void *data, uint32_t version, uint32_t id)
{
    fireboltWmContext *f_fbWmCtx = reinterpret_cast<fireboltWmContext*>(data);
    RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
                            " firebolt_wm@.bind: client@%p data:%p version:%u, id:%u",
                            client, data, version, id);
    if (NULL == f_fbWmCtx)
    {
        RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Error,
                        " firebolt_wm interface instance not valid");
        goto ret_fail;
    }

    if (NULL == f_fbWmCtx->wlResource)
    {
        RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
                        " firebolt_wm id:%d wl_resource_create", id);
        f_fbWmCtx->wlResource = wl_resource_create(client,
                                                &firebolt_wm_interface,
                                                std::min<int>(version, 1), id);
        if (!f_fbWmCtx->wlResource)
        {
            RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Error,
                            " firebolt_wm id:%d wl_resource_create - no memory", id);
            wl_client_post_no_memory(client);
            goto ret_fail;
        }
        else
        {
            RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
                            " firebolt_wm id:%d wl_resource_set_implementation", id);
            wl_resource_set_implementation(f_fbWmCtx->wlResource,
                                            &fireboltWindowManagerImpl,
                                            f_fbWmCtx,
                                            NULL);
        }
    }

ret_fail:
    return;
}

extern "C"
{
    /**
     * moduleInit of firebolt_wm westeros extension plugin
     *
     * @param wstComp   : Object of westeros compositor instance
     * @param display   : Object of the wayland display
     */
    bool moduleInit(WstCompositor *wstComp, struct wl_display *display)
    {
        bool ret = true;
        RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
                                        " moduleInit: firebolt_wm extension wstComp@%p wlDisplay@%p initializing",
                                        wstComp, display);
        if (!bfbWMInitialized)
        {
            if (NULL == f_fbWmCtx)
            {
                f_fbWmCtx = (fireboltWmContext*)malloc(sizeof(fireboltWmContext));
                if (NULL == f_fbWmCtx)
                {
                    RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Error,
                                        " moduleInit: firebolt_wm no memory for context object");
                    ret = false;
                    goto ret_fail;
                }
                else
                {
                    memset(f_fbWmCtx, 0, sizeof(fireboltWmContext));
                    f_fbWmCtx->wstComp = wstComp;
                    f_fbWmCtx->wlDisplay = display;
                    f_fbWmCtx->wlGlobal = wl_global_create(display,
                                                           &firebolt_wm_interface,
                                                           1, f_fbWmCtx,
                                                           firebolt_wm_bind);
                    if (!f_fbWmCtx->wlGlobal)
                    {
                        RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Error,
                                        " moduleInit: Failed to wl_global_create interface:firebolt_wm");
                        ret = false;
                        goto ret_fail;
                    }
                    else
                    {
                        RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
                                        " moduleInit: firebolt_wm extension wstComp@%p wlDisplay@%p wlGlobal@%p initialized",
                                        f_fbWmCtx->wstComp,
                                        f_fbWmCtx->wlDisplay,
                                        f_fbWmCtx->wlGlobal);
                    }
                    bfbWMInitialized = true;
                }
            }
        }
        else
        {
            RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
                                        " moduleInit: firebolt_wm extension already initialized");
        }

    ret_fail:
        return ret;
    }

    /**
     * moduleTerm of firebolt_wm westeros extension plugin
     *
     * @param wstComp : Object of westeros compositor instance
     */
    void moduleTerm(WstCompositor *wstComp)
    {
        RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
                        " moduleTerm: firebolt_wm extension dummy");

        /* TODO: To be implemented */

        bfbWMInitialized = false;
    }
}


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
#include "firebolt_wm.h"
#include "firebolt_wm_protocol_server.h"

static fireboltWmContext *ctx;
static bool is_fireboltwm_init = false;

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
                             wl_fixed_t crop_height );
static void firebolt_wm_create (struct wl_client *client,
                             struct wl_resource *resource,
                             const char *id );
static void firebolt_wm_create_with_bounds (struct wl_client *client,
                             struct wl_resource *resource,
                             const char *id,
                             int32_t x,
                             int32_t y,
                             uint32_t width,
                             uint32_t height);
static void firebolt_wm_create_with_properties (struct wl_client *client,
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
static void firebolt_wm_destroy (struct wl_client *client,
                                 struct wl_resource *resource,
                                  const char *id);
static void firebolt_wm_set_client_bounds (struct wl_client *client,
                                 struct wl_resource *resource,
                                 const char *id,
                                 int32_t x,
                                 int32_t y,
                                 uint32_t width,
                                 uint32_t height);
static void firebolt_wm_set_client_display_bounds (struct wl_client *client,
                                 struct wl_resource *resource,
                                 const char *id,
                                 uint32_t width,
                                 uint32_t height);
static void firebolt_wm_set_client_focus (struct wl_client *client,
                                 struct wl_resource *resource,
                                 const char *id);
static void firebolt_wm_get_properties (struct wl_client *client,
                                 struct wl_resource *resource,
                                 const char *id);
static void firebolt_wm_get_focused_client (struct wl_client *client,
                                 struct wl_resource *resource);
static void firebolt_wm_get_clients (struct wl_client *client,
                                 struct wl_resource *resource);


static const struct firebolt_wm_interface fireboltWindowManagerImplementation = {
    firebolt_wm_set_properties,
    firebolt_wm_create,
    firebolt_wm_create_with_bounds,
    firebolt_wm_create_with_properties,
    firebolt_wm_destroy,
    firebolt_wm_set_client_bounds,
    firebolt_wm_set_client_display_bounds,
    firebolt_wm_set_client_focus,
    firebolt_wm_get_properties,
    firebolt_wm_get_focused_client,
    firebolt_wm_get_clients
};

void fireboltWMBind( struct wl_client *client, void *data, uint32_t version, uint32_t id)
{
    fireboltWmContext *ctx= (fireboltWmContext*)data;
    ctx->wmResource = wl_resource_create(client, &firebolt_wm_interface,
                                                             std::min<int>(version, 1), id);
    if (!ctx->wmResource)
    {
        wl_client_post_no_memory(client);
    }
    else
    {
        wl_resource_set_implementation(ctx->wmResource, &fireboltWindowManagerImplementation, ctx, NULL);
    }
    return;
}

bool firebolt_window_manager::initialise()
{
    /*Need to connect with the actual window manager display*/
    bool retval = true;
    if (is_fireboltwm_init == false)
   {
        ctx= (fireboltWmContext*)calloc( 1, sizeof(fireboltWmContext) );
        ctx->display = wl_display_create();
        RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information, "moduleInit called for firebolt_wm module\n");

        ctx->wmGlobal = wl_global_create(ctx->display, &firebolt_wm_interface,
                                                   1, ctx, fireboltWMBind);
        if (!ctx->wmGlobal)
        {
            RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Error,
                "Error: failed to register firebolt_wm interface\n");
            retval = false;
        }
        else
        {
            is_fireboltwm_init = true;
        }
    }
    return retval;
}

bool firebolt_window_manager::destroy(void)
{
    bool retval = true;

    if (ctx->wmResource){
       wl_resource_destroy(ctx->wmResource);
       ctx->wmResource = 0;
    }
    if ( ctx->display )
    {
      wl_display_destroy(ctx->display);
      ctx->display= 0;
    }
    is_fireboltwm_init = false;
    RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information, "moduleTerm called for firebolt_wm module\n");
    return retval;
}


/**
 * Set the properties of the window manager client app.
 *
 * @param id id of the app or group
 * @param x the left position of the surface in pixel screen coordinates
 * @param y the top position of the surface in pixel screen coordinates
 * @param width the width of the graphics surface in pixel screen coordinates
 * @param height the height of the graphics surface in pixel screen coordinates
 * @param render_width the width of the graphics rendering
 * @param render_height the height of the graphics surface in pixel screen coordinates
 * @param opacity opacity factor
 * @param zorder location in the z-order
 * @param visible the visibility of the surface
 * @param crop_x the cropping left insert (before scale?)
 * @param crop_y the cropping top insert (before scale?)
 * @param crop_width the cropping width (before scale?)
 * @param crop_height the cropping height (before scale?)
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
                             wl_fixed_t crop_height )
{
    RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information, \
        "firebolt_wm_set_properties app id:%s x:%u y:%u width %u height %u order %u visiblity %u crop %u:%u:%u:%u", \
        id,x,y,width,height, zorder,visible,crop_x,crop_y,crop_width, crop_height);
}

/**
 * Create window manager client for the app or group
 *
 * @param id id of the app or group 
 */
static void firebolt_wm_create (struct wl_client *client,
                             struct wl_resource *resource,
                             const char *id )
{
    RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information, "firebolt_wm_create app id:%s",id);
}

/**
 * Create window manager client for the app or group with boundary.
 *
 * @param id id of the app or group
 * @param x the left position of the surface in pixel screen coordinates
 * @param y the top position of the surface in pixel screen coordinates
 * @param width the width of the graphics surface in pixel screen coordinates
 * @param height the height of the graphics surface in pixel screen coordinates
 */
static void firebolt_wm_create_with_bounds (struct wl_client *client,
                             struct wl_resource *resource,
                             const char *id,
                             int32_t x,
                             int32_t y,
                             uint32_t width,
                             uint32_t height)
{
    RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information, "firebolt_wm_create_with_bounds app id:%s x:%u y: %u width %u height %u\n",id,x,y,width,height);
}

/**
 * Create window manager client for the app or group with properties.
 *
 * @param id id of the app or group
 * @param x the left position of the surface in pixel screen coordinates
 * @param y the top position of the surface in pixel screen coordinates
 * @param width the width of the graphics surface in pixel screen coordinates
 * @param height the height of the graphics surface in pixel screen coordinates
 * @param display_width the width of the Waylands display
 * @param display_height the height of the Waylands display 
 * @param opacity opacity factor
 * @param zorder location in the z-order
 * @param visible the visibility of the surface
 * @param crop_x the cropping left insert
 * @param crop_y the cropping top insert
 * @param crop_width the cropping width
 * @param crop_height the cropping height
 * @param focused focused
 */
static void firebolt_wm_create_with_properties (struct wl_client *client,
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
    RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information, \
        "firebolt_wm_create_with_properties app id:%s x:%u y:%u width %u height %u order %u visiblity %u crop %u:%u:%u:%u focused: %u", \
        id,x,y,width,height, zorder,visible,crop_x,crop_y,crop_width, crop_height,focused);
}

/**
 * Create window manager client for the app or group.
 *
 * @param id id of the app or group
 */
static void firebolt_wm_destroy (struct wl_client *client,
                                 struct wl_resource *resource,
                                  const char *id)
{
    RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information, "firebolt_wm_destroy id %s", id);
}

/**
 * sets client window bounds
 *
 * Sets client window bounds for rendering. The client will be
 * rendered inside these bounds without a change to its Wayland
 * display size
 * @param id Id of the app
 * @param x the left position of the surface in pixel screen coordinates
 * @param y the top position of the surface in pixel screen coordinates
 * @param width the width of the graphics surface in pixel screen coordinates
 * @param height the height of the graphics surface in pixel screen coordinates
 */
static void firebolt_wm_set_client_bounds (struct wl_client *client,
                                 struct wl_resource *resource,
                                 const char *id,
                                 int32_t x,
                                 int32_t y,
                                 uint32_t width,
                                 uint32_t height)
{
    RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information, \
        "firebolt_wm_set_client_bounds app id:%s x:%u y: %u width %u height %u",id,x,y,width,height);
}

/**
 * sets client window bounds
 *
 * Sets client window bounds. This will change the size of a
 * clients Wayland display
 * @param id Id of the app
 * @param width the width of an apps Wayland display
 * @param height the height of an apps Wayland display
 */
static void firebolt_wm_set_client_display_bounds (struct wl_client *client,
                                 struct wl_resource *resource,
                                 const char *id,
                                 uint32_t width,
                                 uint32_t height)
{
    RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information, \
        "firebolt_wm_set_client_display_bounds app id:%s width %u height %u",id,width,height);
}

/**
 * sets a client to be the focused app
 *
 * 
 * @param id Id of the app to focus
 */
static void firebolt_wm_set_client_focus (struct wl_client *client,
                                 struct wl_resource *resource,
                                 const char *id)
{
    RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information, " firebolt_wm_set_client_focus app id:%s ",id);
}

/**
 * get the client properties uing the app or group id
 *
 * @param id id of the client or group
 */
static void firebolt_wm_get_properties (struct wl_client *client,
                                 struct wl_resource *resource,
                                 const char *id)
{
    RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information, " firebolt_wm_get_properties app id:%s ",id);
    firebolt_wm_send_client_properties(resource, "dummy-id", 0,0,640,480, 0, 0, 0, 0, 0, 0, 0, 0);
}

/**
 * get the focused client details
 *
 * @param id id of the client or group
 */
static void firebolt_wm_get_focused_client (struct wl_client *client,
                                 struct wl_resource *resource)
{
    RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information, "firebolt_wm_get_focused_client");
    firebolt_wm_send_focused_client(resource, "dummy-id");
}

/**
 * get the list of client details
 *
 * @param id id of the client or group
 */
static void firebolt_wm_get_clients (struct wl_client *client,
                                 struct wl_resource *resource)
{
    RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information, "firebolt_wm_get_clients");
    firebolt_wm_send_clients(resource, "dummy-id");
}


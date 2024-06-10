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
#include <sstream>
#include "logger.h"
#include "westeros-compositor.h"
#include "firebolt_wm.h"
#include "firebolt_wm_protocol_server.h"
#include "compositorcontroller.h"
#include "rdkcompositor.h"

#define FB_WM_DISPLAY_DEFAULT_XY_POSITION       (0)
#define FB_WM_DISPLAY_DEFAULT_OPACITY           (1.0)
#define FB_WM_DISPLAY_DEFAULT_VISIBILE_FLAG     (false)
#define FB_WM_DISPLAY_DEFAULT_FOCUS_FLAG        (false)
#define FB_WM_DISPLAY_DEFAULT_ZORDER_FLAG       (true)

struct fireboltWmContext
{
    firebolt_window_manager *instance;
    WstCompositor           *wstComp;
    struct wl_display       *wlDisplay;
    struct wl_surface       *wlSurface;
    wl_resource             *wlResource;
    wl_global               *wlGlobal;
    std::string             wstDispName;
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
 * Constructor of the firebolt window manager
 *
 * @param Compositor    : Object of RDK window manager compositor
 */
firebolt_window_manager::firebolt_window_manager(std::shared_ptr<RdkWindowManager::RdkCompositor> &Compositor)
    : mCompositor(Compositor)
{
    RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
            " firebolt_wm@.firebolt_window_manager: constructor Compositor:%p", Compositor);
}

/**
 * Destructor of the firebolt window manager
 */
firebolt_window_manager::~firebolt_window_manager()
{
    RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
            " firebolt_wm@.~firebolt_window_manager: destructor");
    mCompositor.reset();
}

/**
 * Creates temporary ownership of the managed RdkWindowManager compositor object
 */
std::shared_ptr<RdkWindowManager::RdkCompositor> firebolt_window_manager::Compositor() const
{
    return mCompositor.lock();
}

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
    if (id != NULL)
    {
        RdkWindowManager::ClientInfo clientInfo;

        /* Set the properties of the client display */
        memset(&clientInfo, 0, sizeof(clientInfo));
        clientInfo.x        = x;
        clientInfo.y        = y;
        clientInfo.width    = width;
        clientInfo.height   = height;
        clientInfo.opacity  = wl_fixed_to_double(opacity);
        clientInfo.zorder   = zorder; /* TBD: zorder to be supported */
        clientInfo.visible  = visible;
        if (!RdkWindowManager::CompositorController::setClientInfo(id, clientInfo))
        {
            RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Error,
                    " firebolt_wm@.set_properties: client@%p resource@%p id:%s"
                    " clientInfo{x:%d y:%d width:%u height:%u" \
                    " opacity:%f zorder:%u visible:%u} - id not exist!",
                    client, resource, id, clientInfo.x, clientInfo.y,
                    clientInfo.width, clientInfo.height, clientInfo.opacity,
                    clientInfo.zorder, clientInfo.visible);
            goto ret_fail;
        }
        else
        {
            RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
                    " firebolt_wm@.set_properties: client@%p resource@%p id:%s"
                    " clientInfo{x:%d y:%d width:%u height:%u" \
                    " opacity:%f zorder:%u visible:%u} - Success",
                    client, resource, id, clientInfo.x, clientInfo.y,
                    clientInfo.width, clientInfo.height, clientInfo.opacity,
                    clientInfo.zorder, clientInfo.visible);

            if ((render_width > 0) || (render_height > 0))
            {
                /* Enable Virtual Display */
                if (RdkWindowManager::CompositorController::enableVirtualDisplay(id, true))
                {
                    RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
                            " firebolt_wm@.set_properties: enableVirtualDisplay id:%s - Success", id);
                }

                /* Set the client virtual display size */
                if (RdkWindowManager::CompositorController::setVirtualResolution(id, render_width, render_width))
                {
                    RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
                            " firebolt_wm@.set_properties: setVirtualResolution id:%s" \
                            " client display{width:%u height:%u} - Success", id, width, height);
                }
            }

            /* TODO: crop and zorder properties to be supported */
        }
    }
    else
    {
        RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Error,
                " firebolt_wm@.set_properties: client@%p resource@%p" \
                " id:%p surface{x:%d y:%d width:%u height:%u} render{width:%u height:%u}" \
                " opacity:%f zorder:%u visible:%u crop{x:%f y:%f width:%f height:%f} - invalid id param!",
                client, resource, id, x, y, width, height, render_width, render_height,
                wl_fixed_to_double(opacity), zorder, visible, wl_fixed_to_double(crop_x),
                wl_fixed_to_double(crop_y), wl_fixed_to_double(crop_width), wl_fixed_to_double(crop_height));
    }

ret_fail:
    return;
}

/**
 * Create window manager surface for the app or group with defaults.
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
    if (id != NULL)
    {
        RdkWindowManager::ClientInfo clientInfo;
        std::string displayName = "display_";
        bool virtualDispEnabled = true;
        uint32_t virtualWidth;
        uint32_t virtualHeight;
        uint32_t width;
        uint32_t height;

        /* Querying device resolution to set default width and height as same as device resolution */
        RdkWindowManager::CompositorController::getScreenResolution(width, height);

        /* Virutal display width and height same as display size */
        virtualWidth  = width;
        virtualHeight = height;

        /* Set display name as display_${id} */
        displayName += id;

        /* Create a new wayland display */
        if (!RdkWindowManager::CompositorController::createDisplay(id, displayName,
                    width, height, virtualDispEnabled, virtualWidth, virtualHeight,
                    FB_WM_DISPLAY_DEFAULT_ZORDER_FLAG, FB_WM_DISPLAY_DEFAULT_FOCUS_FLAG, false))
        {
            RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Error,
                    " firebolt_wm@.create: client@%p resource@%p id:%s dispName:%s" \
                    " defaults{width:%u height:%u zorder:%u focus:%d}" \
                    " virtualDisplay{enable:%d width:%u height:%u} - Failed to create display!",
                    client, resource, id, displayName.c_str(), width, height,
                    FB_WM_DISPLAY_DEFAULT_ZORDER_FLAG, FB_WM_DISPLAY_DEFAULT_FOCUS_FLAG,
                    virtualDispEnabled, virtualWidth, virtualHeight);
        }
        else
        {
            RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
                    " firebolt_wm@.create: client@%p resource@%p id:%s dispName:%s" \
                    " defaults{width:%u height:%u zorder:%u focus:%d}" \
                    " virtualDisplay{enable:%d width:%u height:%u} - Success",
                    client, resource, id, displayName.c_str(), width, height,
                    FB_WM_DISPLAY_DEFAULT_ZORDER_FLAG, FB_WM_DISPLAY_DEFAULT_FOCUS_FLAG,
                    virtualDispEnabled, virtualWidth, virtualHeight);
            
            /* Defaults display settings */
            memset(&clientInfo, 0, sizeof(clientInfo));
            clientInfo.x = clientInfo.y = FB_WM_DISPLAY_DEFAULT_XY_POSITION;
            clientInfo.width   = width;
            clientInfo.height  = height;
            clientInfo.zorder  = 1; /* TBD: topmost + 1 to be supported */
            clientInfo.opacity = FB_WM_DISPLAY_DEFAULT_OPACITY;
            clientInfo.visible = FB_WM_DISPLAY_DEFAULT_VISIBILE_FLAG;

            /* Set the properties of the client display */
            if (!RdkWindowManager::CompositorController::setClientInfo(id, clientInfo))
            {
                RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Error,
                        " firebolt_wm@.create: set_properties id:%s" \
                        " clientInfo{x:%d y:%d width:%u height:%u" \
                        " opacity:%f zorder:%u visible:%u} - id not exist!",
                        id, clientInfo.x, clientInfo.y, clientInfo.width,
                        clientInfo.height, clientInfo.opacity,
                        clientInfo.zorder, clientInfo.visible);
            }
            else
            {
                RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
                        " firebolt_wm@.create: set_properties id:%s" \
                        " clientInfo{x:%d y:%d width:%u height:%u" \
                        " opacity:%f zorder:%u visible:%u} - Success",
                        id, clientInfo.x, clientInfo.y, clientInfo.width,
                        clientInfo.height, clientInfo.opacity,
                        clientInfo.zorder, clientInfo.visible);
            }

            /* TODO: crop properties to be supported */
        }
    }
    else
    {
        RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Error,
                " firebolt_wm@.create: client@%p resource@%p id:%p - invalid id param!",
                client, resource, id);
    }
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
    if (id != NULL)
    {
        std::string displayName = "display_";
        bool virtualDispEnabled = true;
        uint32_t virtualWidth;
        uint32_t virtualHeight;

        /* Virutal display width and height same as display size */
        virtualWidth  = width;
        virtualHeight = height;

        /* Set display name as display_${id} */
        displayName += id;

        /* Create a new wayland display */
        if (!RdkWindowManager::CompositorController::createDisplay(id, displayName,
                    width, height, virtualDispEnabled, virtualWidth, virtualHeight,
                    FB_WM_DISPLAY_DEFAULT_ZORDER_FLAG, FB_WM_DISPLAY_DEFAULT_FOCUS_FLAG, false))
        {
            RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Error,
                    " firebolt_wm@.create_with_bounds: client@%p resource@%p id:%s dispName:%s" \
                    " display{width:%u height:%u zorder:%u focus:%d}" \
                    " virtualDisplay{enable:%d width:%u height:%u} - Failed to create display!",
                    client, resource, id, displayName.c_str(), width, height,
                    FB_WM_DISPLAY_DEFAULT_ZORDER_FLAG, FB_WM_DISPLAY_DEFAULT_FOCUS_FLAG,
                    virtualDispEnabled, virtualWidth, virtualHeight);
        }
        else
        {
            RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
                    " firebolt_wm@.create_with_bounds: client@%p resource@%p id:%s dispName:%s" \
                    " display{width:%u height:%u zorder:%u focus:%d}" \
                    " virtualDisplay{enable:%d width:%u height:%u} - Success",
                    client, resource, id, displayName.c_str(), width, height,
                    FB_WM_DISPLAY_DEFAULT_ZORDER_FLAG, FB_WM_DISPLAY_DEFAULT_FOCUS_FLAG,
                    virtualDispEnabled, virtualWidth, virtualHeight);

            /* Set the properties of the display boundary */
            if (!RdkWindowManager::CompositorController::setBounds(id, x, y, width, height))
            {
                RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Error,
                        " firebolt_wm@.create_with_bounds: id:%s"
                        " boundary{x:%d y:%d width:%u height:%u} - id not exist!",
                        id, x, y, width, height);
            }
            else
            {
                RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
                        " firebolt_wm@.create_with_bounds: id:%s"
                        " boundary{x:%d y:%d width:%u height:%u} - Success",
                        id, x, y, width, height);
            }
        }
    }
    else
    {
        RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Error,
                " firebolt_wm@.create_with_bounds: client@%p resource@%p id:%p - invalid id param!",
                client, resource, id);
    }
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
    if (id != NULL)
    {
        RdkWindowManager::ClientInfo clientInfo;
        std::string displayName = "display_";
        bool virtualDispEnabled = true;
        uint32_t virtualWidth;
        uint32_t virtualHeight;

        /* Virutal display width and height same as display size */
        virtualWidth  = display_width;
        virtualHeight = display_height;

        /* Set display name as display_${id} */
        displayName += id;

        /* Create a new wayland display */
        if (!RdkWindowManager::CompositorController::createDisplay(id, displayName,
                    width, height, virtualDispEnabled, virtualWidth, virtualHeight,
                    (zorder ? true : false), focused, false))
        {
            RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Error,
                    " firebolt_wm@.create_with_properties: client@%p resource@%p id:%s dispName:%s" \
                    " display{width:%u height:%u zorder:%u(%d) focus:%d}" \
                    " virtualDisplay{enable:%d width:%u height:%u} - Failed to create display!",
                    client, resource, id, displayName.c_str(), width, height, zorder,
                    (zorder ? true : false), focused, virtualDispEnabled, virtualWidth, virtualHeight);
        }
        else
        {
            RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
                    " firebolt_wm@.create_with_properties: client@%p resource@%p id:%s dispName:%s" \
                    " display{width:%u height:%u zorder:%u focus:%d}" \
                    " virtualDisplay{enable:%d width:%u height:%u} - Success",
                    client, resource, id, displayName.c_str(), width, height, zorder,
                    (zorder ? true : false), focused, virtualDispEnabled, virtualWidth, virtualHeight);

            /* Set display settings */
            memset(&clientInfo, 0, sizeof(clientInfo));
            clientInfo.x       = x;
            clientInfo.y       = y;
            clientInfo.width   = width;
            clientInfo.height  = height;
            clientInfo.zorder  = zorder; /* TBD: zorder to be supported */
            clientInfo.opacity = wl_fixed_to_double(opacity);
            clientInfo.visible = visible;

            /* Set the properties of the client display */
            if (!RdkWindowManager::CompositorController::setClientInfo(id, clientInfo))
            {
                RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Error,
                        " firebolt_wm@.create_with_properties: set_properties id:%s" \
                        " clientInfo{x:%d y:%d width:%u height:%u" \
                        " opacity:%f zorder:%u visible:%u} - id not exist!",
                        id, clientInfo.x, clientInfo.y, clientInfo.width,
                        clientInfo.height, clientInfo.opacity,
                        clientInfo.zorder, clientInfo.visible);
            }
            else
            {
                RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
                        " firebolt_wm@.create_with_properties: set_properties id:%s" \
                        " clientInfo{x:%d y:%d width:%u height:%u" \
                        " opacity:%f zorder:%u visible:%u} - Success",
                        id, clientInfo.x, clientInfo.y, clientInfo.width,
                        clientInfo.height, clientInfo.opacity,
                        clientInfo.zorder, clientInfo.visible);

                /* TBD: createDisplay() call already has focus parameter, so need to be decided */
                if (!RdkWindowManager::CompositorController::setFocus(id))
                {
                    RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Error,
                            " firebolt_wm@.create_with_properties: setFocus id:%s - id not exist!", id);
                }
                else
                {
                    RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
                            " firebolt_wm@.create_with_properties: setFocus id:%s - Success!", id);

                }

                /* TODO: crop properties to be supported */
            }
        }
    }
    else
    {
        RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Error,
                " firebolt_wm@.create_with_properties: client@%p resource@%p id:%p - invalid id param!",
                client, resource, id);
    }
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
    if (id != NULL)
    {
        if (!RdkWindowManager::CompositorController::kill(id))
        {
            RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Error,
                    " firebolt_wm@.destroy: client@%p resource@%p app id:%s - id not exist!",
                    client, resource, id);
        }
        else
        {
            RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
                    " firebolt_wm@.destroy: client@%p resource@%p app id:%s - destroyed",
                    client, resource, id);
        }
    }
    else
    {
        RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Error,
                " firebolt_wm@.destroy: client@%p resource@%p app id:%p - invalid id param!",
                client, resource, id);
    }
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
    if (id != NULL)
    {
        if (!RdkWindowManager::CompositorController::setBounds(id, x, y, width, height))
        {
            RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Error,
                    " firebolt_wm@.set_client_bounds: client@%p resource@%p" \
                    " id:%s surface{x:%d y:%d width:%u height:%u} - id not exist!",
                    client, resource, id, x, y, width, height);
        }
        else
        {
            RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
                    " firebolt_wm@.set_client_bounds: client@%p resource@%p" \
                    " id:%s surface{x:%d y:%d width:%u height:%u} - Success",
                    client, resource, id, x, y, width, height);
        }
    }
    else
    {
        RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Error,
                " firebolt_wm@.set_client_bounds: client@%p resource@%p" \
                " id:%p surface{x:%d y:%d width:%u height:%u} - invalid id param!",
                client, resource, id, x, y, width, height);
    }

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
            " firebolt_wm@.set_client_display_bounds: client@%p resource@%p" \
            " id:%s surface{width:%u height:%u}",
            client, resource, (id == NULL ? "NULL" :id), width, height);

    if (id != NULL)
    {
        bool isVirtualDispEnabled = false;

        /* Query the value of client virtual display enabled for given id of the app */
        if (!RdkWindowManager::CompositorController::getVirtualDisplayEnabled(id, isVirtualDispEnabled))
        {
            RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Error,
                    " firebolt_wm@.set_client_display_bounds: client@%p resource@%p" \
                    " id:%s display{width:%u height:%u} - id not exist!",
                    client, resource, id, width, height);
            goto ret_fail;
        }

        /* Check whether the client virtual display enabled or not */
        if (!isVirtualDispEnabled)
        {
            /* Enable Virtual Display, if not enabled */
            if (RdkWindowManager::CompositorController::enableVirtualDisplay(id, true))
            {
                RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
                        " firebolt_wm@.set_client_display_bounds: client@%p resource@%p" \
                        " id:%s client display{width:%u height:%u} - Success",
                        client, resource, id, width, height);
            }
        }

        /* Set the client virtual display size */
        if (RdkWindowManager::CompositorController::setVirtualResolution(id, width, height))
        {
            RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
                    " firebolt_wm@.set_client_display_bounds: setVirtualResolution client@%p resource@%p" \
                    " id:%s client display{width:%u height:%u} - Success",
                    client, resource, id, width, height);
        }
    }
    else
    {
        RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Error,
                " firebolt_wm@.set_client_display_bounds: client@%p resource@%p" \
                " id:%s client display{width:%u height:%u} - invalid id param!",
                client, resource, id, width, height);
    }

ret_fail:
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
    if (id != NULL) 
    {
        if (!RdkWindowManager::CompositorController::setFocus(id))
        {
            RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Error,
                    " firebolt_wm@.set_client_focus: client@%p resource@%p app id:%s - id not exist!",
                    client, resource, id);
        }
        else
        {
            RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
                    " firebolt_wm@.set_client_focus: client@%p resource@%p app id:%s  - Success",
                    client, resource, id);
        }
    }
    else
    {
        RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Error,
                " firebolt_wm@.set_client_focus: client@%p resource@%p app id:%p - invalid id param!",
                client, resource, id);
    }
    return;
}

/**
 * get the app or client properties uing the app or group id
 *
 * @param id : id of the client or group
 */
static void firebolt_wm_get_properties(struct wl_client *client,
                                 struct wl_resource *resource,
                                 const char *id)
{
    if (id != NULL)
    {
        RdkWindowManager::ClientInfo clientInfo;

        memset(&clientInfo, 0, sizeof(clientInfo));

        /* Get the properties of the given client app id */
        if (!RdkWindowManager::CompositorController::getClientInfo(id, clientInfo))
        {
            RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Error,
                    " firebolt_wm@.get_properties: client@%p resource@%p id:%s - id not exist!",
                    client, resource, id);
            goto ret_fail;
        }
        else
        {
            RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
                    " firebolt_wm@.get_properties: getClientInfo id:%s" \
                    " clientInfo{x:%d y:%d width:%u height:%u" \
                    " opacity:%f zorder:%u visible:%u} - Success",
                    id, clientInfo.x, clientInfo.y, clientInfo.width,
                    clientInfo.height, clientInfo.opacity,
                    clientInfo.zorder, clientInfo.visible);
        }

        /* TODO: crop and textured properties yet to be filled */

        RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
                " firebolt_wm@.event: post client_properties resource@%p id:%s" \
                " surface{x:%d y:%d width:%u height:%u opacity:%f zorder:%d visible:%u}" \
                " crop{x:%f y:%f width:%f height:%f} textured:%d",
                resource, id, clientInfo.x, clientInfo.y, clientInfo.width, clientInfo.height,
                clientInfo.opacity, clientInfo.zorder, clientInfo.visible, 0, 0, 0, 0, 0);

        /* Notifying client_properties event to the caller of the app id */
        firebolt_wm_send_client_properties(resource, id, clientInfo.x, clientInfo.y,
                    clientInfo.width, clientInfo.height, wl_fixed_from_double(clientInfo.opacity),
                    clientInfo.zorder, clientInfo.visible, wl_fixed_from_double(0),
                    wl_fixed_from_double(0), wl_fixed_from_double(0), wl_fixed_from_double(0), 0);
    }

ret_fail:
    return;
}

/**
 * get the focused client details and sends an focused_client
 * event to the client owning the resource.
 *
 */
static void firebolt_wm_get_focused_client (struct wl_client *client, struct wl_resource *resource)
{
    std::string clientId;

    RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
            " firebolt_wm@.get_focused_client: client@%p resource@%p", client, resource);

    /* Query the name of focused client id/name */
    if (RdkWindowManager::CompositorController::getFocused(clientId))
    {
        /* Notifying focused_client event to the caller of the app id */
        RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
                " firebolt_wm@.get_focused_client: post event focused_client client@%p" \
                " resource@%p focusedClientID:%s", client, resource, clientId.c_str());
        firebolt_wm_send_focused_client(resource, clientId.c_str());
    }

    return;
}

/**
 * get the list of client details and Sends an clients
 * event to the client owning the resource.
 *
 * Return a comma separated list of client ids
 */
static void firebolt_wm_get_clients (struct wl_client *client, struct wl_resource *resource)
{
    std::vector<std::string> clients;
    std::stringstream clientIdList("");

    RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
            " firebolt_wm@.get_clients: client@%p resource@%p", client, resource);

    /* Get list of available client ids */
    RdkWindowManager::CompositorController::getClients(clients);

    /* Preparing a comma separated list of client ids */
    for (size_t i = 0; i < clients.size(); i++)
    {
        clientIdList << clients[i];
        if (i != clients.size()-1)
        {
            clientIdList << ",";
        }
    }

    /* Notifying event using hardcorded value for testing purpose */
    RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
            " firebolt_wm@.event: post clients resource@%p clients %s",
            resource, (char *)clientIdList.str().c_str());

    firebolt_wm_send_clients(resource, (const char *)clientIdList.str().c_str());

    return;
}

/**
 * To destory firebolt_wm interface resource
 *
 */
static void firebolt_wm_resource_destory(struct wl_resource *resource)
{
    RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
            " firebolt_wm@.resource_destory: resource@%p", resource);

    fireboltWmContext *l_fbWmCtx = reinterpret_cast<fireboltWmContext*>(wl_resource_get_user_data(resource));
    if (NULL != l_fbWmCtx)
    {
        auto *instance = reinterpret_cast<firebolt_window_manager*>(l_fbWmCtx->instance);
        RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
                " firebolt_wm@.resource_destory: instance@%p resource:%p",
                l_fbWmCtx->instance, l_fbWmCtx->wlResource);

        /* delete the firebolt window manager extension instance object */
        delete instance;
        l_fbWmCtx->instance = NULL;

        /* To clear resource user data */
        wl_resource_set_user_data(l_fbWmCtx->wlResource, NULL);

        /* resource destroy */
        wl_resource_destroy(l_fbWmCtx->wlResource);
        RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
                " firebolt_wm@.resource_destory: resource@%p sucsess", resource);
    }
    else
    {
        RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Warn,
                " firebolt_wm@.resource_destory: resource@%p - instance not vaild!", resource);

    }
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
static void firebolt_wm_bind(struct wl_client *client, void *data, uint32_t version, uint32_t id)
{
    std::string dispname = "";

    fireboltWmContext *l_fbWmCtx = reinterpret_cast<fireboltWmContext*>(data);
    RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
            " firebolt_wm@.bind: client@%p data:%p version:%u, id:%u",
            client, data, version, id);
    if (NULL == l_fbWmCtx)
    {
        RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Error,
                " firebolt_wm@.bind: interface context object not valid");
        goto ret_fail;
    }

    if (NULL == l_fbWmCtx->instance)
    {
        /* To get rdk compositor object */
        dispname = WstCompositorGetDisplayName(l_fbWmCtx->wstComp);
        RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
                " firebolt_wm@.bind: WstCompositor display name:%s", dispname.c_str());

        std::shared_ptr<RdkWindowManager::RdkCompositor> l_compositor = \
                        RdkWindowManager::CompositorController::getCompositor(dispname);
        if (!l_compositor)
        {
            RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Error,
                    " firebolt_wm@.bind: missing object for compositor instance");
            wl_client_post_no_memory(client);
            goto ret_fail;
        }

        /* To create a new instance of firebolt window manager extension */
        l_fbWmCtx->instance = new firebolt_window_manager(l_compositor);
        if (!l_fbWmCtx->instance)
        {
            RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Error,
                    " firebolt_wm@.bind: id:%d no memory for firebolt_wm instance!", id);
            wl_client_post_no_memory(client);
            goto ret_fail;
        }
        l_fbWmCtx->wstDispName = dispname;

        /* To create resource object for firebolt window manager extension  */
        l_fbWmCtx->wlResource = wl_resource_create(client,
                                                &firebolt_wm_interface,
                                                std::min<int>(version, 1), id);
        if (!l_fbWmCtx->wlResource)
        {
            RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Error,
                    " firebolt_wm@.bind: id:%d wl_resource_create - no memory", id);
            wl_client_post_no_memory(client);

            /* Instance context clean up */
            delete l_fbWmCtx->instance;
            l_fbWmCtx->instance = NULL;

            goto ret_fail;
        }
        else
        {
            RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
                    " firebolt_wm@.bind: id:%d wl_resource_create resource:%p",
                    id, l_fbWmCtx->wlResource);
        }

        /* Set the implementation of firebolt window manager extension */
        RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
                " firebolt_wm@.bind: id:%d wl_resource_set_implementation", id);
        wl_resource_set_implementation(l_fbWmCtx->wlResource,
                                        &fireboltWindowManagerImpl,
                                        l_fbWmCtx,
                                        firebolt_wm_resource_destory);
    }
    else
    {
        RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
                " firebolt_wm@.bind: firebolt_window_manager context instantiated");
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
                " firebolt_wm@.moduleInit: wstComp@%p wlDisplay@%p initializing",
                wstComp, display);
        if (!bfbWMInitialized)
        {
            if (NULL == f_fbWmCtx)
            {
                f_fbWmCtx = (fireboltWmContext *)malloc(sizeof(fireboltWmContext));
                if (NULL == f_fbWmCtx)
                {
                    RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Error,
                            " firebolt_wm@.moduleInit: no memory for context object");
                    ret = false;
                    goto ret_fail;
                }
                else
                {
                    memset(f_fbWmCtx, 0, sizeof(fireboltWmContext));
                    f_fbWmCtx->wstComp = wstComp;
                    f_fbWmCtx->wlDisplay = display;

                    /* Create extension global object */
                    f_fbWmCtx->wlGlobal = wl_global_create(display,
                                                           &firebolt_wm_interface,
                                                           1, f_fbWmCtx,
                                                           firebolt_wm_bind);
                    if (!f_fbWmCtx->wlGlobal)
                    {
                        RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Error,
                                " firebolt_wm@.moduleInit: Failed to wl_global_create interface:firebolt_wm");
                        ret = false;
                        goto ret_fail;
                    }
                    else
                    {
                        RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
                                " firebolt_wm@.moduleInit: wstComp@%p wlDisplay@%p wlGlobal@%p initialized",
                                f_fbWmCtx->wstComp,
                                f_fbWmCtx->wlDisplay,
                                f_fbWmCtx->wlGlobal);
                    }

                    /* Set initialized state flag */
                    bfbWMInitialized = true;
                }
            }
            else
            {
                RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Error,
                        " firebolt_wm@.moduleInit: firebolt_wm extension not in proper state!");
            }
        }
        else
        {
            RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Warn,
                    " firebolt_wm@.moduleInit: firebolt_wm extension already initialized");
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
                " firebolt_wm@.moduleTerm: firebolt_wm extension terminating");

        if (bfbWMInitialized && (NULL != f_fbWmCtx))
        {
            /* Remove extension global object and destroy it */
            if (NULL != f_fbWmCtx->wlGlobal)
            {
                wl_global_destroy (f_fbWmCtx->wlGlobal);
                f_fbWmCtx->wlGlobal = NULL;
            }

            /* Delete extension context object */
            free(f_fbWmCtx);
            f_fbWmCtx = NULL;

            /* Clear initialized state flag */
            bfbWMInitialized = false;
        }
        else
        {
            RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Warn,
                    " firebolt_wm@.moduleTerm: firebolt_wm extension not initialized!");
        }
        return;
    }
}


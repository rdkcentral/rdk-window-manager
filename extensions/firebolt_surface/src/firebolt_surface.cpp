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
#include <thread>
#include <mutex>
#include "logger.h"
#include "firebolt_surface.h"
#include "compositorcontroller.h"

FireboltSurface* FireboltSurface::mInstance = NULL;
std::mutex FireboltSurface::mContextLock;

static void firebolt_surface_destroy(struct wl_client *client, struct wl_resource *resource, int32_t surfaceId);
static void firebolt_surface_set_name(struct wl_client *client, struct wl_resource *resource, int32_t surfaceId, const char *name);
static void firebolt_surface_set_visible(struct wl_client *client, struct wl_resource *resource, int32_t surfaceId, uint32_t visible);
static void firebolt_surface_set_bounds(struct wl_client *client, struct wl_resource *resource, int32_t surfaceId,
                                        int32_t x, int32_t y, int32_t width, int32_t height);
static void firebolt_surface_set_crop(struct wl_client *client, struct wl_resource *resource, int32_t surfaceId,
                                        wl_fixed_t sx, wl_fixed_t sy, wl_fixed_t swidth, wl_fixed_t  sheight);
static void firebolt_surface_set_zorder(struct wl_client *client, struct wl_resource *resource, int32_t surfaceId, wl_fixed_t zorder);
static void firebolt_surface_set_opacity(struct wl_client *client, struct wl_resource *resource, int32_t surfaceId, wl_fixed_t opacity);

/* vtable of firebot_surface interfaces implementation */
static const struct firebolt_surface_interface fireboltSurfaceInterfaceImpl = {
                        .destroy        = firebolt_surface_destroy,
                        .set_name       = firebolt_surface_set_name,
                        .set_visible    = firebolt_surface_set_visible,
                        .set_bounds     = firebolt_surface_set_bounds,
                        .set_crop       = firebolt_surface_set_crop,
                        .set_zorder     = firebolt_surface_set_zorder,
                        .set_opacity    = firebolt_surface_set_opacity
                    };

/**
 * get the client name from wayland resource
 *
 * @param resource      : wayland resource
 * @param clientName    : client name associated with the resource
 */
bool FireboltSurface::getClientNameByResource ( wl_resource *resource, std::string& clientName)
{
    bool ret = false;

    if (resource != nullptr)
    {
        FireboltSurface *fbSurfaceCtx = reinterpret_cast<FireboltSurface*>(wl_resource_get_user_data(resource));
        clientName =  fbSurfaceCtx->mClientNames.find(resource)->second;

        if (!clientName.empty())
        {
            ret = true;
        }
    }
    return ret;
}

/**
 * Constructor of the firebolt_surface
 *
 */
FireboltSurface::FireboltSurface()
        :mWstCompositor(NULL), mWlDisplay(NULL), mWlResource(NULL), mWlGlobal(NULL), mWstDisplayName(), mClientNames()
{
    RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
            " firebolt_surface@.FireboltSurface: constructor");
}

/**
 * Destructor of the firebolt_surface
 */
FireboltSurface::~FireboltSurface()
{
    RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
            " firebolt_surface@.~FireboltSurface: destructor");
}


/**
 * destroy the firebolt_surface
 *
 * Destroy the firebolt_surface  This removes the
 * association with the underlying wl_surface or hardware video
 * surface and removes the surface from the composition.
 *
 * @param surfaceId : Surface id of the app
 */
static void firebolt_surface_destroy(struct wl_client *client, struct wl_resource *resource, int32_t surfaceId)
{
    FireboltSurface *fbSurfaceCtx = reinterpret_cast<FireboltSurface*>(wl_resource_get_user_data(resource));
    std::string     clientName;
    std::map<wl_resource *,std::string>::iterator it;

    if (nullptr != fbSurfaceCtx )
    {
        if ( true == fbSurfaceCtx->getClientNameByResource(resource, clientName))
        {
            if (!RdkWindowManager::CompositorController::fireboltSurfaceDestroy(clientName, surfaceId))
            {
                RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Error,
                                " firebolt_surface@.destroy failed client@%p resource@%p surfaceId:%d",
                                 client, resource, surfaceId);
            }
            else
            {
                RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
                                " firebolt_surface@.destroy client@%p resource@%p surfaceId:%d",
                                client, resource, surfaceId);

                it=fbSurfaceCtx->mClientNames.find(resource);
                fbSurfaceCtx->mClientNames.erase (it);
            }
        }
        else
        {
            RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Error,
                            " firebolt_surface@.destroy failed to get the surface name client@%p resource@%p surfaceId:%d",
                             client, resource, surfaceId);
        }
    }
    else
    {
        RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Error,
                        " firebolt_surface@.destroy failed to get the surface instance client@%p resource@%p surfaceId:%d",
                         client, resource, surfaceId);
    }
    return;
}

/**
 * set the name of the firebolt_surface
 *
 * Sets the name of the firebolt surface
 *
 * @param surfaceId : Surface id of the app
 * @param name      : name of the surface
 */
static void firebolt_surface_set_name(struct wl_client *client, struct wl_resource *resource, int32_t surfaceId,
                                    const char *name)
{
    FireboltSurface *fbSurfaceCtx = reinterpret_cast<FireboltSurface*>(wl_resource_get_user_data(resource));
    std::string     clientName;

    if (nullptr != fbSurfaceCtx )
    {
        if ( true == fbSurfaceCtx->getClientNameByResource(resource, clientName))
        {
            if (!RdkWindowManager::CompositorController::setFireboltSurfaceName(clientName, surfaceId, name))
            {
                RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Error,
                                " firebolt_surface@.set_name failed client@%p resource@%p surfaceId:%d",
                                 client, resource, surfaceId);
            }
            else
            {
                RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
                                " firebolt_surface@.set_name client@%p resource@%p surfaceId:%d name:%s",
                                client, resource, surfaceId, name);
            }
        }
        else
        {
            RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Error,
                            " firebolt_surface@.set_name failed to get the surface name client@%p resource@%p surfaceId:%d",
                             client, resource, surfaceId);
        }
    }
    else
    {
        RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Error,
                        " firebolt_surface@.destroy failed to get the surface instance client@%p resource@%p surfaceId:%d",
                         client, resource, surfaceId);
    }
    return;
}


/*
 * set the visibility of the surface
 * Setting to 0 makes the surface not visible.
 *
 * @param surfaceId : Surface id of the app
 * @param visible   : type usigned int of enum firebolt_surface_visibility
 */
static void firebolt_surface_set_visible(struct wl_client *client, struct wl_resource *resource, int32_t surfaceId,
                                        uint32_t visible)
{
    FireboltSurface *fbSurfaceCtx = reinterpret_cast<FireboltSurface*>(wl_resource_get_user_data(resource));
    std::string     clientName;

    if (nullptr != fbSurfaceCtx )
    {
        if ( true == fbSurfaceCtx->getClientNameByResource(resource, clientName))
        {
            if (!RdkWindowManager::CompositorController::setFireboltSurfaceVisibility(clientName, surfaceId, visible))
            {
                RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Error,
                                " firebolt_surface@.set_visible failed client@%p resource@%p surfaceId:%d ",
                                 client, resource, surfaceId);
            }
            else
            {
                RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
                                " firebolt_surface@.set_visible: client@%p resource@%p surfaceId:%d visible:%u",
                                client, resource, surfaceId, visible);
            }
        }
        else
        {
            RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Error,
                            " firebolt_surface@.set_visible failed to get the surface name client@%p resource@%p surfaceId:%d",
                             client, resource, surfaceId);
        }
    }
    else
    {
        RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Error,
                        " firebolt_surface@.destroy failed to get the surface instance client@%p resource@%p surfaceId:%d",
                         client, resource, surfaceId);
    }
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
 * @param surfaceId : Surface id of the app
 * @param x         : the left position of the surface
 * @param y         : the top position of the surface
 * @param width     : the width of the surface
 * @param height    : the height of the surface
 */
static void firebolt_surface_set_bounds(struct wl_client *client,
                                        struct wl_resource *resource, int32_t surfaceId, int32_t x, int32_t y, int32_t width, int32_t height)
{
    FireboltSurface *fbSurfaceCtx = reinterpret_cast<FireboltSurface*>(wl_resource_get_user_data(resource));
    std::string     clientName;

    if (nullptr != fbSurfaceCtx )
    {
        if ( true == fbSurfaceCtx->getClientNameByResource(resource, clientName))
        {
            if (!RdkWindowManager::CompositorController::setFireboltSurfaceBounds(clientName, surfaceId, x, y, width, height))
            {
                RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Error,
                                " firebolt_surface@.set_bounds failed client@%p resource@%p surfaceId:%d ",
                                 client, resource, surfaceId);
            }
            else
            {
                RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
                                " firebolt_surface@.set_bounds: client@%p resource@%p surfaceId:%d x:%d y:%d width:%d height:%d",
                                client, resource, surfaceId, x, y, width, height);
            }
        }
        else
        {
            RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Error,
                            " firebolt_surface@.set_bounds failed to get the surface name client@%p resource@%p surfaceId:%d",
                             client, resource, surfaceId);
        }
    }
    else
    {
        RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Error,
                        " firebolt_surface@.destroy failed to get the surface instance client@%p resource@%p surfaceId:%d",
                         client, resource, surfaceId);
    }
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
 * @param surfaceId : Surface id of the app
 * @param sx        : the left position of the surface, fixed point number
 * @param sy        : the top position of the surface, fixed point number
 * @param swidth    : the width of the surface, fixed point number
 * @param sheight   : the height of the surface, fixed point number representing
 */
static void firebolt_surface_set_crop(struct wl_client *client, struct wl_resource *resource, int32_t surfaceId,
                                        wl_fixed_t sx, wl_fixed_t sy, wl_fixed_t swidth, wl_fixed_t sheight)
{
    FireboltSurface *fbSurfaceCtx = reinterpret_cast<FireboltSurface*>(wl_resource_get_user_data(resource));
    std::string     clientName;

    if (nullptr != fbSurfaceCtx )
    {
        if ( true == fbSurfaceCtx->getClientNameByResource(resource, clientName))
        {
            if ( !RdkWindowManager::CompositorController::setFireboltSurfaceCrop(clientName, surfaceId, wl_fixed_to_int(sx), wl_fixed_to_int(sy),\
                                    wl_fixed_to_int(swidth), wl_fixed_to_int(sheight)))
            {
                RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Error,
                                " firebolt_surface@.set_crop failed client@%p resource@%p surfaceId:%d ",
                                 client, resource, surfaceId);
            }
            else
            {
                RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
                                " firebolt_surface@.set_crop: client@%p resource@%p surfaceId:%d x:%f y:%f width:%f height:%f",
                                client, resource, surfaceId, wl_fixed_to_double(sx), wl_fixed_to_double(sy),
                                wl_fixed_to_double(swidth), wl_fixed_to_double(sheight));
            }
        }
        else
        {
            RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Error,
                            " firebolt_surface@.set_crop failed to get the surface name client@%p resource@%p surfaceId:%d",
                             client, resource, surfaceId);
        }
    }
    else
    {
        RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Error,
                        " firebolt_surface@.destroy failed to get the surface instance client@%p resource@%p surfaceId:%d",
                         client, resource, surfaceId);
    }
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
 * @param surfaceId : Surface id of the app
 * @param zorder    : z-order of the surface relative to other surface
 */
static void firebolt_surface_set_zorder(struct wl_client *client, struct wl_resource *resource, int32_t surfaceId, wl_fixed_t zorder)
{
    FireboltSurface *fbSurfaceCtx = reinterpret_cast<FireboltSurface*>(wl_resource_get_user_data(resource));
    std::string     clientName;

    if (nullptr != fbSurfaceCtx )
    {
        if ( true == fbSurfaceCtx->getClientNameByResource(resource, clientName))
        {
            if ( !RdkWindowManager::CompositorController::setFireboltSurfaceZorder(clientName, surfaceId, wl_fixed_to_int(zorder)))
            {
                RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Error,
                                " firebolt_surface@.set_zorder failed client@%p resource@%p surfaceId:%d ",
                                 client, resource, surfaceId);
            }
            else
            {
                RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
                                " firebolt_surface@.set_zorder: client@%p resource@%p surfaceId:%d zorder:%f",
                                client, resource, surfaceId, wl_fixed_to_double(zorder));
            }
        }
        else
        {
            RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Error,
                            " firebolt_surface@.set_zorder failed to get the surface name client@%p resource@%p surfaceId:%d",
                             client, resource, surfaceId);
        }
    }
    else
    {
        RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Error,
                        " firebolt_surface@.destroy failed to get the surface instance client@%p resource@%p surfaceId:%d",
                         client, resource, surfaceId);
    }
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
 * @param surfaceId : Surface id of the app
 * @param opacity   : opacity value of the surface
 */
static void firebolt_surface_set_opacity(struct wl_client *client, struct wl_resource *resource, int32_t surfaceId, wl_fixed_t opacity)
{
    FireboltSurface *fbSurfaceCtx = reinterpret_cast<FireboltSurface*>(wl_resource_get_user_data(resource));
    std::string     clientName;

    if (nullptr != fbSurfaceCtx )
    {
        if ( true == fbSurfaceCtx->getClientNameByResource(resource, clientName))
        {
            if ( !RdkWindowManager::CompositorController::setFireboltSurfaceOpacity(clientName, surfaceId, wl_fixed_to_double(opacity)))
            {
                RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Error,
                                " firebolt_surface@.set_opacity failed client@%p resource@%p surfaceId:%d ",
                                 client, resource, surfaceId);
            }
            else
            {
                RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
                                " firebolt_surface@.set_opacity: client@%p resource@%p surfaceId:%d opacity :%f",
                                client, resource, surfaceId, wl_fixed_to_double(opacity));
            }
        }
        else
        {
            RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Error,
                            " firebolt_surface@.set_opacity failed to get the surface name client@%p resource@%p surfaceId:%d",
                             client, resource, surfaceId);
        }
    }
    else
    {
        RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Error,
                        " firebolt_surface@.destroy failed to get the surface instance client@%p resource@%p surfaceId:%d",
                         client, resource, surfaceId);
    }
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
    FireboltSurface *fbSurfaceCtx = reinterpret_cast<FireboltSurface*>(data);
    RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
                            " firebolt_surface@.bind: client@%p data:%p version:%u, id:%u",
                            client, data, version, id);

    if (NULL == fbSurfaceCtx)
    {
        RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Error,
                        " firebolt_surface interface instance not valid");
        goto ret_fail;
    }
    else
    {
        std::string clientName = "";
        std::lock_guard<std::mutex> locker(FireboltSurface::mContextLock);

        bool found = RdkWindowManager::CompositorController::getClientName(fbSurfaceCtx->mWstCompositor, clientName);
        if (found)
        {
            /* To get westeros compositor object */
            fbSurfaceCtx->mWstDisplayName = WstCompositorGetDisplayName(fbSurfaceCtx->mWstCompositor);
            RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
                    " firebolt_surface@.bind: WstCompositor display name:%s", fbSurfaceCtx->mWstDisplayName.c_str());

            /* To create resource object for firebolt window manager surface extension  */
            fbSurfaceCtx->mWlResource = wl_resource_create(client,
                                                    &firebolt_surface_interface,
                                                    std::min<int>(version, 1), id);
            if (!fbSurfaceCtx->mWlResource)
            {
                RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Error,
                        " firebolt_surface@.bind: id:%d wl_resource_create - no memory", id);
                wl_client_post_no_memory(client);
                goto ret_fail;
            }
            else
            {
                RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
                        " firebolt_wm@.bind: id:%d wl_resource_create resource:%p",
                        id, fbSurfaceCtx->mWlResource);

                fbSurfaceCtx->mClientNames[fbSurfaceCtx->mWlResource] = clientName;
                RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
                                " firebolt_surface id:%d wl_resource_set_implementation", id);
                wl_resource_set_implementation(fbSurfaceCtx->mWlResource,
                                            &fireboltSurfaceInterfaceImpl,
                                            fbSurfaceCtx,
                                            NULL);
            }
        }
        else
        {
            RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Error, " firebolt_surface@.bind: id:%d getClientName failed", id);
        }
    }

ret_fail:
    return;
}

extern "C"
{
    static FireboltSurface* fireboltSurfaceCreateContext(void)
    {
        std::lock_guard<std::mutex> locker(FireboltSurface::mContextLock);
        if (NULL == FireboltSurface::mInstance)
        {
            FireboltSurface::mInstance = new FireboltSurface();
            if (NULL == FireboltSurface::mInstance)
            {
                RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Error,
                        " firebolt_wm@.fireboltWmCreateContext: no memory for context object");
            }
            else
            {
                RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
                        " firebolt_wm@.fireboltWmCreateContext instance:%p created", FireboltSurface::mInstance);

            }
        }
        return FireboltSurface::mInstance;
    }

    static void fireboltSurfaceDeleteContext(void)
    {
        std::lock_guard<std::mutex> locker(FireboltSurface::mContextLock);
        if (NULL != FireboltSurface::mInstance)
        {
            RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
                    " firebolt_surface@.fireboltSurfaceDeleteContext instance:%p wlGlobal@%p destory",
                    FireboltSurface::mInstance, FireboltSurface::mInstance->mWlGlobal);

            /* Remove extension global object and destroy it */
            if (NULL != FireboltSurface::mInstance->mWlGlobal)
            {
                wl_global_destroy (FireboltSurface::mInstance->mWlGlobal);
                FireboltSurface::mInstance->mWlGlobal = NULL;
            }
            FireboltSurface::mInstance->mClientNames.clear();

            delete(FireboltSurface::mInstance);
            FireboltSurface::mInstance = NULL;
        }
        return;
    }

    static bool fireboltSurfaceHasContext(void)
    {
        std::lock_guard<std::mutex> locker(FireboltSurface::mContextLock);
        return (NULL != FireboltSurface::mInstance) ? true : false;
    }

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

        if (!fireboltSurfaceHasContext())
        {
            FireboltSurface* fbSurfaceCtx = NULL;
            /* To create a new instance of firebolt window manager extension */
            fbSurfaceCtx = fireboltSurfaceCreateContext();

            if (NULL != fbSurfaceCtx)
            {
                fbSurfaceCtx->mWstCompositor   = wstComp;
                fbSurfaceCtx->mWlDisplay = display;

                /* Create extension global object */
                fbSurfaceCtx->mWlGlobal = wl_global_create(display,
                                                       &firebolt_surface_interface,
                                                       1, fbSurfaceCtx,
                                                       firebolt_surface_bind);
                if (!fbSurfaceCtx->mWlGlobal)
                {
                    RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Error,
                            " firebolt_surface@.moduleInit: Failed to wl_global_create interface:firebolt_surface");
                    ret = false;
                    goto ret_fail;
                }
                else
                {
                    RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
                            " firebolt_surface@.moduleInit: wstComp@%p wlDisplay@%p wlGlobal@%p initialized",
                            fbSurfaceCtx->mWstCompositor,
                            fbSurfaceCtx->mWlDisplay,
                            fbSurfaceCtx->mWlGlobal);
                }
            }
            else
            {
                RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Error,
                        " firebolt_surface@.moduleInit: firebolt_surface extension create failed!");
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
                " firebolt_surface@.moduleTerm: firebolt_surface extension terminating");

        if (fireboltSurfaceHasContext())
        {
            /* Delete extension context */
            fireboltSurfaceDeleteContext();
        }
        else
        {
            RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Warn,
                    " firebolt_surface@.moduleTerm: firebolt_surface extension not initialized!");
        }
    }
}


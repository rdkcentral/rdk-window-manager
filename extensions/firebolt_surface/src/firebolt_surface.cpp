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

/* vtable of firebot_surface interface implementation */
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
 * Constructor of the firebolt_surface
 *
 */
FireboltSurface::FireboltSurface()
        :mWstCompositor(NULL), mWlGlobal(NULL), mWlDisplay(NULL), mWstDisplayName(), mClientListMap()
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
 * Get Firebolt Surface ClientInfo from wayland resource
 *
 * @param resource  : wayland resource
 * @return FireboltSurfaceClientInfo: Pointer to associated clientInfo struct
 */
FireboltSurfaceClientInfo* FireboltSurface::getFireboltSurfaceClientInfo(wl_resource *resource)
{
    FireboltSurfaceClientInfo *clientInfo = NULL;

    if (NULL != resource)
    {
        FireboltSurface *fbSurfaceCtx = reinterpret_cast<FireboltSurface*>(wl_resource_get_user_data(resource));
        if (NULL != fbSurfaceCtx)
        {
            FireboltSurface::ClientListMap::iterator it = fbSurfaceCtx->mClientListMap.find(resource);
            if (it != fbSurfaceCtx->mClientListMap.end()) 
            {
                clientInfo = reinterpret_cast<FireboltSurfaceClientInfo*>(it->second);
            }
        }
    }
    return clientInfo;
}

/**
 * Get Firebolt Surface ClientName by wayland resource
 *
 * @param resource  : wayland resource
 * @return std::string: returns ClientName
 */
std::string FireboltSurface::getFireboltSurfaceClientName(wl_resource *resource)
{
    FireboltSurfaceClientInfo *clientInfo = NULL;
    std::string clientName = "";

    FireboltSurface *fbSurfaceCtx = reinterpret_cast<FireboltSurface*>(wl_resource_get_user_data(resource));
    if ((NULL != fbSurfaceCtx) && (NULL != fbSurfaceCtx->mWstCompositor))
    {
        FireboltSurface::ClientListMap::iterator it = fbSurfaceCtx->mClientListMap.find(resource);
        if (it == fbSurfaceCtx->mClientListMap.end()) 
        {
            RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Error,
                    " firebolt_surface@.ClientName: ClientListMap not found for resource@%p mWstCompositor@%p",
                    resource, fbSurfaceCtx->mWstCompositor);
            goto ret_fail;
        }

        clientInfo = reinterpret_cast<FireboltSurfaceClientInfo*>(it->second);
        if (NULL != clientInfo)
        {
            if (!clientInfo->clientName.empty())
            {
                /* Get bind clientName from clientInfo list */
                clientName.assign(clientInfo->clientName);
                RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
                        " firebolt_surface@.ClientName: clientInfo resource@%p mWstCompositor@%p ClientName:%s found",
                        resource, fbSurfaceCtx->mWstCompositor, clientName.c_str());
            }
            else
            {
                /* clientName not found in clientInfo list, so try to query the clientName again */
                if ((!RdkWindowManager::CompositorController::getClientName(fbSurfaceCtx->mWstCompositor, clientName))
                    || (clientName.empty()))
                {
                    RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Error,
                            " firebolt_surface@.ClientName: getClientName resource@%p WstCompositor@%p not yet found!",
                            resource, fbSurfaceCtx->mWstCompositor);
                    goto ret_fail;
                }

                /* clientName found now and saving it in clientInfo list */
                clientInfo->clientName.assign(clientName);
                RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
                        " firebolt_surface@.ClientName: getClientName resource@%p mWstCompositor@%p ClientName:%s found",
                        resource, fbSurfaceCtx->mWstCompositor, clientName.c_str());
            }
        }
        else
        {
            RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Error,
                    " firebolt_surface@.ClientName: clientInfo not found for resource@%p mWstCompositor@%p",
                    resource, fbSurfaceCtx->mWstCompositor);
        }
    }
    else
    {
        RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Error,
                " firebolt_surface@.ClientName:: clientName not found for resource@%p fbSurfaceCtx@%p mWstCompositor@%p",
                resource, fbSurfaceCtx, ((NULL != fbSurfaceCtx) ? fbSurfaceCtx->mWstCompositor : NULL));
    }

ret_fail:
    return clientName;
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

    if (NULL != resource)
    {
        FireboltSurface *fbSurfaceCtx = reinterpret_cast<FireboltSurface*>(wl_resource_get_user_data(resource));
        if (NULL == fbSurfaceCtx)
        {
            RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Error,
                    " firebolt_surface@.destroy: client@%p resource@%p surfaceId:%d - fbSurfaceCtx not found!",
                    client, resource, surfaceId);
            goto ret_fail;
        }

        std::string clientName = fbSurfaceCtx->getFireboltSurfaceClientName(resource);
        if (!clientName.empty())
        {
            if (!RdkWindowManager::CompositorController::fireboltSurfaceDestroy(clientName, surfaceId))
            {
                RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Error,
                        " firebolt_surface@.destroy: client@%p resource@%p surfaceId:%d failed",
                         client, resource, surfaceId);

                goto ret_fail;
            }
            else
            {
                RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
                        " firebolt_surface@.destroy: client@%p resource@%p surfaceId:%d destoryed",
                        client, resource, surfaceId);
            }
        }
        else
        {
            RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Error,
                    " firebolt_surface@.destroy: Client not found client@%p resource@%p surfaceId:%d",
                    client, resource, surfaceId);
        }
    }
    else
    {
        RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Error,
                " firebolt_surface@.destroy: client@%p resource@%p surfaceId:%d - invalid param",
                client, resource, surfaceId);
    }

ret_fail:
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
static void firebolt_surface_set_name(struct wl_client *client, struct wl_resource *resource,   
                                    int32_t surfaceId, const char *name)
{
    if (NULL != resource)
    {
        std::string surfaceName = "";

        FireboltSurface *fbSurfaceCtx = reinterpret_cast<FireboltSurface*>(wl_resource_get_user_data(resource));
        if (NULL == fbSurfaceCtx)
        {
            RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Error,
                    " firebolt_surface@.set_name: client@%p resource@%p surfaceId:%d name@%p - fbSurfaceCtx not found!",
                    client, resource, surfaceId, name);

            goto ret_fail;
        }

        std::string clientName = fbSurfaceCtx->getFireboltSurfaceClientName(resource);
        if (!clientName.empty())
        {
            surfaceName.assign(name);
            if (!RdkWindowManager::CompositorController::setFireboltSurfaceName(clientName, surfaceId, surfaceName))
            {
                RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Error,
                        " firebolt_surface@.set_name: client@%p resource@%p surfaceId:%d surfaceName:%s failed",
                        client, resource, surfaceId, surfaceName.c_str());
                goto ret_fail;
            }
            else
            {
                RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
                        " firebolt_surface@.set_name: client@%p resource@%p surfaceId:%d surfaceName:%s success",
                        client, resource, surfaceId, surfaceName.c_str());
            }
        }
        else
        {
            RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Error,
                    " firebolt_surface@.set_name: Client not found client@%p resource@%p surfaceId:%d name@%p",
                    client, resource, surfaceId, name);
        }
    }
    else
    {
        RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Error,
                " firebolt_surface@.set_name: client@%p resource@%p surfaceId:%d name@%p - invalid param",
                client, resource, surfaceId, name);
    }

ret_fail:
    return;
}


/*
 * set the visibility of the surface
 * Setting to 0 makes the surface not visible.
 *
 * @param surfaceId : Surface id of the app
 * @param visible   : type usigned int of enum firebolt_surface_visibility
 */
static void firebolt_surface_set_visible(struct wl_client *client, struct wl_resource *resource,
                                        int32_t surfaceId, uint32_t visible)
{
    if (NULL != resource)
    {
        FireboltSurface *fbSurfaceCtx = reinterpret_cast<FireboltSurface*>(wl_resource_get_user_data(resource));
        if (NULL == fbSurfaceCtx)
        {
            RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Error,
                    " firebolt_surface@.set_visible: client@%p resource@%p surfaceId:%d visiblity:%u"
                    " - fbSurfaceCtx not found!", client, resource, surfaceId, visible);

            goto ret_fail;
        }

        std::string clientName = fbSurfaceCtx->getFireboltSurfaceClientName(resource);
        if (!clientName.empty())
        {
            if (!RdkWindowManager::CompositorController::setFireboltSurfaceVisibility(clientName, surfaceId, visible))
            {
                RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Error,
                        " firebolt_surface@.set_visible: client@%p resource@%p surfaceId:%d visiblity:%u failed",
                        client, resource, surfaceId, visible);
                goto ret_fail;
            }
            else
            {
                RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
                        " firebolt_surface@.set_visible: client@%p resource@%p surfaceId:%d visiblity:%u success",
                        client, resource, surfaceId, visible);
            }
        }
        else
        {
            RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Error,
                    " firebolt_surface@.set_visible: Client not found client@%p resource@%p surfaceId:%d visiblity:%u",
                    client, resource, surfaceId, visible);
        }
    }
    else
    {
        RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Error,
                " firebolt_surface@.set_visible: client@%p resource@%p surfaceId:%d visiblity:%u - invalid param",
                client, resource, surfaceId, visible);
    }

ret_fail:
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
static void firebolt_surface_set_bounds(struct wl_client *client, struct wl_resource *resource, int32_t surfaceId,
                                        int32_t x, int32_t y, int32_t width, int32_t height)
{
    if (NULL != resource)
    {
        FireboltSurface *fbSurfaceCtx = reinterpret_cast<FireboltSurface*>(wl_resource_get_user_data(resource));
        if (NULL == fbSurfaceCtx)
        {
            RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Error,
                    " firebolt_surface@.set_bounds: client@%p resource@%p surfaceId:%d"
                    " Surface{x:%d y:%d width:%d height:%d} - fbSurfaceCtx not found!",
                    client, resource, surfaceId, x, y, width, height);

            goto ret_fail;
        }

        std::string clientName = fbSurfaceCtx->getFireboltSurfaceClientName(resource);
        if (!clientName.empty())
        {
            if (!RdkWindowManager::CompositorController::setFireboltSurfaceBounds(clientName, surfaceId, x, y, width, height))
            {
                RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Error,
                        " firebolt_surface@.set_bounds: client@%p resource@%p surfaceId:%d"
                        " Surface{x:%d y:%d width:%d height:%d} failed", client, resource, surfaceId, x, y, width, height);
            }
            else
            {
                RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
                        " firebolt_surface@.set_bounds: client@%p resource@%p surfaceId:%d"
                        " Surface{x:%d y:%d width:%d height:%d} success", client, resource, surfaceId, x, y, width, height);
            }
        }
        else
        {
            RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Error,
                    " firebolt_surface@.set_bounds: Client not found client@%p resource@%p surfaceId:%d"
                    " Surface{x:%d y:%d width:%d height:%d} failed", client, resource, surfaceId, x, y, width, height);
        }
    }
    else
    {
        RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Error,
                " firebolt_surface@.set_bounds: client@%p resource@%p surfaceId:%d",
                " Surface{x:%d y:%d width:%d height:%d} - invalid param", client, resource, surfaceId, x, y, width, height);
    }

ret_fail:
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
    if (NULL != resource)
    {
        FireboltSurface *fbSurfaceCtx = reinterpret_cast<FireboltSurface*>(wl_resource_get_user_data(resource));
        if (NULL == fbSurfaceCtx)
        {
            RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Error,
                    " firebolt_surface@.set_crop: client@%p resource@%p surfaceId:%d"
                    " Surface{x:%f y:%f width:%f height:%f} - fbSurfaceCtx not found!", client, resource, surfaceId,
                    wl_fixed_to_double(sx), wl_fixed_to_double(sy), wl_fixed_to_double(swidth), wl_fixed_to_double(sheight));

            goto ret_fail;
        }

        std::string clientName = fbSurfaceCtx->getFireboltSurfaceClientName(resource);
        if (!clientName.empty())
        {
            if (!RdkWindowManager::CompositorController::setFireboltSurfaceCrop(clientName, surfaceId, wl_fixed_to_int(sx),
                                              wl_fixed_to_int(sy), wl_fixed_to_int(swidth), wl_fixed_to_int(sheight)))
            {
                RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Error,
                        " firebolt_surface@.set_crop: client@%p resource@%p surfaceId:%d"
                        " Surface{x:%d y:%d width:%d height:%d} failed", client, resource, surfaceId, wl_fixed_to_int(sx),
                        wl_fixed_to_int(sy), wl_fixed_to_int(swidth), wl_fixed_to_int(sheight));
            }
            else
            {
                RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
                        " firebolt_surface@.set_crop: client@%p resource@%p surfaceId:%d"
                        " Surface{x:%d y:%d width:%d height:%d} success", client, resource, surfaceId, wl_fixed_to_int(sx),
                        wl_fixed_to_int(sy), wl_fixed_to_int(swidth), wl_fixed_to_int(sheight));
            }
        }
        else
        {
            RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Error,
                    " firebolt_surface@.set_crop: Client not found client@%p resource@%p surfaceId:%d"
                    " Surface{x:%f y:%f width:%f height:%f} failed", client, resource, surfaceId, wl_fixed_to_double(sx),
                    wl_fixed_to_double(sy), wl_fixed_to_double(swidth), wl_fixed_to_double(sheight));
        }
    }
    else
    {
        RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Error,
                " firebolt_surface@.set_crop: client@%p resource@%p surfaceId:%d",
                " Surface{x:%f y:%f width:%f height:%f} - invalid param", client, resource, surfaceId, wl_fixed_to_double(sx),
                    wl_fixed_to_double(sy), wl_fixed_to_double(swidth), wl_fixed_to_double(sheight));
    }

ret_fail:
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
    if (NULL != resource)
    {
        FireboltSurface *fbSurfaceCtx = reinterpret_cast<FireboltSurface*>(wl_resource_get_user_data(resource));
        if (NULL == fbSurfaceCtx)
        {
            RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Error,
                    " firebolt_surface@.set_zorder: client@%p resource@%p surfaceId:%d zorder:%f"
                    " - fbSurfaceCtx not found!", client, resource, surfaceId, wl_fixed_to_double(zorder));

            goto ret_fail;
        }

        std::string clientName = fbSurfaceCtx->getFireboltSurfaceClientName(resource);
        if (!clientName.empty())
        {
            if (!RdkWindowManager::CompositorController::setFireboltSurfaceZorder(clientName, surfaceId, wl_fixed_to_int(zorder)))
            {
                RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Error,
                        " firebolt_surface@.set_zorder: client@%p resource@%p surfaceId:%d zorder:%d failed",
                        client, resource, surfaceId, wl_fixed_to_int(zorder));
                goto ret_fail;
            }
            else
            {
                RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
                        " firebolt_surface@.set_zorder: client@%p resource@%p surfaceId:%d zorder:%d success",
                        client, resource, surfaceId, wl_fixed_to_int(zorder));
            }
        }
        else
        {
            RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Error,
                    " firebolt_surface@.set_zorder: Client not found client@%p resource@%p surfaceId:%d zorder:%f",
                    client, resource, surfaceId, wl_fixed_to_double(zorder));
        }
    }
    else
    {
        RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Error,
                " firebolt_surface@.set_zorder: client@%p resource@%p surfaceId:%d zorder:%f - invalid param",
                client, resource, surfaceId, wl_fixed_to_double(zorder));
    }

ret_fail:
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
    if (NULL != resource)
    {
        FireboltSurface *fbSurfaceCtx = reinterpret_cast<FireboltSurface*>(wl_resource_get_user_data(resource));
        if (NULL == fbSurfaceCtx)
        {
            RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Error,
                    " firebolt_surface@.set_opacity: client@%p resource@%p surfaceId:%d opacity:%f"
                    " - fbSurfaceCtx not found!", client, resource, surfaceId, wl_fixed_to_double(opacity));

            goto ret_fail;
        }

        std::string clientName = fbSurfaceCtx->getFireboltSurfaceClientName(resource);
        if (!clientName.empty())
        {
            if (!RdkWindowManager::CompositorController::setFireboltSurfaceOpacity(clientName, surfaceId, wl_fixed_to_double(opacity)))
            {
                RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Error,
                        " firebolt_surface@.set_opacity: client@%p resource@%p surfaceId:%d opacity:%d failed",
                        client, resource, surfaceId, wl_fixed_to_int(opacity));
                goto ret_fail;
            }
            else
            {
                RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
                        " firebolt_surface@.set_opacity: client@%p resource@%p surfaceId:%d opacity:%d success",
                        client, resource, surfaceId, wl_fixed_to_int(opacity));
            }
        }
        else
        {
            RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Error,
                    " firebolt_surface@.set_opacity: Client not found client@%p resource@%p opacity:%d zorder:%f",
                    client, resource, surfaceId, wl_fixed_to_double(opacity));
        }
    }
    else
    {
        RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Error,
                " firebolt_surface@.set_opacity: client@%p resource@%p surfaceId:%d opacity:%f - invalid param",
                client, resource, surfaceId, wl_fixed_to_double(opacity));
    }

ret_fail:
    return;
}

/**
 * To destory firebolt_surface interface resource
 *
 */
static void firebolt_surface_resource_destory(struct wl_resource *resource)
{
    RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
            " firebolt_surface@.resource_destory: resource@%p", resource);

    FireboltSurface *fbSurfaceCtx = reinterpret_cast<FireboltSurface*>(wl_resource_get_user_data(resource));
    if (NULL != fbSurfaceCtx)
    {
        std::lock_guard<std::mutex> locker(FireboltSurface::mContextLock);
        /* Erase map of resource and client name */
        FireboltSurface::ClientListMap::iterator it = fbSurfaceCtx->mClientListMap.find(resource);
        if (it != fbSurfaceCtx->mClientListMap.end()) 
        {
            FireboltSurfaceClientInfo *clientInfo = reinterpret_cast<FireboltSurfaceClientInfo*>(it->second);
            if ((NULL != clientInfo) && (resource == clientInfo->resource))
            {
                /* To clear resource user data */
                wl_resource_set_user_data(clientInfo->resource, NULL);

                /* resource destroy */
                wl_resource_destroy(clientInfo->resource);

                RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
                        " firebolt_surface@.resource_destory: instance@%p clientInfo->resource:%p",
                        fbSurfaceCtx->mInstance, clientInfo->resource);

                /* delete client info */
                delete clientInfo;
                it->second = NULL;
            }
            else
            {
                RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Warn,
                        " firebolt_surface@.resource_destory: resource@%p - incorrect"
                        " clientInfo@%p resource@%p clientInfo->resource@%p",
                        clientInfo, resource, (clientInfo ? clientInfo->resource : NULL));
            }
            fbSurfaceCtx->mClientListMap.erase(it);
        }
        else
        {
            RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Error,
                    " firebolt_surface@.resource_destory: resource@%p - client not found!", resource);
        }
    }
    else
    {
        RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Warn,
                " firebolt_surface@.resource_destory: resource@%p - instance not vaild!", resource);
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
static void firebolt_surface_bind(struct wl_client *client, void *data, uint32_t version, uint32_t id)
{
    RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
            " firebolt_surface@.bind: client@%p data:%p version:%u, id:%u",
            client, data, version, id);

    FireboltSurface *fbSurfaceCtx = reinterpret_cast<FireboltSurface*>(data);
    if (NULL == fbSurfaceCtx)
    {
        RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Error,
                " firebolt_surface@.bind: interface instance not valid");
        goto ret_fail;
    }
    else
    {
        std::string clientName = "";
        struct wl_resource *resource = NULL;

        std::lock_guard<std::mutex> locker(FireboltSurface::mContextLock);

        /* To get westeros compositor object */
        fbSurfaceCtx->mWstDisplayName = WstCompositorGetDisplayName(fbSurfaceCtx->mWstCompositor);
        RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
                " firebolt_surface@.bind: WstCompositor display name:%s", fbSurfaceCtx->mWstDisplayName.c_str());

        /* To create resource object for firebolt surface extension  */
        resource = wl_resource_create(client,
                                    &firebolt_surface_interface,
                                    std::min<int>(version, 1), id);
        if (!resource)
        {
            RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Error,
                    " firebolt_surface@.bind: id:%u wl_resource_create - no memory", id);
            wl_client_post_no_memory(client);

            goto ret_fail;
        }
        else
        {
            RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
                    " firebolt_surface@.bind: id:%u wl_resource_create resource@%p", id, resource);

            /* Map of wl_resource against client Info */
            FireboltSurfaceClientInfo *clientInfo = new FireboltSurfaceClientInfo;
            if (NULL == clientInfo)
            {
                RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Error,
                        " firebolt_surface@.bind: id:%u FireboltSurfaceClientInfo - no memory", id);
                wl_client_post_no_memory(client);

                goto ret_fail;
            }
            else
            {
                if (RdkWindowManager::CompositorController::getClientName(fbSurfaceCtx->mWstCompositor, clientName))
                {
                    RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
                            " firebolt_surface@.bind: mWstCompositor@%p id:%u getClientName:%s",
                            fbSurfaceCtx->mWstCompositor, id, clientName.c_str());
                }
                else
                {
                    RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Error,
                            " firebolt_surface@.bind: mWstCompositor@%p id:%u getClientName failed",
                            fbSurfaceCtx->mWstCompositor, id);
                }

                /* Set client info detail */
                clientInfo->resource    = resource;
                clientInfo->clientId    = id;
                clientInfo->display     = wl_client_get_display(client);
                clientInfo->clientName.assign(clientName);
                fbSurfaceCtx->mClientListMap[resource] = clientInfo;

                RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
                                " firebolt_surface id:%d wl_resource_set_implementation", id);
                wl_resource_set_implementation(resource,
                                            &fireboltSurfaceInterfaceImpl,
                                            fbSurfaceCtx,
                                            firebolt_surface_resource_destory);
            }
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
                        " firebolt_wm@.fireboltWmCreateContext: instance:%p created", FireboltSurface::mInstance);
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
                    " firebolt_surface@.fireboltSurfaceDeleteContext: instance@%p wlGlobal@%p destory",
                    FireboltSurface::mInstance, FireboltSurface::mInstance->mWlGlobal);

            /* Remove extension global object and destroy it */
            if (NULL != FireboltSurface::mInstance->mWlGlobal)
            {
                wl_global_destroy (FireboltSurface::mInstance->mWlGlobal);
                FireboltSurface::mInstance->mWlGlobal = NULL;
            }

            /* Clear map of resource and client Info */
            for (auto it = FireboltSurface::mInstance->mClientListMap.begin(); it != FireboltSurface::mInstance->mClientListMap.end(); it++)
            {
                FireboltSurfaceClientInfo *clientInfo = reinterpret_cast<FireboltSurfaceClientInfo*>(it->second);
                if (NULL != clientInfo)
                {
                    /* To clear resource user data */
                    wl_resource_set_user_data(clientInfo->resource, NULL);

                    /* resource destroy */
                    wl_resource_destroy(clientInfo->resource);

                    /* delete client info */
                    delete clientInfo;
                    it->second = NULL;
                }
            }
            FireboltSurface::mInstance->mClientListMap.clear();

            /* Delete context */
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
     * @param wstCompositor : Object of westeros compositor instance
     * @param display       : Object of the wayland display
     */
    bool moduleInit(WstCompositor *wstCompositor, struct wl_display *display)
    {
        bool ret = true;
        RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
                " firebolt_surface@.moduleInit: firebolt_surface extension wstCompositor@%p display@%p initializing",
                wstCompositor, display);

        if (!fireboltSurfaceHasContext())
        {
            FireboltSurface* fbSurfaceCtx = NULL;
            /* To create a new instance of firebolt window manager extension */
            fbSurfaceCtx = fireboltSurfaceCreateContext();
            if (NULL != fbSurfaceCtx)
            {
                fbSurfaceCtx->mWstCompositor = wstCompositor;
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
                            " firebolt_surface@.moduleInit: wstCompositor@%p display@%p wlGlobal@%p initialized",
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
            RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Warn,
                    " firebolt_surface@.moduleInit: firebolt_surface extension already initialized");
        }

    ret_fail:
        return ret;
    }

    /**
     * moduleTerm of firebolt_surface westeros extension plugin
     *
     * @param wstCompositor : Object of westeros compositor instance
     */
    void moduleTerm(WstCompositor *wstCompositor)
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
        return;
    }
}


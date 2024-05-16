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

#include "firebolt_surface.h"
#include "westeros-compositor.h"

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

static void firebolt_surface_destroy(struct wl_client *client,
                                             struct wl_resource *resource);
											 
static void firebolt_surface_setName(struct wl_client *client,
                                             struct wl_resource *resource,const char *name);
											 
static void firebolt_surface_setVisible(struct wl_client *client,
                                                struct wl_resource *resource,uint32_t visible);
												
static void firebolt_surface_setBounds(struct wl_client *client,
                                               struct wl_resource *resource,int32_t x, int32_t y, int32_t width, int32_t height);
											   
static void firebolt_surface_setCrop(struct wl_client *client,
                                             struct wl_resource *resource, wl_fixed_t sx, wl_fixed_t sy, wl_fixed_t swidth, wl_fixed_t  sheight);
											 
static void firebolt_surface_setZOrder(struct wl_client *client,
                                               struct wl_resource *resource, wl_fixed_t zorder);
											   
static void firebolt_surface_setOpacity(struct wl_client *client,
                                                struct wl_resource *resource, wl_fixed_t opacity);
											 
/* struct for firebot_surface interface implementation*/
static const struct firebolt_surface_interface fireboltSurfaceInterfaceImpl =
{
    firebolt_surface_destroy,
    firebolt_surface_setName,
    firebolt_surface_setVisible,
    firebolt_surface_setBounds,
    firebolt_surface_setCrop,
    firebolt_surface_setZOrder,
    firebolt_surface_setOpacity
};

/*
 * Destroy the firebolt_surface object. This removes the
 * association with the underlying wl_surface or hardware video
 * surface and removes the surface from the composition.
 * 
 */
static void firebolt_surface_destroy(struct wl_client *client,
                                             struct wl_resource *resource)
{
  RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information, " Destroy called");
}

/*
 * Sets the name of the surface
 * @param name - the name of the string .
 */
static void firebolt_surface_setName(struct wl_client *client,
                                             struct wl_resource *resource,const char *name) {
    RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information, " name :%s",name); 
}

/*
 * Setting to 0 makes the surface not visible.
 * @param visible - type usigned int of enum firebolt_surface_visibility
 */
static void firebolt_surface_setVisible(struct wl_client *client,
                                                struct wl_resource *resource,uint32_t visible) {
    RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information, " visible :%d",visible); 
}

/*
 * The surface bounds of a surface is its composited pixel
   position and dimensions.  If unset then the surface is set
   to the top left, with width and height that match the underlying
   surface dimensions.
 
 * The width and height of the effective surface bounds must be
   greater than zero. Setting an invalid size will raise an invalid_size error .
 
 *@param x : the left position of the surface
 *@param y :  the top position of the surface
 *@param width : the width of the surface
 *@param height : the height of the surface
 */
static void firebolt_surface_setBounds(struct wl_client *client,
                                               struct wl_resource *resource,int32_t x, int32_t y, int32_t width, int32_t height) {
    RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information, " x :%d , y: %d , width :%d , height :%d",x , y, width, height); 
}

/*Sets the cropping of a given surface.
 *
 * Cropping works on an arbitrary scale, not on pixels. The input
   surface is defined as a rectangle with width and height of 1.0
   and therefore all cropping values should be fixed point
   values between 0.0 and 1.0 inclusive.
 
 * For example to crop the top right quarter of the video
   then set (x, y, width, height) to (0.5, 0.0, 0.5, 0.5).
 
 * @param sx : the left position of the surface , fixed point number
 * @param sy : the top position of the surface , fixed point number
 * @param swidth : the width of the surface , fixed point number
 * @param sheight : the height of the surface , fixed point number representing
 */
static void firebolt_surface_setCrop(struct wl_client *client,
                                             struct wl_resource *resource, wl_fixed_t sx, wl_fixed_t sy, wl_fixed_t swidth, wl_fixed_t  sheight) {
    RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information, " sx :%f sy:%f swidth:%f sheight:%f",
                      wl_fixed_to_double(sx) ,wl_fixed_to_double(sy),wl_fixed_to_double(swidth) ,wl_fixed_to_double(sheight)); 
}

/*
 * Sets the z-order of the surface relative to other surfaces within
   the client's display.

 * The z-order should be in the range of 0.0 - 1.0 inclusive.

 * @param zorder
 */
static void firebolt_surface_setZOrder(struct wl_client *client,
                                               struct wl_resource *resource, wl_fixed_t zorder) {
    RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information, " zorder:%f",wl_fixed_to_double(zorder)); 
}

/*
 * Sets the opacity of the surface.  It may not be possible to set the opacity on hardware video surfaces.

 * The opacity should be in the range of 0.0 - 1.0 inclusive.

 * @param opacity
 */

static void firebolt_surface_setOpacity(struct wl_client *client,
                                                struct wl_resource *resource, wl_fixed_t opacity) {
    RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information, " opacity :%f",wl_fixed_to_double(opacity)); 
}



static void firebolt_Surface_ResourceDestroy(struct wl_resource *resource)
{
    auto *fireboltModule = reinterpret_cast<FireboltSurface*>(wl_resource_get_user_data(resource));

    wl_resource_set_user_data(resource, nullptr);
    delete fireboltModule;
}

/*firebolt_surface bind function*/
void firebolt_surface_bind(struct wl_client *client, void *data, uint32_t version, uint32_t id)
{
    fireboltSurfaceCtx *f_fbSurfaceCtx = (fireboltSurfaceCtx *)data;
    RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information, " firebolt_surface_bind");

    if (NULL == f_fbSurfaceCtx->wlResource)
    {
        RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information, " firebolt_surface id:%d wl_resource_create", id);
        f_fbSurfaceCtx->wlResource= wl_resource_create(client,
                                                        &firebolt_surface_interface,
                                                        std::min<int>(version, 1), id);
        if (!f_fbSurfaceCtx->wlResource)
        {
            RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Error, " firebolt_surface id:%d wl_resource_create - no memory", id);
            wl_client_post_no_memory(client);
        }
        else
        {
            RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information, " firebolt_surface id:%d wl_resource_set_implementation", id);
            wl_resource_set_implementation(f_fbSurfaceCtx->wlResource, &fireboltSurfaceInterfaceImpl, f_fbSurfaceCtx, NULL);
        }
    }
    else
    {
        RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information, " firebolt_surface_bind id:%d already bound!", id);
    }
    return;
}

extern "C"
{
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
                                        " moduleInit: Failed to create memory for fireboltSurfaceCtx");
                    ret = false;
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

        return ret;
    }

    void moduleTerm(WstCompositor *wstComp)
    {
        RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information, " moduleTerm: firebolt_surface extension dummy");
        bfbSurfaceInitialized = false;
    }
}


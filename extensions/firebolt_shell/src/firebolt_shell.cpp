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
#include "firebolt_shell.h"
#include "compositorcontroller.h"

FireboltShell* FireboltShell::mInstance = NULL;
std::mutex FireboltShell::mContextLock;

static void firebolt_shell_get_firebolt_surface(struct wl_client *client,
                    struct wl_resource *resource,
                    int32_t surfaceId,
                    uint32_t type);

/* vtable of firebolt_shell interface implementation */
static const struct firebolt_shell_interface fireboltShellInterfaceImpl = {
                        .get_firebolt_surface = firebolt_shell_get_firebolt_surface
                    };


/**
 * Constructor of the firebolt shell
 *
 */
FireboltShell::FireboltShell()
        :mWstCompositor(NULL), mWlDisplay(NULL), mWlResource(NULL), mWlGlobal(NULL), mWstDisplayName(), mClientNamesMap()
{
    RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
            " firebolt_shell@.FireboltShell: constructor");
}

/**
 * Destructor of the firebolt shell
 */
FireboltShell::~FireboltShell()
{
    RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
            " firebolt_shell@.~FireboltShell: destructor");
}

/** Get the mapped client name from wayland resource
 *
 * @param resource  : wayland resource
 * @param clientName: client name associated with the resource
 */
bool FireboltShell::getClientNameByResource(wl_resource *resource, std::string& clientName)
{
    bool ret = false;

    if (NULL != resource)
    {
        FireboltShell *fbShellCtx = reinterpret_cast<FireboltShell*>(wl_resource_get_user_data(resource));
        if(NULL != fbShellCtx)
        {
            clientName = fbShellCtx->mClientNamesMap.find(resource)->second;
            if (!clientName.empty())
            {
                ret = true;
            }
        }
    }
    return ret;
}

/**
 * create a firebolt surface surface from a given wayland surface
 *
 * Create a firebolt_surface wrapper around wl_surfaces, sent in reply
 * to a get_firebolt_surface request for video surfaces
 *
 * @param id   : Surface id of the app
 * @param type : firebolt_shell_firebolt_surface_type FIREBOLT_SHELL_FIREBOLT_SURFACE_TYPE_*
 */
static void firebolt_shell_get_firebolt_surface(struct wl_client *client,
            				     struct wl_resource *resource,
            				     int32_t surfaceId,
            				     uint32_t type)
{
    RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
                    " firebolt_shell@.get_firebolt_surface: client@%p resource@%p surfaceId:%d type:%u",
                    client, resource, surfaceId, type);

    FireboltShell *fbShellCtx = reinterpret_cast<FireboltShell*>(wl_resource_get_user_data(resource));
    if ((NULL != fbShellCtx) && (NULL != fbShellCtx->mWstCompositor))
    {
        std::string clientName = "";
        if (RdkWindowManager::CompositorController::getClientName(fbShellCtx->mWstCompositor, clientName))
        {
            if (RdkWindowManager::CompositorController::getFireboltSurface(clientName, surfaceId, type))
            {
                /* Notifying event for video surfaces */
                if (type == FIREBOLT_SHELL_FIREBOLT_SURFACE_TYPE_VIDEO)
                {
                    const char *hwVideoSurfaceId = "1"; /* hardcorded for testing purpose */
                    RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Warn,
                                    " firebolt_shell@.event: firebolt_video_surface_id %s", hwVideoSurfaceId);

                    /* Sending hardcoded hw firebolt_video_surface_id for testing purpose */
                    firebolt_shell_send_firebolt_video_surface_id(resource, hwVideoSurfaceId);
                }
            }
            else
            {
                RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Error,
                                " firebolt_shell@.get_firebolt_surface: failed to get firebolt surface"
                                " clientName:%s surfaceId:%d type:%u", clientName, surfaceId, type);
            }
        }
        else
        {
            RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Error,
                            " firebolt_shell@.get_firebolt_surface: Client name not found Instance@%p resource@%p"
                            " surfaceId:%d WstCompositor:%d", fbShellCtx, resource, surfaceId, fbShellCtx->mWstCompositor);
        }
    }
    else
    {
        RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Warn,
                " firebolt_shell@.get_firebolt_surface: resource@%p - instance/compositor not vaild!", resource);

    }
    
    return;
}

/**
 * To destory firebolt_shell interface resource
 *
 */
static void firebolt_shell_resource_destory(struct wl_resource *resource)
{
    RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
            " firebolt_shell@.resource_destory: resource@%p", resource);

    FireboltShell *fbShellCtx = reinterpret_cast<FireboltShell*>(wl_resource_get_user_data(resource));
    if (NULL != fbShellCtx)
    {
        RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
                " firebolt_shell@.resource_destory: instance@%p resource:%p mWlResource:%p",
                fbShellCtx->mInstance, resource, fbShellCtx->mWlResource);

        /* Erase map of resource and client name */
        FireboltShell::ClientNamesMap::iterator it = fbShellCtx->mClientNamesMap.find(resource);
        fbShellCtx->mClientNamesMap.erase(it);
        
        /* To clear resource user data */
        wl_resource_set_user_data(resource, NULL);

        /* resource destroy */
        wl_resource_destroy(resource);
        std::lock_guard<std::mutex> locker(FireboltShell::mContextLock);
        fbShellCtx->mWlResource = NULL;
    }
    else
    {
        RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Warn,
                " firebolt_shell@.resource_destory: resource@%p - instance not vaild!", resource);

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
static void firebolt_shell_bind(struct wl_client *client, void *data, uint32_t version, uint32_t id)
{
    FireboltShell *fbShellCtx = reinterpret_cast<FireboltShell*>(data);
    RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
                            " firebolt_shell@.bind: client@%p data:%p version:%u, id:%u",
                            client, data, version, id);

    if (NULL == fbShellCtx)
    {
        RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Error,
                        " firebolt_shell@.bind: interface instance not valid");
        goto ret_fail;
    }
    else
    {
        std::string clientName = "";
        std::lock_guard<std::mutex> locker(FireboltShell::mContextLock);

        bool found = RdkWindowManager::CompositorController::getClientName(fbShellCtx->mWstCompositor, clientName);
        if (found)
        {
            /* To get westeros compositor object */
            fbShellCtx->mWstDisplayName = WstCompositorGetDisplayName(fbShellCtx->mWstCompositor);
            RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
                    " firebolt_shell@.bind: WstCompositor display name:%s", fbShellCtx->mWstDisplayName.c_str());

            /* To create resource object for firebolt window manager shell extension  */
            fbShellCtx->mWlResource = wl_resource_create(client,
                                                    &firebolt_shell_interface,
                                                    std::min<int>(version, 1), id);
            if (!fbShellCtx->mWlResource)
            {
                RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Error,
                        " firebolt_shell@.bind: id:%d wl_resource_create - no memory", id);
                wl_client_post_no_memory(client);

                goto ret_fail;
            }
            else
            {
                RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
                        " firebolt_wm@.bind: id:%d wl_resource_create resource:%p",
                        id, fbShellCtx->mWlResource);

                /* Map of wl_resource against client name */
                fbShellCtx->mClientNamesMap[fbShellCtx->mWlResource] = clientName;

                RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
                                " firebolt_shell id:%d wl_resource_set_implementation", id);
                wl_resource_set_implementation(fbShellCtx->mWlResource,
                                            &fireboltShellInterfaceImpl,
                                            fbShellCtx,
                                            firebolt_shell_resource_destory);
            }
        }
        else
        {
            RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Error,
                    " firebolt_shell@.bind: id:%d getClientName failed", id);
        }
    }

ret_fail:
    return;
}

extern "C"
{
    static FireboltShell* fireboltShellCreateContext(void)
    {
        std::lock_guard<std::mutex> locker(FireboltShell::mContextLock);
        if (NULL == FireboltShell::mInstance)
        {
            FireboltShell::mInstance = new FireboltShell();
            if (NULL == FireboltShell::mInstance)
            {
                RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Error,
                        " firebolt_shell@.fireboltShellCreateContext: no memory for context object");
            }
            else
            {
                RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
                        " firebolt_shell@.fireboltShellCreateContext instance:%p created", FireboltShell::mInstance);

            }
        }
        return FireboltShell::mInstance;
    }

    static void fireboltShellDeleteContext(void)
    {
        std::lock_guard<std::mutex> locker(FireboltShell::mContextLock);
        if (NULL != FireboltShell::mInstance)
        {
            RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
                    " firebolt_shell@.fireboltShellDeleteContext instance:%p wlGlobal@%p destory",
                    FireboltShell::mInstance, FireboltShell::mInstance->mWlGlobal);

            /* Remove extension global object and destroy it */
            if (NULL != FireboltShell::mInstance->mWlGlobal)
            {
                wl_global_destroy (FireboltShell::mInstance->mWlGlobal);
                FireboltShell::mInstance->mWlGlobal = NULL;
            }

            /* Clear map of resource and client names */
            FireboltShell::mInstance->mClientNamesMap.clear();

            delete(FireboltShell::mInstance);
            FireboltShell::mInstance = NULL;
        }
        return;
    }

    static bool fireboltShellHasContext(void)
    {
        std::lock_guard<std::mutex> locker(FireboltShell::mContextLock);
        return (NULL != FireboltShell::mInstance) ? true : false;
    }

    /**
     * moduleInit of firebolt_shell westeros extension plugin
     *
     * @param wstCompositor : Object of westeros compositor instance
     * @param display       : Object of the wayland display
     */
    bool moduleInit(WstCompositor *wstCompositor, struct wl_display *display)
    {
        bool ret = true;
        RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
                " firebolt_shell@.moduleInit: wstCompositor@%p wlDisplay@%p initializing",
                wstCompositor, display);
        if (!fireboltShellHasContext())
        {
            FireboltShell* fbShellCtx = NULL;
            /* To create a new instance of firebolt shell extension */
            fbShellCtx = fireboltShellCreateContext();
            if (NULL != fbShellCtx)
            {
                fbShellCtx->mWstCompositor   = wstCompositor;
                fbShellCtx->mWlDisplay = display;

                /* Create extension global object */
                fbShellCtx->mWlGlobal = wl_global_create(display,
                                                       &firebolt_shell_interface,
                                                       1, fbShellCtx,
                                                       firebolt_shell_bind);
                if (!fbShellCtx->mWlGlobal)
                {
                    RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Error,
                            " firebolt_shell@.moduleInit: Failed to wl_global_create interface:firebolt_shell");
                    ret = false;
                    goto ret_fail;
                }
                else
                {
                    RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
                            " firebolt_shell@.moduleInit: wstCompositor@%p wlDisplay@%p wlGlobal@%p initialized",
                            fbShellCtx->mWstCompositor,
                            fbShellCtx->mWlDisplay,
                            fbShellCtx->mWlGlobal);
                }
            }
            else
            {
                RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Error,
                        " firebolt_shell@.moduleInit: firebolt_shell extension create failed!");
            }
        }
        else
        {
            RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Warn,
                    " firebolt_shell@.moduleInit: firebolt_shell extension already initialized");
        }

    ret_fail:
        return ret;
    }

    /**
     * moduleTerm of firebolt_shell westeros extension plugin
     *
     * @param wstCompositor : Object of westeros compositor instance
     */
    void moduleTerm(WstCompositor *wstCompositor)
    {
        RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
                " firebolt_shell@.moduleTerm: firebolt_shell extension terminating");

        if (fireboltShellHasContext())
        {
            /* Delete extension context */
            fireboltShellDeleteContext();
        }
        else
        {
            RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Warn,
                    " firebolt_shell@.moduleTerm: firebolt_shell extension not initialized!");
        }
        return;
    }
}


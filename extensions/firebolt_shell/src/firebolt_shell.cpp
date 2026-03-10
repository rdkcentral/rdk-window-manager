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

typedef std::map<WstCompositor*, FireboltShell*> FireboltShellCompositorListMap;
static FireboltShellCompositorListMap f_fireboltShellCompositorList;
std::mutex FireboltShell::mContextLock;
std::shared_ptr<RdkWindowManager::FireboltExtensionEventListener> FireboltShell::mFireboltShellEventListener = nullptr;

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
        :mWstCompositor(NULL), mWlGlobal(NULL), mWlDisplay(NULL), mWstDisplayName(), mClientListMap(), mInstance(NULL)
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

/**
 * Get Firebolt Shell ClientInfo from wayland resource
 *
 * @param resource  : wayland resource
 * @return FireboltShellClientInfo: Pointer to associated clientInfo struct
 */
FireboltShellClientInfo* FireboltShell::getFireboltShellClientInfo(wl_resource *resource)
{
    FireboltShellClientInfo *clientInfo = NULL;

    if (NULL != resource)
    {
        FireboltShell *fbShellCtx = reinterpret_cast<FireboltShell*>(wl_resource_get_user_data(resource));
        if (NULL != fbShellCtx)
        {
            std::lock_guard<std::mutex> locker(FireboltShell::mContextLock);
            FireboltShell::ClientListMap::iterator it = fbShellCtx->mClientListMap.find(resource);
            if (it != fbShellCtx->mClientListMap.end()) 
            {
                clientInfo = reinterpret_cast<FireboltShellClientInfo*>(it->second);
            }
        }
    }
    return clientInfo;
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
        FireboltShellClientInfo *clientInfo = fbShellCtx->getFireboltShellClientInfo(resource);
        if (NULL != clientInfo)
        {
            if (!clientInfo->clientName.empty())
            {
                RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
                        " firebolt_shell@.get_firebolt_surface: mWstCompositor@%p surfaceId:%d getClientName:%s",
                        fbShellCtx->mWstCompositor, surfaceId, clientInfo->clientName.c_str());

                /* Assigning binded client name here */
                clientName.assign(clientInfo->clientName);
            }
            else
            {
                RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Warn,
                        " firebolt_shell@.get_firebolt_surface: mWstCompositor@%p surfaceId:%d name:empty",
                        fbShellCtx->mWstCompositor, surfaceId);

                /* clientInfo doesn't have clientName, so try to query the clientName again */
                if (RdkWindowManager::CompositorController::getClientName(fbShellCtx->mWstCompositor, clientName))
                {
                    if (!clientName.empty())
                    {
                        RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
                                " firebolt_shell@.get_firebolt_surface: getClientName mWstCompositor@%p surfaceId:%d getClientName:%s",
                                fbShellCtx->mWstCompositor, surfaceId, clientName.c_str());

                        /* Update clientName to clientInfo struct if valid */
                        clientInfo->clientName.assign(clientName);
                    }
                    else
                    {
                        RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Error,
                                " firebolt_shell@.get_firebolt_surface: getClientNamem WstCompositor@%p surfaceId:%d getClientName failed",
                                fbShellCtx->mWstCompositor, surfaceId);
                    }
                }
            }

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
                        " clientName:%s surfaceId:%d type:%u", clientName.c_str(), surfaceId, type);
            }
        }
        else
        {
            RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Error,
                    " firebolt_shell@.get_firebolt_surface: Client Info not found instance@%p resource@%p"
                    " surfaceId:%d WstCompositor@%p", fbShellCtx, resource, surfaceId, fbShellCtx->mWstCompositor);
        }
    }
    else
    {
        RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Warn,
                " firebolt_shell@.get_firebolt_surface: resource@%p - instance/compositor not vaild!", resource);

    }
    
    return;
}

void FireboltShell::FireboltShellListener::notify_focus_event(
                                const char* clientName,
                                const std::string& eventName,
                                void (*fbShellEventCallback)(wl_resource*, const char*))
{
    if (!clientName)
    {
        RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Error,
            " firebolt_shell@.notify_focus_event:%s - clientName is NULL", eventName.c_str());
        return;
    }

    std::lock_guard<std::mutex> locker(FireboltShell::mContextLock);
    bool found = false;
    std::string name(clientName);

    for (const auto& compositorEntry : f_fireboltShellCompositorList)
    {
        FireboltShell* fbShellCtx = compositorEntry.second;
        if (!fbShellCtx)
        {
            RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Warn,
                " firebolt_shell@.notify_focus_event:%s context is NULL in compositor list", eventName.c_str());
            continue;
        }

        for (const auto& clientEntry : fbShellCtx->mClientListMap)
        {
            wl_resource* resource = clientEntry.first;
            FireboltShellClientInfo* clientInfo = clientEntry.second;

            if (!clientInfo)
            {
                RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Warn,
                    " firebolt_shell@.notify_focus_event:%s for client '%s' notified to listener resource@%p(clientInfo not found!)",
                    eventName.c_str(), name.c_str(), resource);
            }
            else
            {
                RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
                    " firebolt_shell@.notify_focus_event:%s for client '%s' notified to listener resource@%p(%s)",
                    eventName.c_str(), name.c_str(), resource, clientInfo->clientName.c_str());
            }

            /* Notify all clients */
            fbShellEventCallback(resource, name.c_str());
            found = true;
        }
    }

    if (!found)
    {
        RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Warn,
            " firebolt_shell@.notify_focus_event:%s client '%s' discarded - CompositorList.size:%zu",
            eventName.c_str(), name.c_str(), f_fireboltShellCompositorList.size());
    }
}

void FireboltShell::FireboltShellListener::on_focus(const char* clientName)
{
    notify_focus_event(clientName, RdkWindowManager::RDK_WINDOW_MANAGER_FIREBOLT_EXTENTION_EVENT_ON_FOCUS, firebolt_shell_send_on_focus);
}

void FireboltShell::FireboltShellListener::on_blur(const char* clientName)
{
    notify_focus_event(clientName, RdkWindowManager::RDK_WINDOW_MANAGER_FIREBOLT_EXTENTION_EVENT_ON_BLUR, firebolt_shell_send_on_blur);
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
        std::lock_guard<std::mutex> locker(FireboltShell::mContextLock);
        /* Erase map of resource and client name */
        FireboltShell::ClientListMap::iterator it = fbShellCtx->mClientListMap.find(resource);
        if (it != fbShellCtx->mClientListMap.end()) 
        {
            FireboltShellClientInfo *clientInfo = reinterpret_cast<FireboltShellClientInfo*>(it->second);
            if ((NULL != clientInfo) && (resource == clientInfo->resource))
            {
                /* To clear resource user data */
                wl_resource_set_user_data(clientInfo->resource, NULL);

                RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
                        " firebolt_shell@.resource_destory: instance@%p clientInfo->resource:%p",
                        fbShellCtx->mInstance, clientInfo->resource);

                /* delete client info */
                delete clientInfo;
                it->second = NULL;
            }
            else
            {
                RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Warn,
                        " firebolt_shell@.resource_destory: incorrect"
                        " clientInfo@%p resource@%p clientInfo->resource@%p",
                        clientInfo, resource, (clientInfo ? clientInfo->resource : NULL));
            }
            fbShellCtx->mClientListMap.erase(it);
        }
        else
        {
            RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Error,
                    " firebolt_shell@.resource_destory: resource@%p - client not found!", resource);
        }
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
    RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
            " firebolt_shell@.bind: client@%p data@%p version:%u, id:%u",
            client, data, version, id);

    FireboltShell *fbShellCtx = reinterpret_cast<FireboltShell*>(data);
    if (NULL == fbShellCtx)
    {
        RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Error,
                " firebolt_shell@.bind: interface instance not valid");
        goto ret_fail;
    }
    else
    {
        std::string clientName = "";
        struct wl_resource *resource = NULL;

        std::lock_guard<std::mutex> locker(FireboltShell::mContextLock);

        /* To get westeros compositor object */
        fbShellCtx->mWstDisplayName = WstCompositorGetDisplayName(fbShellCtx->mWstCompositor);
        RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
                " firebolt_shell@.bind: WstCompositor display name:%s", fbShellCtx->mWstDisplayName.c_str());

        /* To create resource object for firebolt shell extension  */
        resource = wl_resource_create(client,
                                    &firebolt_shell_interface,
                                    std::min<int>(version, 1), id);
        if (!resource)
        {
            RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Error,
                    " firebolt_shell@.bind: id:%u wl_resource_create - no memory", id);
            wl_client_post_no_memory(client);

            goto ret_fail;
        }
        else
        {
            RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
                    " firebolt_shell@.bind: id:%u wl_resource_create resource:%p", id, resource);

            /* Map of wl_resource against client Info */
            FireboltShellClientInfo *clientInfo = new FireboltShellClientInfo;
            if (NULL == clientInfo)
            {
                RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Error,
                        " firebolt_shell@.bind: id:%u FireboltShellClientInfo - no memory", id);
                wl_client_post_no_memory(client);

                goto ret_fail;
            }
            else
            {
                if (RdkWindowManager::CompositorController::getClientName(fbShellCtx->mWstCompositor, clientName))
                {
                    RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
                            " firebolt_shell@.bind: mWstCompositor@%p id:%u getClientName:%s",
                            fbShellCtx->mWstCompositor, id, clientName.c_str());
                }
                else
                {
                    RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Error,
                            " firebolt_shell@.bind: mWstCompositor@%p id:%u getClientName failed",
                            fbShellCtx->mWstCompositor, id);
                }

                /* Set client info detail */
                clientInfo->resource    = resource;
                clientInfo->clientId    = id;
                clientInfo->display     = wl_client_get_display(client);
                clientInfo->clientName.assign(clientName);
                fbShellCtx->mClientListMap[resource] = clientInfo;

                RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
                                " firebolt_shell id:%u wl_resource_set_implementation", id);
                wl_resource_set_implementation(resource,
                                            &fireboltShellInterfaceImpl,
                                            fbShellCtx,
                                            firebolt_shell_resource_destory);
            }
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
        FireboltShell *instance = new FireboltShell();
        if (NULL == instance)
        {
            RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Error,
                    " firebolt_shell@.fireboltShellCreateContext: no memory for context object");
        }
        else
        {
            RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
                    " firebolt_shell@.fireboltShellCreateContext: instance@%p created", instance);
            instance->mInstance = instance;
        }

        return instance;
    }

    static void fireboltShellDeleteContext(FireboltShell *fireboltShellContext)
    {
        std::lock_guard<std::mutex> locker(FireboltShell::mContextLock);
        if (NULL != fireboltShellContext->mInstance)
        {
            RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
                    " firebolt_shell@.fireboltShellDeleteContext: instance@%p wlGlobal@%p destory",
                    fireboltShellContext->mInstance, fireboltShellContext->mInstance->mWlGlobal);

            /* Remove extension global object and destroy it */
            if (NULL != fireboltShellContext->mInstance->mWlGlobal)
            {
                wl_global_destroy (fireboltShellContext->mInstance->mWlGlobal);
                fireboltShellContext->mInstance->mWlGlobal = NULL;
            }

            /* Clear map of resource and client Info */
            for (auto it = fireboltShellContext->mInstance->mClientListMap.begin(); it != fireboltShellContext->mInstance->mClientListMap.end(); it++)
            {
                if (it != fireboltShellContext->mInstance->mClientListMap.end())
                {
                    FireboltShellClientInfo *clientInfo = reinterpret_cast<FireboltShellClientInfo*>(it->second);
                    if (NULL != clientInfo)
                    {
                        /* To clear resource user data */
                        wl_resource_set_user_data(clientInfo->resource, NULL);

                        /* delete client info */
                        delete clientInfo;
                        it->second = NULL;
                    }
                }
            }
            fireboltShellContext->mInstance->mClientListMap.clear();

            /* Delete context */
            delete(fireboltShellContext->mInstance);
            fireboltShellContext->mInstance = NULL;
        }
        return;
    }

    static FireboltShell* fireboltShellHasContext(WstCompositor *wstCompositor)
    {
        std::lock_guard<std::mutex> locker(FireboltShell::mContextLock);
        FireboltShell* fbShellCtx = NULL;
        if (NULL != wstCompositor)
        {
            FireboltShellCompositorListMap::iterator it = f_fireboltShellCompositorList.find(wstCompositor);
            if (it != f_fireboltShellCompositorList.end())
            {
                fbShellCtx = reinterpret_cast<FireboltShell*>(it->second);
            }
        }
        RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
                " firebolt_shell@.fireboltShellHasContext wstCompositor@%p fbShellCtx@%p",
                wstCompositor, fbShellCtx);

        return fbShellCtx;
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
                " firebolt_shell@.moduleInit: firebolt_shell extension wstCompositor@%p display@%p initializing",
                wstCompositor, display);

        if ((NULL != wstCompositor) && (NULL == fireboltShellHasContext(wstCompositor)))
        {
            FireboltShell* fbShellCtx = NULL;
            /* To create a new instance of firebolt shell extension */
            fbShellCtx = fireboltShellCreateContext();
            if (NULL != fbShellCtx)
            {
                std::lock_guard<std::mutex> locker(FireboltShell::mContextLock);
                fbShellCtx->mWstCompositor  = wstCompositor;
                fbShellCtx->mWlDisplay      = display;

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
                            " firebolt_shell@.moduleInit: wstCompositor@%p display@%p wlGlobal@%p initialized",
                            fbShellCtx->mWstCompositor,
                            fbShellCtx->mWlDisplay,
                            fbShellCtx->mWlGlobal);
                    f_fireboltShellCompositorList[wstCompositor] = fbShellCtx;

                    /* Register eventlListener callback with CompositorController */
                    if (nullptr == FireboltShell::mFireboltShellEventListener)
                    {
                        FireboltShell::mFireboltShellEventListener = std::make_shared<FireboltShell::FireboltShellListener>();
                        if (false == RdkWindowManager::CompositorController::addFireboltExtensionListener(
                                                                             firebolt_shell_interface.name,
                                                                             FireboltShell::mFireboltShellEventListener))
                        {
                            RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Error,
                                    " firebolt_shell@.moduleInit: CompositorController::addFireboltExtensionListener failed for %s",
                                    firebolt_shell_interface.name);

                            /* Reset listener */
                            FireboltShell::mFireboltShellEventListener.reset();
                        }
                        else
                        {
                            RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,
                                    " firebolt_shell@.moduleInit: CompositorController::addFireboltExtensionListener success for %s",
                                    firebolt_shell_interface.name);
                        }
                    }
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

        FireboltShell *fireboltShellContext = fireboltShellHasContext(wstCompositor);
        if (NULL != fireboltShellContext)
        {
            /* Unregister eventlListener callback with CompositorController */
            if (nullptr == FireboltShell::mFireboltShellEventListener)
            {
                /* Remove event listener */
                RdkWindowManager::CompositorController::removeFireboltExtensionListener(
                                                                firebolt_shell_interface.name,
                                                                FireboltShell::mFireboltShellEventListener);
                /* Reset listener */
                FireboltShell::mFireboltShellEventListener.reset();
            }

            /* Delete extension context */
            fireboltShellDeleteContext(fireboltShellContext);
            f_fireboltShellCompositorList.erase(wstCompositor);
        }
        else
        {
            RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Warn,
                    " firebolt_shell@.moduleTerm: firebolt_shell extension not initialized!");
        }
        return;
    }
}


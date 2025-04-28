/*
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
 
#include <gmock/gmock.h>
#include "wayland-server.h"
#include "wayland-server-core.h"
#include "wayland-server-coreImpl.h"
#include "firebolt_wm_protocol_server.h"


WaylandServerImpl* WaylandServer::impl = nullptr;

WaylandServer::WaylandServer() {}

void WaylandServer::setImpl(WaylandServerImpl* newImpl)
{
    ASSERT_TRUE(((nullptr == impl) && (nullptr != newImpl)) || ((nullptr != impl) && (nullptr == newImpl)));
    impl = newImpl;
}


void WaylandServer::wl_resource_post_event(struct wl_resource *resource, uint32_t opcode, ...) 
{
  
    assert(nullptr != impl);

    va_list args;
    va_start(args, opcode);

    // Handle different opcodes and call the appropriate mock method
    switch (opcode) {
        case FIREBOLT_WM_CLIENT_PROPERTIES: {
            const char *id = va_arg(args, const char*);
            int32_t x = va_arg(args, int32_t);
            int32_t y = va_arg(args, int32_t);
            uint32_t width = va_arg(args, uint32_t);
            uint32_t height = va_arg(args, uint32_t);
            wl_fixed_t opacity = va_arg(args, wl_fixed_t);
            int32_t zorder = va_arg(args, int32_t);
            int32_t visible = va_arg(args, int32_t);
            wl_fixed_t crop_x = va_arg(args, wl_fixed_t);
            wl_fixed_t crop_y = va_arg(args, wl_fixed_t);
            wl_fixed_t crop_width = va_arg(args, wl_fixed_t);
            wl_fixed_t crop_height = va_arg(args, wl_fixed_t);
            int32_t textured = va_arg(args, int32_t);

            // Call the mock method for client properties
            impl->wl_resource_post_event_for_client_properties(
                resource, opcode, id, x, y, width, height, opacity, zorder, visible, crop_x, crop_y, crop_width, crop_height, textured
            );
            break;
        }

        case FIREBOLT_WM_FOCUSED_CLIENT: {
            const char *id = va_arg(args, const char*);
            // Call the mock method for focused client
            impl->wl_resource_post_event_for_focused_client(resource, opcode, id);
            break;
        }

        case FIREBOLT_WM_CLIENTS: {
            const char *id = va_arg(args, const char*);
            // Call the mock method for generic client
            impl->wl_resource_post_event_for_client(resource, id);
            break;
        }

        default:
            // Handle any other opcodes or invalid ones
            break;
    }

    va_end(args);
}

void * WaylandServer::wl_resource_get_user_data(struct wl_resource *resource)
{
	assert(nullptr != impl);
	return impl->wl_resource_get_user_data(resource);
}
void
WaylandServer::wl_resource_set_user_data(struct wl_resource *resource, void *data)
{
	assert(nullptr != impl);
	impl->wl_resource_set_user_data(resource, data);
}

struct wl_resource *
WaylandServer::wl_resource_create(struct wl_client *client,
		   const struct wl_interface *interface,
		   int version, uint32_t id)
{
	assert(nullptr != impl);
	return impl->wl_resource_create(client, interface, version, id);
}
 void
WaylandServer::wl_client_post_no_memory(struct wl_client *client)
{
	assert(nullptr != impl);
	impl->wl_client_post_no_memory(client);
}
 struct wl_display *
WaylandServer::wl_client_get_display(struct wl_client *client)
{
	assert(nullptr != impl);
	return impl->wl_client_get_display(client);
}

void
WaylandServer::wl_resource_set_implementation(struct wl_resource *resource,
			       const void *implementation,
			       void *data, wl_resource_destroy_func_t destroy)
{
	assert(nullptr != impl);
	impl->wl_resource_set_implementation(resource, implementation, data, destroy);
}
void
WaylandServer::wl_global_destroy(struct wl_global *global)
{
	assert(nullptr != impl);
	impl->wl_global_destroy(global);
}
struct wl_global *
WaylandServer::wl_global_create(struct wl_display *display,
		 const struct wl_interface *interface, int version,
		 void *data, wl_global_bind_func_t bind)
{
	assert(nullptr != impl);
	return impl->wl_global_create(display, interface, version, data, bind);
}


/*void (*wl_resource_post_event)(struct wl_resource *resource_, uint32_t event, const char *id, int32_t x, int32_t y, uint32_t width, uint32_t height, wl_fixed_t opacity, int32_t zorder, int32_t visible, wl_fixed_t crop_x, wl_fixed_t crop_y, wl_fixed_t crop_width, wl_fixed_t crop_height, int32_t textured)= &WaylandServer::wl_resource_post_event_for_client_properties;
void (*wl_resource_post_event)(struct wl_resource *resource, const char *id) = &WaylandServer::wl_resource_post_event_for_client;
void (*wl_resource_post_event)(struct wl_resource *resource, uint32_t opcode, const char *id) = &WaylandServer::wl_resource_post_event_for_focused_client;*/
void (*wl_resource_post_event)(struct wl_resource *resource, uint32_t opcode, ...) = &WaylandServer::wl_resource_post_event;

void* (*wl_resource_get_user_data)(struct wl_resource *resource) = &WaylandServer::wl_resource_get_user_data;
void (*wl_resource_set_user_data)(struct wl_resource *resource, void *data) = &WaylandServer::wl_resource_set_user_data;
struct wl_resource* (*wl_resource_create)(struct wl_client *client,
                                              const struct wl_interface *interface,
                                              int version, uint32_t id) = &WaylandServer::wl_resource_create;
void (*wl_client_post_no_memory)(struct wl_client *client) = &WaylandServer::wl_client_post_no_memory;
struct wl_display* (*wl_client_get_display)(struct wl_client *client) = &WaylandServer::wl_client_get_display;
void (*wl_resource_set_implementation)(struct wl_resource *resource,
                                           const void *implementation,
                                           void *data, wl_resource_destroy_func_t destroy) = &WaylandServer::wl_resource_set_implementation;
void (*wl_global_destroy)(struct wl_global *global) = &WaylandServer::wl_global_destroy;
struct wl_global* (*wl_global_create)(struct wl_display *display,
                                          const struct wl_interface *interface, int version,
                                          void *data, wl_global_bind_func_t bind) = &WaylandServer::wl_global_create;

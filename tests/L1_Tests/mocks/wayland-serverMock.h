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
#include "wayland-server-coreImpl.h"

class WaylandServerMockImpl : public WaylandServerImpl
{
    public:
    MOCK_METHOD(void, wl_resource_post_event_for_client_properties, (struct wl_resource *resource_, uint32_t event, const char *id, int32_t x, int32_t y, uint32_t width, uint32_t height, wl_fixed_t opacity, int32_t zorder, int32_t visible, wl_fixed_t crop_x, wl_fixed_t crop_y, wl_fixed_t crop_width, wl_fixed_t crop_height, int32_t textured), (override));
    MOCK_METHOD(void, wl_resource_post_event_for_client, (struct wl_resource *resource, const char *id), (override));
    MOCK_METHOD(void, wl_resource_post_event_for_focused_client, (struct wl_resource *resource, uint32_t opcode, const char *id), (override));
    MOCK_METHOD(void*, wl_resource_get_user_data, (struct wl_resource *resource), (override));
    MOCK_METHOD(void, wl_resource_set_user_data, (struct wl_resource *resource, void *data), (override));
    MOCK_METHOD(struct wl_resource*, wl_resource_create, (struct wl_client *client, const struct wl_interface *interface, int version, uint32_t id), (override));
    MOCK_METHOD(void, wl_client_post_no_memory, (struct wl_client *client), (override));
    MOCK_METHOD(struct wl_display*, wl_client_get_display, (struct wl_client *client), (override));
    MOCK_METHOD(void, wl_resource_set_implementation, (struct wl_resource *resource, const void *implementation, void *data, wl_resource_destroy_func_t destroy), (override));
    MOCK_METHOD(void, wl_global_destroy, (struct wl_global *global), (override));
    MOCK_METHOD(struct wl_global*, wl_global_create, (struct wl_display *display, const struct wl_interface *interface, int version, void *data, wl_global_bind_func_t bind), (override));
};

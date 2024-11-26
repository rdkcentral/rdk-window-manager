/*
 * If not stated otherwise in this file or this component's Licenses.txt file the
 * following copyright and licenses apply:
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

/** \file
 *
 *  \brief Include the server API, deprecations and protocol C API.
 *
 *  \warning Use of this header file is discouraged. Prefer including
 *  wayland-server-core.h instead, which does not include the
 *  server protocol header and as such only defines the library
 *  API, excluding the deprecated API below.
 */

#ifndef WAYLAND_SERVER_H
#define WAYLAND_SERVER_H

#include <stdint.h>
#include "wayland-server-core.h"

#ifdef  __cplusplus
extern "C" {
#endif

/*
 * The user can set this macro to hide the wl_object, wl_resource and wl_buffer
 * objects alongside the associated API.
 *
 * The structs were meant to be opaque, although we missed that in the early days.
 *
 * NOTE: the list of structs, functions, etc in this section MUST NEVER GROW.
 * Otherwise we will break forward compatibility and applications that used to
 * build fine will no longer be able to do so.
 */
#ifndef WL_HIDE_DEPRECATED

struct wl_object {
	const struct wl_interface *interface;
	const void *implementation;
	uint32_t id;
};

struct wl_resource {
	struct wl_object object;
	wl_resource_destroy_func_t destroy;
	struct wl_list link;
	struct wl_signal destroy_signal;
	struct wl_client *client;
	void *data;
};

uint32_t
wl_client_add_resource(struct wl_client *client,
		       struct wl_resource *resource) WL_DEPRECATED;

struct wl_resource *
wl_client_add_object(struct wl_client *client,
		     const struct wl_interface *interface,
		     const void *implementation,
		     uint32_t id, void *data) WL_DEPRECATED;

struct wl_resource *
wl_client_new_object(struct wl_client *client,
		     const struct wl_interface *interface,
		     const void *implementation, void *data) WL_DEPRECATED;

struct wl_global *
wl_display_add_global(struct wl_display *display,
		      const struct wl_interface *interface,
		      void *data,
		      wl_global_bind_func_t bind) WL_DEPRECATED;

void
wl_display_remove_global(struct wl_display *display,
			 struct wl_global *global) WL_DEPRECATED;

#endif

#ifdef  __cplusplus
}
#endif

#include "wayland-server-protocol.h"

#endif

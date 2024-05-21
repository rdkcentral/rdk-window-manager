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

#ifndef FIREBOLT_SHELL_SERVER_PROTOCOL_H
#define FIREBOLT_SHELL_SERVER_PROTOCOL_H

#include <stdint.h>
#include <stddef.h>
#include "wayland-server.h"

#ifdef  __cplusplus
extern "C" {
#endif

struct wl_client;
struct wl_resource;

/**
 * @page page_firebolt_shell The firebolt_shell protocol
 * @section page_ifaces_firebolt_shell Interfaces
 * - @subpage page_iface_firebolt_shell - 
 */
struct firebolt_shell;
struct firebolt_surface;
struct wl_surface;

/**
 * @page page_iface_firebolt_shell firebolt_shell
 * @section page_iface_firebolt_shell_desc Description
 *
 * Enhance westeros to support the new shell APIs.
 * @section page_iface_firebolt_shell_api API
 * See @ref iface_firebolt_shell.
 */
/**
 * @defgroup iface_firebolt_shell The firebolt_shell interface
 *
 * Enhance westeros to support the new shell APIs.
 */
extern const struct wl_interface firebolt_shell_interface;

#ifndef FIREBOLT_SHELL_FIREBOLT_SURFACE_TYPE_ENUM
#define FIREBOLT_SHELL_FIREBOLT_SURFACE_TYPE_ENUM
/**
 * @ingroup iface_firebolt_shell
 * surface types
 *
 * Describes the surface type and passed to the get_firebolt_surface
 */
enum firebolt_shell_firebolt_surface_type {
	/**
	 * a regular surface
	 */
	FIREBOLT_SHELL_FIREBOLT_SURFACE_TYPE_STANDARD = 1,
	/**
	 * a video surface
	 */
	FIREBOLT_SHELL_FIREBOLT_SURFACE_TYPE_VIDEO = 2,
	/**
	 * Display on top of all other surfaces.  Does not have input focus.
	 */
	FIREBOLT_SHELL_FIREBOLT_SURFACE_TYPE_POPUP = 3,
	/**
	 * Display on top of all other surfaces.  Receives input focus.
	 */
	FIREBOLT_SHELL_FIREBOLT_SURFACE_TYPE_NOTIFICATION = 4,
};
#endif /* FIREBOLT_SHELL_FIREBOLT_SURFACE_TYPE_ENUM */

/**
 * @ingroup iface_firebolt_shell
 * @struct firebolt_shell_interface
 */
struct firebolt_shell_interface {
	/**
	 * create a firebolt shell surface from a surface
	 *
	 * Create a firebolt_surface wrapper around wl_surfaces
	 */
	void (*get_firebolt_surface)(struct wl_client *client,
				     struct wl_resource *resource,
				     uint32_t id,
				     struct wl_resource *surface,
				     uint32_t type);
};

#define FIREBOLT_SHELL_FIREBOLT_VIDEO_SURFACE_ID 0

/**
 * @ingroup iface_firebolt_shell
 */
#define FIREBOLT_SHELL_FIREBOLT_VIDEO_SURFACE_ID_SINCE_VERSION 1

/**
 * @ingroup iface_firebolt_shell
 */
#define FIREBOLT_SHELL_GET_FIREBOLT_SURFACE_SINCE_VERSION 1

/**
 * @ingroup iface_firebolt_shell
 * Sends an firebolt_video_surface_id event to the client owning the resource.
 * @param resource_ The client's resource
 * @param video_id the video id for identifying the hardware video surface
 */
static inline void
firebolt_shell_send_firebolt_video_surface_id(struct wl_resource *resource_, const char *video_id)
{
	wl_resource_post_event(resource_, FIREBOLT_SHELL_FIREBOLT_VIDEO_SURFACE_ID, video_id);
}

#ifdef  __cplusplus
}
#endif

#endif

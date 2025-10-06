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

#ifndef FIREBOLT_SHELL_CLIENT_PROTOCOL_H
#define FIREBOLT_SHELL_CLIENT_PROTOCOL_H

#include <stdint.h>
#include <stddef.h>
#include "wayland-client.h"

#ifdef  __cplusplus
extern "C" {
#endif

/**
 * @page page_firebolt_shell The firebolt_shell protocol
 * @section page_ifaces_firebolt_shell Interfaces
 * - @subpage page_iface_firebolt_shell - 
 */
struct firebolt_shell;

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
 * @struct firebolt_shell_listener
 */
struct firebolt_shell_listener {
	/**
	 * sent in reply to a get_firebolt_surface request for video surfaces
	 *
	 * Returns a video id string if a video surface is requested from
	 * the get_firebolt_surface request
	 * @param video_id the video id for identifying the hardware video surface
	 */
	void (*firebolt_video_surface_id)(void *data,
					  struct firebolt_shell *firebolt_shell,
					  const char *video_id);
	/**
	 * sent when a display is focused
	 *
	 * Notify on_focus event when a display is focused
	 */
	void (*on_focus)(void *data,
			 struct firebolt_shell *firebolt_shell,
			 const char *client_id);
	/**
	 * sent when a display is blurred
	 *
	 * Notify on_blur event when a display is blurred
	 */
	void (*on_blur)(void *data,
			struct firebolt_shell *firebolt_shell,
			const char *client_id);
};

/**
 * @ingroup iface_firebolt_shell
 */
static inline int
firebolt_shell_add_listener(struct firebolt_shell *firebolt_shell,
			    const struct firebolt_shell_listener *listener, void *data)
{
	return wl_proxy_add_listener((struct wl_proxy *) firebolt_shell,
				     (void (**)(void)) listener, data);
}

#define FIREBOLT_SHELL_GET_FIREBOLT_SURFACE 0

/**
 * @ingroup iface_firebolt_shell
 */
#define FIREBOLT_SHELL_FIREBOLT_VIDEO_SURFACE_ID_SINCE_VERSION 1
/**
 * @ingroup iface_firebolt_shell
 */
#define FIREBOLT_SHELL_ON_FOCUS_SINCE_VERSION 1
/**
 * @ingroup iface_firebolt_shell
 */
#define FIREBOLT_SHELL_ON_BLUR_SINCE_VERSION 1

/**
 * @ingroup iface_firebolt_shell
 */
#define FIREBOLT_SHELL_GET_FIREBOLT_SURFACE_SINCE_VERSION 1

/** @ingroup iface_firebolt_shell */
static inline void
firebolt_shell_set_user_data(struct firebolt_shell *firebolt_shell, void *user_data)
{
	wl_proxy_set_user_data((struct wl_proxy *) firebolt_shell, user_data);
}

/** @ingroup iface_firebolt_shell */
static inline void *
firebolt_shell_get_user_data(struct firebolt_shell *firebolt_shell)
{
	return wl_proxy_get_user_data((struct wl_proxy *) firebolt_shell);
}

static inline uint32_t
firebolt_shell_get_version(struct firebolt_shell *firebolt_shell)
{
	return wl_proxy_get_version((struct wl_proxy *) firebolt_shell);
}

/** @ingroup iface_firebolt_shell */
static inline void
firebolt_shell_destroy(struct firebolt_shell *firebolt_shell)
{
	wl_proxy_destroy((struct wl_proxy *) firebolt_shell);
}

/**
 * @ingroup iface_firebolt_shell
 *
 * Create a firebolt_surface wrapper around wl_surfaces
 */
static inline void
firebolt_shell_get_firebolt_surface(struct firebolt_shell *firebolt_shell, int32_t surfaceId, uint32_t type)
{
	wl_proxy_marshal((struct wl_proxy *) firebolt_shell,
			 FIREBOLT_SHELL_GET_FIREBOLT_SURFACE, surfaceId, type);
}

#ifdef  __cplusplus
}
#endif

#endif

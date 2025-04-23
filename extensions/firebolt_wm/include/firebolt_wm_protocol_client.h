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

#ifndef FIREBOLT_WM_CLIENT_PROTOCOL_H
#define FIREBOLT_WM_CLIENT_PROTOCOL_H

#include <stdint.h>
#include <stddef.h>
#include "wayland-client.h"

#ifdef  __cplusplus
extern "C" {
#endif

/**
 * @page page_firebolt_wm The firebolt_wm protocol
 * @section page_ifaces_firebolt_wm Interfaces
 * - @subpage page_iface_firebolt_wm - 
 */
struct firebolt_wm;

/**
 * @page page_iface_firebolt_wm firebolt_wm
 * @section page_iface_firebolt_wm_api API
 * See @ref iface_firebolt_wm.
 */
/**
 * @defgroup iface_firebolt_wm The firebolt_wm interface
 */
extern const struct wl_interface firebolt_wm_interface;

/**
 * @ingroup iface_firebolt_wm
 * @struct firebolt_wm_listener
 */
struct firebolt_wm_listener {
	/**
	 * @param id Id of the app
	 * @param x the left position of the surface in pixel screen coordinates
	 * @param y the top position of the surface in pixel screen coordinates
	 * @param width the width of the graphics surface in pixel screen coordinates
	 * @param height the height of the graphics surface in pixel screen coordinates
	 * @param opacity opacticty factor
	 * @param zorder location in the z-order
	 * @param visible the visibility of the surface
	 * @param crop_x the cropping left insert
	 * @param crop_y the cropping top insert
	 * @param crop_width the cropping width
	 * @param crop_height the cropping height
	 * @param textured boolean for whether should use textured graphics (fbo or video) or not
	 */
	void (*client_properties)(void *data,
				  struct firebolt_wm *firebolt_wm,
				  const char *id,
				  int32_t x,
				  int32_t y,
				  uint32_t width,
				  uint32_t height,
				  wl_fixed_t opacity,
				  int32_t zorder,
				  int32_t visible,
				  wl_fixed_t crop_x,
				  wl_fixed_t crop_y,
				  wl_fixed_t crop_width,
				  wl_fixed_t crop_height,
				  int32_t textured);
	/**
	 * sent in reply to a get_focused_client request
	 *
	 * Returns the focused client id. Empty string means no focused
	 * client
	 * @param id focused client id
	 */
	void (*focused_client)(void *data,
			       struct firebolt_wm *firebolt_wm,
			       const char *id);
	/**
	 * sent in reply to a get_clients request
	 *
	 * Returns list of clients
	 * @param id focused client id
	 */
	void (*clients)(void *data,
			struct firebolt_wm *firebolt_wm,
			const char *id);
};

/**
 * @ingroup iface_firebolt_wm
 */
static inline int
firebolt_wm_add_listener(struct firebolt_wm *firebolt_wm,
			 const struct firebolt_wm_listener *listener, void *data)
{
	return wl_proxy_add_listener((struct wl_proxy *) firebolt_wm,
				     (void (**)(void)) listener, data);
}

#define FIREBOLT_WM_SET_PROPERTIES 0
#define FIREBOLT_WM_CREATE 1
#define FIREBOLT_WM_CREATE_WITH_BOUNDS 2
#define FIREBOLT_WM_CREATE_WITH_PROPERTIES 3
#define FIREBOLT_WM_DESTROY 4
#define FIREBOLT_WM_SET_CLIENT_BOUNDS 5
#define FIREBOLT_WM_SET_CLIENT_DISPLAY_BOUNDS 6
#define FIREBOLT_WM_SET_CLIENT_FOCUS 7
#define FIREBOLT_WM_GET_PROPERTIES 8
#define FIREBOLT_WM_GET_FOCUSED_CLIENT 9
#define FIREBOLT_WM_GET_CLIENTS 10
#define FIREBOLT_WM_SET_OWNER 11

/**
 * @ingroup iface_firebolt_wm
 */
#define FIREBOLT_WM_CLIENT_PROPERTIES_SINCE_VERSION 1
/**
 * @ingroup iface_firebolt_wm
 */
#define FIREBOLT_WM_FOCUSED_CLIENT_SINCE_VERSION 1
/**
 * @ingroup iface_firebolt_wm
 */
#define FIREBOLT_WM_CLIENTS_SINCE_VERSION 1

/**
 * @ingroup iface_firebolt_wm
 */
#define FIREBOLT_WM_SET_PROPERTIES_SINCE_VERSION 1
/**
 * @ingroup iface_firebolt_wm
 */
#define FIREBOLT_WM_CREATE_SINCE_VERSION 1
/**
 * @ingroup iface_firebolt_wm
 */
#define FIREBOLT_WM_CREATE_WITH_BOUNDS_SINCE_VERSION 1
/**
 * @ingroup iface_firebolt_wm
 */
#define FIREBOLT_WM_CREATE_WITH_PROPERTIES_SINCE_VERSION 1
/**
 * @ingroup iface_firebolt_wm
 */
#define FIREBOLT_WM_DESTROY_SINCE_VERSION 1
/**
 * @ingroup iface_firebolt_wm
 */
#define FIREBOLT_WM_SET_CLIENT_BOUNDS_SINCE_VERSION 1
/**
 * @ingroup iface_firebolt_wm
 */
#define FIREBOLT_WM_SET_CLIENT_DISPLAY_BOUNDS_SINCE_VERSION 1
/**
 * @ingroup iface_firebolt_wm
 */
#define FIREBOLT_WM_SET_CLIENT_FOCUS_SINCE_VERSION 1
/**
 * @ingroup iface_firebolt_wm
 */
#define FIREBOLT_WM_GET_PROPERTIES_SINCE_VERSION 1
/**
 * @ingroup iface_firebolt_wm
 */
#define FIREBOLT_WM_GET_FOCUSED_CLIENT_SINCE_VERSION 1
/**
 * @ingroup iface_firebolt_wm
 */
#define FIREBOLT_WM_GET_CLIENTS_SINCE_VERSION 1
/**
 * @ingroup iface_firebolt_wm
 */
#define FIREBOLT_WM_SET_OWNER_SINCE_VERSION 1

/** @ingroup iface_firebolt_wm */
static inline void
firebolt_wm_set_user_data(struct firebolt_wm *firebolt_wm, void *user_data)
{
	wl_proxy_set_user_data((struct wl_proxy *) firebolt_wm, user_data);
}

/** @ingroup iface_firebolt_wm */
static inline void *
firebolt_wm_get_user_data(struct firebolt_wm *firebolt_wm)
{
	return wl_proxy_get_user_data((struct wl_proxy *) firebolt_wm);
}

static inline uint32_t
firebolt_wm_get_version(struct firebolt_wm *firebolt_wm)
{
	return wl_proxy_get_version((struct wl_proxy *) firebolt_wm);
}

/**
 * @ingroup iface_firebolt_wm
 */
static inline void
firebolt_wm_set_properties(struct firebolt_wm *firebolt_wm, const char *id, int32_t x, int32_t y, uint32_t width, uint32_t height, uint32_t render_width, uint32_t render_height, wl_fixed_t opacity, int32_t zorder, int32_t visible, wl_fixed_t crop_x, wl_fixed_t crop_y, wl_fixed_t crop_width, wl_fixed_t crop_height)
{
	wl_proxy_marshal((struct wl_proxy *) firebolt_wm,
			 FIREBOLT_WM_SET_PROPERTIES, id, x, y, width, height, render_width, render_height, opacity, zorder, visible, crop_x, crop_y, crop_width, crop_height);
}

/**
 * @ingroup iface_firebolt_wm
 *
 * Defaults:
 * x, y = 0 Width, height, display width,
 * display height = device resolution Opacity = 1.0
 * Visible = false Z-order = topmost + 1 crop_x, crop_y = 0
 * crop_width, crop_height = 0.0
 */
static inline void
firebolt_wm_create(struct firebolt_wm *firebolt_wm, const char *id)
{
	wl_proxy_marshal((struct wl_proxy *) firebolt_wm,
			 FIREBOLT_WM_CREATE, id);
}

/**
 * @ingroup iface_firebolt_wm
 */
static inline void
firebolt_wm_create_with_bounds(struct firebolt_wm *firebolt_wm, const char *id, int32_t x, int32_t y, uint32_t width, uint32_t height)
{
	wl_proxy_marshal((struct wl_proxy *) firebolt_wm,
			 FIREBOLT_WM_CREATE_WITH_BOUNDS, id, x, y, width, height);
}

/**
 * @ingroup iface_firebolt_wm
 */
static inline void
firebolt_wm_create_with_properties(struct firebolt_wm *firebolt_wm, const char *id, int32_t x, int32_t y, uint32_t width, uint32_t height, uint32_t display_width, uint32_t display_height, wl_fixed_t opacity, int32_t zorder, int32_t visible, wl_fixed_t crop_x, wl_fixed_t crop_y, wl_fixed_t crop_width, wl_fixed_t crop_height, int32_t focused)
{
	wl_proxy_marshal((struct wl_proxy *) firebolt_wm,
			 FIREBOLT_WM_CREATE_WITH_PROPERTIES, id, x, y, width, height, display_width, display_height, opacity, zorder, visible, crop_x, crop_y, crop_width, crop_height, focused);
}

/**
 * @ingroup iface_firebolt_wm
 */
static inline void
firebolt_wm_destroy(struct firebolt_wm *firebolt_wm, const char *id)
{
	wl_proxy_marshal((struct wl_proxy *) firebolt_wm,
			 FIREBOLT_WM_DESTROY, id);

	wl_proxy_destroy((struct wl_proxy *) firebolt_wm);
}

/**
 * @ingroup iface_firebolt_wm
 *
 * Sets client window bounds for rendering.  The client will be rendered inside these bounds without a change to its Wayland display size
 */
static inline void
firebolt_wm_set_client_bounds(struct firebolt_wm *firebolt_wm, const char *id, int32_t x, int32_t y, uint32_t width, uint32_t height)
{
	wl_proxy_marshal((struct wl_proxy *) firebolt_wm,
			 FIREBOLT_WM_SET_CLIENT_BOUNDS, id, x, y, width, height);
}

/**
 * @ingroup iface_firebolt_wm
 *
 * Sets client display window bounds. This will change the size of a clients Wayland display
 */
static inline void
firebolt_wm_set_client_display_bounds(struct firebolt_wm *firebolt_wm, const char *id, uint32_t width, uint32_t height)
{
	wl_proxy_marshal((struct wl_proxy *) firebolt_wm,
			 FIREBOLT_WM_SET_CLIENT_DISPLAY_BOUNDS, id, width, height);
}

/**
 * @ingroup iface_firebolt_wm
 *
 */
static inline void
firebolt_wm_set_client_focus(struct firebolt_wm *firebolt_wm, const char *id)
{
	wl_proxy_marshal((struct wl_proxy *) firebolt_wm,
			 FIREBOLT_WM_SET_CLIENT_FOCUS, id);
}

/**
 * @ingroup iface_firebolt_wm
 */
static inline void
firebolt_wm_get_properties(struct firebolt_wm *firebolt_wm, const char *id)
{
	wl_proxy_marshal((struct wl_proxy *) firebolt_wm,
			 FIREBOLT_WM_GET_PROPERTIES, id);
}

/**
 * @ingroup iface_firebolt_wm
 */
static inline void
firebolt_wm_get_focused_client(struct firebolt_wm *firebolt_wm)
{
	wl_proxy_marshal((struct wl_proxy *) firebolt_wm,
			 FIREBOLT_WM_GET_FOCUSED_CLIENT);
}

/**
 * @ingroup iface_firebolt_wm
 */
static inline void
firebolt_wm_get_clients(struct firebolt_wm *firebolt_wm)
{
	wl_proxy_marshal((struct wl_proxy *) firebolt_wm,
			 FIREBOLT_WM_GET_CLIENTS);
}

/**
 * @ingroup iface_firebolt_wm
 *
 */
static inline void
firebolt_wm_set_owner(struct firebolt_wm *firebolt_wm, const char *id, int32_t owner)
{
	wl_proxy_marshal((struct wl_proxy *) firebolt_wm,
			 FIREBOLT_WM_SET_OWNER, id, owner);
}

#ifdef  __cplusplus
}
#endif

#endif

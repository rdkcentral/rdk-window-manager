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

#ifndef FIREBOLT_WM_SERVER_PROTOCOL_H
#define FIREBOLT_WM_SERVER_PROTOCOL_H

#include <stdint.h>
#include <stddef.h>
#include "wayland-server.h"

#ifdef  __cplusplus
extern "C" {
#endif

struct wl_client;
struct wl_resource;

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
 * @struct firebolt_wm_interface
 */
struct firebolt_wm_interface {
	/**
	 * @param id id of the app or group
	 * @param x the left position of the surface in pixel screen coordinates
	 * @param y the top position of the surface in pixel screen coordinates
	 * @param width the width of the graphics surface in pixel screen coordinates
	 * @param height the height of the graphics surface in pixel screen coordinates
	 * @param render_width the width of the graphics rendering
	 * @param render_height the height of the graphics surface in pixel screen coordinates
	 * @param opacity opacity factor
	 * @param zorder location in the z-order
	 * @param visible the visibility of the surface
	 * @param crop_x the cropping left insert (before scale?)
	 * @param crop_y the cropping top insert (before scale?)
	 * @param crop_width the cropping width (before scale?)
	 * @param crop_height the cropping height (before scale?)
	 */
	void (*set_properties)(struct wl_client *client,
			       struct wl_resource *resource,
			       const char *id,
			       int32_t x,
			       int32_t y,
			       uint32_t width,
			       uint32_t height,
			       uint32_t render_width,
			       uint32_t render_height,
			       wl_fixed_t opacity,
			       int32_t zorder,
			       int32_t visible,
			       wl_fixed_t crop_x,
			       wl_fixed_t crop_y,
			       wl_fixed_t crop_width,
			       wl_fixed_t crop_height);
	/**
	 * set surface opacity
	 *
	 * Defaults: x, y = 0 Width, height, display width, display
	 * height = device resolution Opacity = 1.0 Visible = false Z-order
	 * = topmost + 1 Crop_x, crop_y = 0 Crop_width, crop_height = 0.0
	 * @param id id of the app or group
	 */
	void (*create)(struct wl_client *client,
		       struct wl_resource *resource,
		       const char *id);
	/**
	 * @param id id of the app or group
	 * @param x the left position of the surface in pixel screen coordinates
	 * @param y the top position of the surface in pixel screen coordinates
	 * @param width the width of the graphics surface in pixel screen coordinates
	 * @param height the height of the graphics surface in pixel screen coordinates
	 */
	void (*create_with_bounds)(struct wl_client *client,
				   struct wl_resource *resource,
				   const char *id,
				   int32_t x,
				   int32_t y,
				   uint32_t width,
				   uint32_t height);
	/**
	 * @param id id of the app or group
	 * @param x the left position of the surface in pixel screen coordinates
	 * @param y the top position of the surface in pixel screen coordinates
	 * @param width the width of the graphics surface in pixel screen coordinates
	 * @param height the height of the graphics surface in pixel screen coordinates
	 * @param display_width the width of the Waylands display
	 * @param display_height the height of the Waylands display 
	 * @param opacity opacity factor
	 * @param zorder location in the z-order
	 * @param visible the visibility of the surface
	 * @param crop_x the cropping left insert
	 * @param crop_y the cropping top insert
	 * @param crop_width the cropping width
	 * @param crop_height the cropping height
	 * @param focused focused
	 */
	void (*create_with_properties)(struct wl_client *client,
				       struct wl_resource *resource,
				       const char *id,
				       int32_t x,
				       int32_t y,
				       uint32_t width,
				       uint32_t height,
				       uint32_t display_width,
				       uint32_t display_height,
				       wl_fixed_t opacity,
				       int32_t zorder,
				       int32_t visible,
				       wl_fixed_t crop_x,
				       wl_fixed_t crop_y,
				       wl_fixed_t crop_width,
				       wl_fixed_t crop_height,
				       int32_t focused);
	/**
	 * @param id id of the app or group
	 */
	void (*destroy)(struct wl_client *client,
			struct wl_resource *resource,
			const char *id);
	/**
	 * sets client window bounds
	 *
	 * Sets client window bounds for rendering. The client will be
	 * rendered inside these bounds without a change to its Wayland
	 * display size
	 * @param id Id of the app
	 * @param x the left position of the surface in pixel screen coordinates
	 * @param y the top position of the surface in pixel screen coordinates
	 * @param width the width of the graphics surface in pixel screen coordinates
	 * @param height the height of the graphics surface in pixel screen coordinates
	 */
	void (*set_client_bounds)(struct wl_client *client,
				  struct wl_resource *resource,
				  const char *id,
				  int32_t x,
				  int32_t y,
				  uint32_t width,
				  uint32_t height);
	/**
	 * sets client display window bounds
	 *
	 * Sets client display window bounds. This will change the size of a
	 * clients Wayland display
	 * @param id Id of the app
	 * @param width the width of an apps Wayland display
	 * @param height the height of an apps Wayland display
	 */
	void (*set_client_display_bounds)(struct wl_client *client,
					  struct wl_resource *resource,
					  const char *id,
					  uint32_t width,
					  uint32_t height);
	/**
	 * sets a client to be the focused app
	 *
	 * 
	 * @param id Id of the app to focus
	 */
	void (*set_client_focus)(struct wl_client *client,
				 struct wl_resource *resource,
				 const char *id);
	/**
	 * @param id id of the client or group
	 */
	void (*get_properties)(struct wl_client *client,
			       struct wl_resource *resource,
			       const char *id);
	/**
	 * get the focused client id
	 *
	 * 
	 */
	void (*get_focused_client)(struct wl_client *client,
				   struct wl_resource *resource);
	/**
	 * returns a list of clients
	 *
	 * 
	 */
	void (*get_clients)(struct wl_client *client,
			    struct wl_resource *resource);
};

#define FIREBOLT_WM_CLIENT_PROPERTIES 0
#define FIREBOLT_WM_FOCUSED_CLIENT 1
#define FIREBOLT_WM_CLIENTS 2

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
 * Sends an client_properties event to the client owning the resource.
 * @param resource_ The client's resource
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
static inline void
firebolt_wm_send_client_properties(struct wl_resource *resource_, const char *id, int32_t x, int32_t y, uint32_t width, uint32_t height, wl_fixed_t opacity, int32_t zorder, int32_t visible, wl_fixed_t crop_x, wl_fixed_t crop_y, wl_fixed_t crop_width, wl_fixed_t crop_height, int32_t textured)
{
	wl_resource_post_event(resource_, FIREBOLT_WM_CLIENT_PROPERTIES, id, x, y, width, height, opacity, zorder, visible, crop_x, crop_y, crop_width, crop_height, textured);
}

/**
 * @ingroup iface_firebolt_wm
 * Sends an focused_client event to the client owning the resource.
 * @param resource_ The client's resource
 * @param id focused client id
 */
static inline void
firebolt_wm_send_focused_client(struct wl_resource *resource_, const char *id)
{
	wl_resource_post_event(resource_, FIREBOLT_WM_FOCUSED_CLIENT, id);
}

/**
 * @ingroup iface_firebolt_wm
 * Sends an clients event to the client owning the resource.
 * @param resource_ The client's resource
 * @param id focused client id
 */
static inline void
firebolt_wm_send_clients(struct wl_resource *resource_, const char *id)
{
	wl_resource_post_event(resource_, FIREBOLT_WM_CLIENTS, id);
}

#ifdef  __cplusplus
}
#endif

#endif

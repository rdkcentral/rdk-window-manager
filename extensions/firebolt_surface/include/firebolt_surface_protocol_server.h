/**
 * * If not stated otherwise in this file or this component's LICENSE
 * * file the following copyright and licenses apply:
 * *
 * * Copyright 2024 RDK Management
 * *
 * * Licensed under the Apache License, Version 2.0 (the "License");
 * * you may not use this file except in compliance with the License.
 * * You may obtain a copy of the License at
 * *
 * * http://www.apache.org/licenses/LICENSE-2.0
 * *
 * * Unless required by applicable law or agreed to in writing, software
 * * distributed under the License is distributed on an "AS IS" BASIS,
 * * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * * See the License for the specific language governing permissions and
 * * limitations under the License.
 * **/

#ifndef FIREBOLT_SURFACE_SERVER_PROTOCOL_H
#define FIREBOLT_SURFACE_SERVER_PROTOCOL_H

#include <stdint.h>
#include <stddef.h>
#include "wayland-server.h"

#ifdef  __cplusplus
extern "C" {
#endif

struct wl_client;
struct wl_resource;

/**
 * @page page_firebolt_surface The firebolt_surface protocol
 * @section page_ifaces_firebolt_surface Interfaces
 * - @subpage page_iface_firebolt_surface - 
 */
struct firebolt_surface;

/**
 * @page page_iface_firebolt_surface firebolt_surface
 * @section page_iface_firebolt_surface_api API
 * See @ref iface_firebolt_surface.
 */
/**
 * @defgroup iface_firebolt_surface The firebolt_surface interface
 */
extern const struct wl_interface firebolt_surface_interface;

#ifndef FIREBOLT_SURFACE_FIREBOLT_SURFACE_VISIBILITY_ENUM
#define FIREBOLT_SURFACE_FIREBOLT_SURFACE_VISIBILITY_ENUM
/**
 * @ingroup iface_firebolt_surface
 * surface visibility
 *
 * Passed as the visibility argument for the set_visible request.
 */
enum firebolt_surface_firebolt_surface_visibility {
	/**
	 * hide the surface
	 */
	FIREBOLT_SURFACE_FIREBOLT_SURFACE_VISIBILITY_HIDE = 0,
	/**
	 * show the surface
	 */
	FIREBOLT_SURFACE_FIREBOLT_SURFACE_VISIBILITY_SHOW = 1,
};
#endif /* FIREBOLT_SURFACE_FIREBOLT_SURFACE_VISIBILITY_ENUM */

/**
 * @ingroup iface_firebolt_surface
 * @struct firebolt_surface_interface
 */
struct firebolt_surface_interface {
	/**
	 * destroy the firebolt_surface
	 *
	 * Destroy the firebolt_surface object. This removes the
	 * association with the underlying wl_surface or hardware video
	 * surface and removes the surface from the composition.
	 */
	void (*destroy)(struct wl_client *client,
			struct wl_resource *resource);
	/**
	 * set the name of the firebolt_surface
	 *
	 * Sets the name of the surface
	 */
	void (*set_name)(struct wl_client *client,
			 struct wl_resource *resource,
			 const char *name);
	/**
	 * set the visibility of the surface
	 *
	 * Setting to 0 makes the surface not visible.
	 */
	void (*set_visible)(struct wl_client *client,
			    struct wl_resource *resource,
			    uint32_t visible);
	/**
	 * set the surface bounds
	 *
	 * The surface bounds of a surface is its composited pixel
	 * position and dimensions. If unset then the surface is set to the
	 * top left, with width and height that match the underlying
	 * surface dimensions.
	 *
	 * The width and height of the effective surface bounds must be
	 * greater than zero. Setting an invalid size will raise an
	 *
	 * invalid_size error.
	 */
	void (*set_bounds)(struct wl_client *client,
			   struct wl_resource *resource,
			   int32_t x,
			   int32_t y,
			   int32_t width,
			   int32_t height);
	/**
	 * set the cropping of the surface within the surface
	 *
	 * Sets the cropping of a given surface.
	 *
	 * Cropping works on an arbitrary scale, not on pixels. The input
	 * surface is defined as a rectangle with width and height of 1.0
	 * and therefore all cropping values should be fixed point values
	 * between 0.0 and 1.0 inclusive.
	 *
	 * For example to crop the top right quarter of the video then set
	 * (x, y, width, height) to (0.5, 0.0, 0.5, 0.5).
	 */
	void (*set_crop)(struct wl_client *client,
			 struct wl_resource *resource,
			 wl_fixed_t sx,
			 wl_fixed_t sy,
			 wl_fixed_t swidth,
			 wl_fixed_t sheight);
	/**
	 * set the relative z-order of the surface
	 *
	 * Sets the z-order of the surface relative to other surfaces
	 * within the client’s display.
	 *
	 * The z-order should be in the range of 0.0 - 1.0 inclusive.
	 */
	void (*set_zorder)(struct wl_client *client,
			   struct wl_resource *resource,
			   wl_fixed_t zorder);
	/**
	 * set surface opacity
	 *
	 * Sets the opacity of the surface. It may not be possible to set
	 * the opacity on hardware video surfaces.
	 *
	 * The opacity should be in the range of 0.0 - 1.0 inclusive.
	 */
	void (*set_opacity)(struct wl_client *client,
			    struct wl_resource *resource,
			    wl_fixed_t opacity);
};


/**
 * @ingroup iface_firebolt_surface
 */
#define FIREBOLT_SURFACE_DESTROY_SINCE_VERSION 1
/**
 * @ingroup iface_firebolt_surface
 */
#define FIREBOLT_SURFACE_SET_NAME_SINCE_VERSION 1
/**
 * @ingroup iface_firebolt_surface
 */
#define FIREBOLT_SURFACE_SET_VISIBLE_SINCE_VERSION 1
/**
 * @ingroup iface_firebolt_surface
 */
#define FIREBOLT_SURFACE_SET_BOUNDS_SINCE_VERSION 1
/**
 * @ingroup iface_firebolt_surface
 */
#define FIREBOLT_SURFACE_SET_CROP_SINCE_VERSION 1
/**
 * @ingroup iface_firebolt_surface
 */
#define FIREBOLT_SURFACE_SET_ZORDER_SINCE_VERSION 1
/**
 * @ingroup iface_firebolt_surface
 */
#define FIREBOLT_SURFACE_SET_OPACITY_SINCE_VERSION 1

#ifdef  __cplusplus
}
#endif

#endif

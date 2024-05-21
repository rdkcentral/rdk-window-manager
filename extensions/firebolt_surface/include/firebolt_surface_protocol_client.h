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

#ifndef FIREBOLT_SURFACE_CLIENT_PROTOCOL_H
#define FIREBOLT_SURFACE_CLIENT_PROTOCOL_H

#include <stdint.h>
#include <stddef.h>
#include "wayland-client.h"

#ifdef  __cplusplus
extern "C" {
#endif

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

#define FIREBOLT_SURFACE_DESTROY 0
#define FIREBOLT_SURFACE_SET_NAME 1
#define FIREBOLT_SURFACE_SET_VISIBLE 2
#define FIREBOLT_SURFACE_SET_BOUNDS 3
#define FIREBOLT_SURFACE_SET_CROP 4
#define FIREBOLT_SURFACE_SET_ZORDER 5
#define FIREBOLT_SURFACE_SET_OPACITY 6


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

/** @ingroup iface_firebolt_surface */
static inline void
firebolt_surface_set_user_data(struct firebolt_surface *firebolt_surface, void *user_data)
{
	wl_proxy_set_user_data((struct wl_proxy *) firebolt_surface, user_data);
}

/** @ingroup iface_firebolt_surface */
static inline void *
firebolt_surface_get_user_data(struct firebolt_surface *firebolt_surface)
{
	return wl_proxy_get_user_data((struct wl_proxy *) firebolt_surface);
}

static inline uint32_t
firebolt_surface_get_version(struct firebolt_surface *firebolt_surface)
{
	return wl_proxy_get_version((struct wl_proxy *) firebolt_surface);
}

/**
 * @ingroup iface_firebolt_surface
 *
 * Destroy the firebolt_surface object. This removes the
 * association with the underlying wl_surface or hardware video
 * surface and removes the surface from the composition.
 */
static inline void
firebolt_surface_destroy(struct firebolt_surface *firebolt_surface)
{
	wl_proxy_marshal((struct wl_proxy *) firebolt_surface,
			 FIREBOLT_SURFACE_DESTROY);

	wl_proxy_destroy((struct wl_proxy *) firebolt_surface);
}

/**
 * @ingroup iface_firebolt_surface
 *
 * Sets the name of the surface
 */
static inline void
firebolt_surface_set_name(struct firebolt_surface *firebolt_surface, const char *name)
{
	wl_proxy_marshal((struct wl_proxy *) firebolt_surface,
			 FIREBOLT_SURFACE_SET_NAME, name);
}

/**
 * @ingroup iface_firebolt_surface
 *
 * Setting to 0 makes the surface not visible.
 */
static inline void
firebolt_surface_set_visible(struct firebolt_surface *firebolt_surface, uint32_t visible)
{
	wl_proxy_marshal((struct wl_proxy *) firebolt_surface,
			 FIREBOLT_SURFACE_SET_VISIBLE, visible);
}

/**
 * @ingroup iface_firebolt_surface
 *
 * The surface bounds of a surface is its composited pixel
 * position and dimensions.  If unset then the surface is set
 * to the top left, with width and height that match the underlying
 * surface dimensions.
 *
 * The width and height of the effective surface bounds must be
 * greater than zero. Setting an invalid size will raise an
 *
 * invalid_size error.
 */
static inline void
firebolt_surface_set_bounds(struct firebolt_surface *firebolt_surface, int32_t x, int32_t y, int32_t width, int32_t height)
{
	wl_proxy_marshal((struct wl_proxy *) firebolt_surface,
			 FIREBOLT_SURFACE_SET_BOUNDS, x, y, width, height);
}

/**
 * @ingroup iface_firebolt_surface
 *
 * Sets the cropping of a given surface.
 *
 * Cropping works on an arbitrary scale, not on pixels. The input
 * surface is defined as a rectangle with width and height of 1.0
 * and therefore all cropping values should be fixed point
 * values between 0.0 and 1.0 inclusive.
 *
 * For example to crop the top right quarter of the video
 * then set (x, y, width, height) to (0.5, 0.0, 0.5, 0.5).
 */
static inline void
firebolt_surface_set_crop(struct firebolt_surface *firebolt_surface, wl_fixed_t sx, wl_fixed_t sy, wl_fixed_t swidth, wl_fixed_t sheight)
{
	wl_proxy_marshal((struct wl_proxy *) firebolt_surface,
			 FIREBOLT_SURFACE_SET_CROP, sx, sy, swidth, sheight);
}

/**
 * @ingroup iface_firebolt_surface
 *
 * Sets the z-order of the surface relative to other surfaces within
 * the client’s display.
 *
 * The z-order should be in the range of 0.0 - 1.0 inclusive.
 */
static inline void
firebolt_surface_set_zorder(struct firebolt_surface *firebolt_surface, wl_fixed_t zorder)
{
	wl_proxy_marshal((struct wl_proxy *) firebolt_surface,
			 FIREBOLT_SURFACE_SET_ZORDER, zorder);
}

/**
 * @ingroup iface_firebolt_surface
 *
 * Sets the opacity of the surface.  It may not be possible to set the opacity on hardware video surfaces.
 *
 * The opacity should be in the range of 0.0 - 1.0 inclusive.
 */
static inline void
firebolt_surface_set_opacity(struct firebolt_surface *firebolt_surface, wl_fixed_t opacity)
{
	wl_proxy_marshal((struct wl_proxy *) firebolt_surface,
			 FIREBOLT_SURFACE_SET_OPACITY, opacity);
}

#ifdef  __cplusplus
}
#endif

#endif

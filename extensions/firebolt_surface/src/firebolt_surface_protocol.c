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

#include <stdlib.h>
#include <stdint.h>
#include "wayland-util.h"


static const struct wl_interface *firebolt_surface_types[] = {
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
};

static const struct wl_message firebolt_surface_requests[] = {
	{ "destroy", "i", firebolt_surface_types + 0 },
	{ "set_name", "is", firebolt_surface_types + 0 },
	{ "set_visible", "iu", firebolt_surface_types + 0 },
	{ "set_bounds", "iiiii", firebolt_surface_types + 0 },
	{ "set_crop", "iffff", firebolt_surface_types + 0 },
	{ "set_zorder", "if", firebolt_surface_types + 0 },
	{ "set_opacity", "if", firebolt_surface_types + 0 },
	{ "get_properties", "i", firebolt_surface_types + 0 },
};

static const struct wl_message firebolt_surface_events[] = {
	{ "surface_properties", "iiiuufiiffffs", firebolt_surface_types + 0 },
};

WL_EXPORT const struct wl_interface firebolt_surface_interface = {
	"firebolt_surface", 1,
	8, firebolt_surface_requests,
	1, firebolt_surface_events,
};


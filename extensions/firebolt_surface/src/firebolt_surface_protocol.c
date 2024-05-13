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

#include <stdlib.h>
#include <stdint.h>
#include "wayland-util.h"


static const struct wl_interface *firebolt_surface_types[] = {
	NULL,
	NULL,
	NULL,
	NULL,
};

static const struct wl_message firebolt_surface_requests[] = {
	{ "destroy", "", firebolt_surface_types + 0 },
	{ "set_name", "s", firebolt_surface_types + 0 },
	{ "set_visible", "u", firebolt_surface_types + 0 },
	{ "set_bounds", "iiii", firebolt_surface_types + 0 },
	{ "set_crop", "ffff", firebolt_surface_types + 0 },
	{ "set_zorder", "f", firebolt_surface_types + 0 },
	{ "set_opacity", "f", firebolt_surface_types + 0 },
};

WL_EXPORT const struct wl_interface firebolt_surface_interface = {
	"firebolt_surface", 1,
	7, firebolt_surface_requests,
	0, NULL,
};


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

extern const struct wl_interface firebolt_surface_interface;
extern const struct wl_interface wl_surface_interface;

static const struct wl_interface *types[] = {
	NULL,
	&firebolt_surface_interface,
	&wl_surface_interface,
	NULL,
};

static const struct wl_message firebolt_shell_requests[] = {
	{ "get_firebolt_surface", "nou", types + 1 },
};

static const struct wl_message firebolt_shell_events[] = {
	{ "firebolt_video_surface_id", "s", types + 0 },
};

WL_EXPORT const struct wl_interface firebolt_shell_interface = {
	"firebolt_shell", 1,
	1, firebolt_shell_requests,
	1, firebolt_shell_events,
};


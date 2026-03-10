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


static const struct wl_interface *firebolt_shell_types[] = {
	NULL,
	NULL,
};

static const struct wl_message firebolt_shell_requests[] = {
	{ "get_firebolt_surface", "iu", firebolt_shell_types + 0 },
};

static const struct wl_message firebolt_shell_events[] = {
	{ "firebolt_video_surface_id", "s", firebolt_shell_types + 0 },
	{ "on_focus", "s", firebolt_shell_types + 0 },
	{ "on_blur", "s", firebolt_shell_types + 0 },
};

WL_EXPORT const struct wl_interface firebolt_shell_interface = {
	"firebolt_shell", 1,
	1, firebolt_shell_requests,
	3, firebolt_shell_events,
};


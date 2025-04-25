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


static const struct wl_interface *firebolt_wm_types[] = {
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
	NULL,
	NULL,
};

static const struct wl_message firebolt_wm_requests[] = {
	{ "set_properties", "siiuuuufiiffff", firebolt_wm_types + 0 },
	{ "create", "s", firebolt_wm_types + 0 },
	{ "create_with_bounds", "siiuu", firebolt_wm_types + 0 },
	{ "create_with_properties", "siiuuuufiiffffi", firebolt_wm_types + 0 },
	{ "destroy", "s", firebolt_wm_types + 0 },
	{ "set_client_bounds", "siiuu", firebolt_wm_types + 0 },
	{ "set_client_display_bounds", "suu", firebolt_wm_types + 0 },
	{ "set_client_focus", "s", firebolt_wm_types + 0 },
	{ "get_properties", "s", firebolt_wm_types + 0 },
	{ "get_focused_client", "", firebolt_wm_types + 0 },
	{ "get_clients", "", firebolt_wm_types + 0 },
	{ "set_owner", "si", firebolt_wm_types + 0 },
	{ "get_owner", "s", firebolt_wm_types + 0 },
};

static const struct wl_message firebolt_wm_events[] = {
	{ "client_properties", "siiuufiiffffi", firebolt_wm_types + 0 },
	{ "focused_client", "s", firebolt_wm_types + 0 },
	{ "clients", "s", firebolt_wm_types + 0 },
	{ "client_owner", "si", firebolt_wm_types + 0 },
};

WL_EXPORT const struct wl_interface firebolt_wm_interface = {
	"firebolt_wm", 1,
	13, firebolt_wm_requests,
	4, firebolt_wm_events,
};


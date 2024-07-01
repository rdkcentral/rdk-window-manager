# If not stated otherwise in this file or this component's license file the
# following copyright and licenses apply:
#
# Copyright 2024 RDK Management
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

add_library( rdkwmextfireboltwm_shared
        SHARED
        src/firebolt_wm_protocol.c
        )

target_link_libraries( rdkwmextfireboltwm_shared -lwayland-client)

target_include_directories( rdkwmextfireboltwm_shared
        PRIVATE
        include
        $<TARGET_PROPERTY:rdkwindowmanager_shared,INTERFACE_INCLUDE_DIRECTORIES>
        )

set_target_properties( rdkwmextfireboltwm_shared
        PROPERTIES
        OUTPUT_NAME "rdkwmextfireboltwm"
        )

install( TARGETS rdkwmextfireboltwm_shared
         DESTINATION lib/
        )

add_library( wstplugin_rdkwmfireboltwm_shared
        SHARED
        src/firebolt_wm.cpp
        src/firebolt_wm_protocol.c
        )

target_link_libraries( wstplugin_rdkwmfireboltwm_shared -lwayland-server rdkwindowmanager_shared)

target_include_directories( wstplugin_rdkwmfireboltwm_shared
        PRIVATE
        include
        $<TARGET_PROPERTY:rdkwindowmanager_shared,INTERFACE_INCLUDE_DIRECTORIES>
        )

set_target_properties( wstplugin_rdkwmfireboltwm_shared
        PROPERTIES
        OUTPUT_NAME "wstplugin_rdkwmfireboltwm"
        )

install( TARGETS wstplugin_rdkwmfireboltwm_shared
         DESTINATION lib/plugins/westeros/
        )

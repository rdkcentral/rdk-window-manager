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

cmake_minimum_required(VERSION 2.8)
project(window_manager)
set(CMAKE_CXX_STANDARD 14)

option(RDK_WINDOW_MANAGER_BUILD_APP "RDK_WINDOW_MANAGER_BUILD_APP" ON)
option(RDK_WINDOW_MANAGER_BUILD_WEBSOCKET_IPC "RDK_WINDOW_MANAGER_BUILD_WEBSOCKET_IPC" OFF)
option(RDK_WINDOW_MANAGER_BUILD_KEY_METADATA "RDK_WINDOW_MANAGER_BUILD_KEY_METADATA" OFF)
option(RDK_WINDOW_MANAGER_BUILD_HIDDEN_SUPPORT "RDK_WINDOW_MANAGER_BUILD_HIDDEN_SUPPORT" OFF)
option(RDK_WINDOW_MANAGER_BUILD_FORCE_1080 "RDK_WINDOW_MANAGER_BUILD_FORCE_1080" OFF)
option(RDK_WINDOW_MANAGER_BUILD_FORCE_ANIMATE "RDK_WINDOW_MANAGER_BUILD_FORCE_ANIMATE" OFF)
option(RDK_WINDOW_MANAGER_BUILD_EXTERNAL_APPLICATION_SURFACE_COMPOSITION "RDK_WINDOW_MANAGER_BUILD_EXTERNAL_APPLICATION_SURFACE_COMPOSITION" ON)
option(RDK_WINDOW_MANAGER_BUILD_KEYBUBBING_TOP_MODE "RDK_WINDOW_MANAGER_BUILD_KEYBUBBING_TOP_MODE" ON)
option(RDK_WINDOW_MANAGER_BUILD_ENABLE_KEYREPEATS "RDK_WINDOW_MANAGER_BUILD_ENABLE_KEYREPEATS" OFF)

set(RDK_WINDOW_MANAGER_SOURCES
  src/rdkwindowmanager.cpp
  src/compositorcontroller.cpp
  src/essosinstance.cpp
  src/rdkcompositor.cpp
  src/rdkcompositorsurface.cpp
  src/rdkcompositornested.cpp
  src/linuxkeys.cpp
  src/eastereggs.cpp
  src/animation.cpp
  src/animationutilities.cpp
  src/rdkwindowmanagerdata.cpp
  src/rdkwindowmanagerjson.cpp
  src/linuxinput.cpp
  src/logger.cpp
  src/rdkwindowmanagerimage.cpp
  src/permissions.cpp
  src/framebuffer.cpp
  src/framebufferrenderer.cpp
  csrc/ursor.cpp
)
set(RDK_WINDOW_MANAGER_LINK_LIBRARIES -lz -lessos -lEGL -lGLESv2 -lwayland-client -lwesteros_compositor -lpthread -ljpeg -lpng16)

if (RDK_WINDOW_MANAGER_BUILD_WEBSOCKET_IPC)
  set(RDK_WINDOW_MANAGER_SOURCES ${RDK_WINDOW_MANAGER_SOURCES} src/messageHandler.cpp)
  set(RDK_WINDOW_MANAGER_LINK_LIBRARIES ${RDK_WINDOW_MANAGER_LINK_LIBRARIES} -luWS)
  add_definitions("-DRDK_WINDOW_MANAGER_ENABLE_WEBSOCKET_IPC")
endif (RDK_WINDOW_MANAGER_BUILD_WEBSOCKET_IPC)

include_directories(AFTER ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/include)

if (RDK_WINDOW_MANAGER_WESTEROS_PLUGIN_FOLDER)
    add_definitions(-DRDK_WINDOW_MANAGER_WESTEROS_PLUGIN_DIRECTORY="${RDK_WINDOW_MANAGER_WESTEROS_PLUGIN_FOLDER}")
else()
    add_definitions(-DRDK_WINDOW_MANAGER_WESTEROS_PLUGIN_DIRECTORY="/usr/lib/plugins/westeros/")
endif (RDK_WINDOW_MANAGER_WESTEROS_PLUGIN_FOLDER)

if (RDK_WINDOW_MANAGER_BUILD_KEY_METADATA)
  add_definitions("-DRDK_WINDOW_MANAGER_ENABLE_KEY_METADATA")
endif (RDK_WINDOW_MANAGER_BUILD_KEY_METADATA)

if (RDK_WINDOW_MANAGER_BUILD_HIDDEN_SUPPORT)
  add_definitions("-DRDK_WINDOW_MANAGER_ENABLE_HIDDEN_SUPPORT")
endif (RDK_WINDOW_MANAGER_BUILD_HIDDEN_SUPPORT)

if (RDK_WINDOW_MANAGER_BUILD_FORCE_1080)
  add_definitions("-DRDK_WINDOW_MANAGER_ENABLE_FORCE_1080")
endif (RDK_WINDOW_MANAGER_BUILD_FORCE_1080)

if (RDK_WINDOW_MANAGER_BUILD_FORCE_ANIMATE)
  add_definitions("-DRDK_WINDOW_MANAGER_ENABLE_FORCE_ANIMATE")
endif (RDK_WINDOW_MANAGER_BUILD_FORCE_ANIMATE)

if (RDK_WINDOW_MANAGER_BUILD_EXTERNAL_APPLICATION_SURFACE_COMPOSITION)
  add_definitions("-DRDK_WINDOW_MANAGER_ENABLE_EXTERNAL_APPLICATION_SURFACE_COMPOSITION")
endif (RDK_WINDOW_MANAGER_BUILD_EXTERNAL_APPLICATION_SURFACE_COMPOSITION)

if (RDK_WINDOW_MANAGER_BUILD_KEYBUBBING_TOP_MODE)
  add_definitions("-DRDK_WINDOW_MANAGER_ENABLE_KEYBUBBING_TOP_MODE")
endif (RDK_WINDOW_MANAGER_BUILD_KEYBUBBING_TOP_MODE)

if (RDK_WINDOW_MANAGER_BUILD_ENABLE_KEYREPEATS)
  add_definitions("-DRDK_WINDOW_MANAGER_ENABLE_KEYREPEATS")
endif (RDK_WINDOW_MANAGER_BUILD_ENABLE_KEYREPEATS)

if(BUILD_ENABLE_ERM)
    add_definitions("-DENABLE_ERM")
    set(RDK_WINDOW_MANAGER_LINK_LIBRARIES ${RDK_WINDOW_MANAGER_LINK_LIBRARIES} -lessosrmgr)
endif()

add_library(rdkwindowmanager_shared SHARED ${RDK_WINDOW_MANAGER_SOURCES})
set_target_properties(rdkwindowmanager_shared PROPERTIES OUTPUT_NAME rdkwindowmanager)
target_link_libraries(rdkwindowmanager_shared ${RDK_WINDOW_MANAGER_LINK_LIBRARIES})


if (RDK_WINDOW_MANAGER_BUILD_APP)
    message("Building rdkwindowmanager executable")
    add_executable(rdkwindowmanager src/main.cpp)
    add_dependencies(rdkwindowmanager rdkwindowmanager_shared)
    target_link_libraries(rdkwindowmanager ${RDK_WINDOW_MANAGER_LINK_LIBRARIES} rdkwindowmanager_shared -lpthread)
endif (RDK_WINDOW_MANAGER_BUILD_APP)

# If not stated otherwise in this file or this component's LICENSE file the
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

if (RDK_WINDOW_MANAGER_VNC_SERVER)
# Add the top level directory as an include path for all modules
set( CMAKE_VERBOSE_MAKEFILE on )
message("CMAKE_SOURCE_DIR=${CMAKE_SOURCE_DIR}")
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/CMake/")
message("CMAKE_MODULE_PATH=${CMAKE_MODULE_PATH}")

find_package( GLIB REQUIRED gobject gio gio-unix )
find_package( LibSoup REQUIRED )
find_package( Boost REQUIRED )
endif (RDK_WINDOW_MANAGER_VNC_SERVER)

option(RDK_WINDOW_MANAGER_BUILD_APP "RDK_WINDOW_MANAGER_BUILD_APP" ON)
option(RDK_WINDOW_MANAGER_BUILD_KEY_METADATA "RDK_WINDOW_MANAGER_BUILD_KEY_METADATA" OFF)
option(RDK_WINDOW_MANAGER_BUILD_HIDDEN_SUPPORT "RDK_WINDOW_MANAGER_BUILD_HIDDEN_SUPPORT" OFF)
option(RDK_WINDOW_MANAGER_BUILD_FORCE_1080 "RDK_WINDOW_MANAGER_BUILD_FORCE_1080" ON)
option(RDK_WINDOW_MANAGER_BUILD_FORCE_ANIMATE "RDK_WINDOW_MANAGER_BUILD_FORCE_ANIMATE" OFF)
option(RDK_WINDOW_MANAGER_BUILD_EXTERNAL_APPLICATION_SURFACE_COMPOSITION "RDK_WINDOW_MANAGER_BUILD_EXTERNAL_APPLICATION_SURFACE_COMPOSITION" ON)
option(RDK_WINDOW_MANAGER_BUILD_KEYBUBBING_TOP_MODE "RDK_WINDOW_MANAGER_BUILD_KEYBUBBING_TOP_MODE" ON)
option(RDK_WINDOW_MANAGER_BUILD_ENABLE_KEYREPEATS "RDK_WINDOW_MANAGER_BUILD_ENABLE_KEYREPEATS" OFF)
option(RDK_WINDOW_MANAGER_BUILD_EXTENSIONS "RDK_WINDOW_MANAGER_BUILD_EXTENSIONS" ON)
if (RDK_WINDOW_MANAGER_BUILD_EXTENSIONS)
option(RDK_WINDOW_MANAGER_BUILD_FIREBOLT_SURFACE_EXTENSION "RDK_WINDOW_MANAGER_BUILD_FIREBOLT_SURFACE_EXTENSION" ON)
option(RDK_WINDOW_MANAGER_BUILD_FIREBOLT_SHELL_EXTENSION "RDK_WINDOW_MANAGER_BUILD_FIREBOLT_SHELL_EXTENSION" ON)
option(RDK_WINDOW_MANAGER_BUILD_FIREBOLT_WM_EXTENSION "RDK_WINDOW_MANAGER_BUILD_FIREBOLT_WM_EXTENSION" ON)
endif (RDK_WINDOW_MANAGER_BUILD_EXTENSIONS)
option(RDK_WINDOW_MANAGER_BUILD_TEST_APP "RDK_WINDOW_MANAGER_BUILD_TEST_APP" ON)
if (RDK_WINDOW_MANAGER_BUILD_TEST_APP)
option(RDK_WINDOW_MANAGER_LOGGER "RDK_WINDOW_MANAGER_LOGGER" ON)
option (RDK_WINDOW_MANAGER_BUILD_TEST_APP_WITH_OPENGL "RDK_WINDOW_MANAGER_BUILD_TEST_APP_WITH_OPENGL" ON)
endif (RDK_WINDOW_MANAGER_BUILD_TEST_APP)

option(ENABLE_RDKWINDOWMANAGER_VNCSERVER2 "Enable RDKWindowManager VNCServer2 integration for RFB frame passing and key routing" OFF)
option(RDK_WINDOW_MANAGER_BUILD_SPLASH_SCREEN "Enable splash screen support (image path and duration via env variables)" OFF)
option(ENABLE_RDKWINDOWMANAGER_RENDER_MINIMIZE "Enable resize of window to 1x1 and maximize inside enableDisplayRender" ON)

set(RDK_WINDOW_MANAGER_SOURCES
  src/rdkwindowmanager.cpp
  src/compositorcontroller.cpp
  src/essosinstance.cpp
  src/rdkcompositor.cpp
  src/rdkcompositornested.cpp
  src/linuxkeys.cpp
  src/rdkwindowmanagerdata.cpp
  src/rdkwindowmanagerjson.cpp
  src/linuxinput.cpp
  src/logger.cpp
  src/rdkwindowmanagerimage.cpp
  src/framebuffer.cpp
  src/framebufferrenderer.cpp
  src/cursor.cpp
)

set(LIB_PATH "$ENV{LIB_PATH}")
link_directories(${LIB_PATH})

set(RDK_WINDOW_MANAGER_LINK_LIBRARIES -lz -lessos -lEGL -lGLESv2 -lwayland-client -lwesteros_compositor -lpthread -ljpeg -lpng16 -lwayland-egl)
include_directories(${INCLUDE_HEADER_DIR})

include_directories(AFTER ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/include)

if(RDK_WINDOW_MANAGER_VNC_SERVER)
     message(STATUS "Enable VNC server support")
     add_definitions("-DRDK_WINDOW_MANAGER_VNC_SERVER")
endif()

if(ENABLE_RDKWINDOWMANAGER_VNCSERVER2)
     message(STATUS "Enable RDKWindowManager VNCServer2 integration")
     add_definitions("-DENABLE_RDKWINDOWMANAGER_VNCSERVER2")
endif()

if(RDK_WINDOW_MANAGER_BUILD_SPLASH_SCREEN)
    message(STATUS "Enable splash screen support")
    add_definitions("-DRDK_WINDOW_MANAGER_ENABLE_SPLASH_SCREEN")
endif(RDK_WINDOW_MANAGER_BUILD_SPLASH_SCREEN)
if(ENABLE_RDKWINDOWMANAGER_RENDER_MINIMIZE)
     message(STATUS "Enable render minimize: resize window to 1x1 and maximize inside enableDisplayRender")
     add_definitions("-DENABLE_RDKWINDOWMANAGER_RENDER_MINIMIZE")
endif()

if (RDK_WINDOW_MANAGER_WESTEROS_PLUGIN_FOLDER)
    add_definitions(-DRDK_WINDOW_MANAGER_WESTEROS_PLUGIN_DIRECTORY="${RDK_WINDOW_MANAGER_WESTEROS_PLUGIN_FOLDER}")
else()
    add_definitions(-DRDK_WINDOW_MANAGER_WESTEROS_PLUGIN_DIRECTORY="/usr/lib/plugins/westeros/")
endif (RDK_WINDOW_MANAGER_WESTEROS_PLUGIN_FOLDER)

if (RDK_WINDOW_MANAGER_ADDITIONAL_EXTENSIONS_CONFIG)
    add_definitions(-DRDK_WINDOW_MANAGER_ADDITIONAL_EXTENSIONS_CONFIG="${RDK_WINDOW_MANAGER_ADDITIONAL_EXTENSIONS_CONFIG}")
endif (RDK_WINDOW_MANAGER_ADDITIONAL_EXTENSIONS_CONFIG)

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

if (RDK_WINDOW_MANAGER_VNC_SERVER)
add_definitions("-DRDK_WINDOW_MANAGER_VNC_SERVER_PORT=5900")
include_directories(AFTER ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/include/VncServer)
list( APPEND RDK_WINDOW_MANAGER_SOURCES
  src/VncServer/VncServer.cpp
  src/VncServer/VncServerFactory.cpp
  src/VncServer/VncFrameBuffer.cpp
  src/VncServer/VncClient.cpp
  src/VncServer/VncBuffer.cpp
  src/VncServer/VncSoupTcpServer.cpp
  src/VncServer/VncSoupTcpSocket.cpp
)
if(ENABLE_RDKWINDOWMANAGER_VNCSERVER2)
  list( APPEND RDK_WINDOW_MANAGER_SOURCES
    src/VncServer/VncBridgeServer.cpp
  )
endif()
include_directories(${GLIB_INCLUDE_DIRS})
include_directories(${GIO_INCLUDE_DIRS})
include_directories(${GIO_UNIX_INCLUDE_DIRS})
include_directories(${LIBSOUP_INCLUDE_DIRS})
endif (RDK_WINDOW_MANAGER_VNC_SERVER)

if (RDK_WINDOW_MANAGER_LOGGER)
  add_definitions("-DRDK_WINDOW_MANAGER_LOGGER")
  add_definitions(-DRDK_WINDOW_MANAGER_LOGFILE=\"/opt/logs/rdkwindowmanager.log\")
  set(RDK_WINDOW_MANAGER_LOGGER_SOURCE tests/testlogmonitor.cpp)
endif (RDK_WINDOW_MANAGER_LOGGER)

# Wayland Extensions files and extension server portion binded with RDK window manager.
# Extension protocol interface as shared library to be linked with respective application
if (RDK_WINDOW_MANAGER_BUILD_EXTENSIONS)
    add_definitions("-DRDK_WINDOW_MANAGER_BUILD_EXTENSIONS")
    set(RDK_WINDOW_MANAGER_TESTAPP_LINK_LIBRARIES -lwayland-client -lrt)

    if (RDK_WINDOW_MANAGER_BUILD_FIREBOLT_SURFACE_EXTENSION)
        message("Building rdkwindowmanager firebolt_surface extension") 
        add_definitions("-DRDK_WINDOW_MANAGER_BUILD_FIREBOLT_SURFACE_EXTENSION")
        include_directories(${PROJECT_SOURCE_DIR}/extensions/firebolt_surface/include)
        add_subdirectory(extensions/firebolt_surface)
        set(RDK_WINDOW_MANAGER_TESTAPP_LINK_LIBRARIES ${RDK_WINDOW_MANAGER_TESTAPP_LINK_LIBRARIES} rdkwmextfireboltsurface_shared)
    endif (RDK_WINDOW_MANAGER_BUILD_FIREBOLT_SURFACE_EXTENSION)

    if (RDK_WINDOW_MANAGER_BUILD_FIREBOLT_SHELL_EXTENSION)
        message("Building rdkwindowmanager firebolt_shell extension") 
        add_definitions("-DRDK_WINDOW_MANAGER_BUILD_FIREBOLT_SHELL_EXTENSION")
        include_directories(${PROJECT_SOURCE_DIR}/extensions/firebolt_shell/include)
        add_subdirectory(extensions/firebolt_shell)
        set(RDK_WINDOW_MANAGER_TESTAPP_LINK_LIBRARIES ${RDK_WINDOW_MANAGER_TESTAPP_LINK_LIBRARIES} rdkwmextfireboltshell_shared)
    endif (RDK_WINDOW_MANAGER_BUILD_FIREBOLT_SHELL_EXTENSION)

    if (RDK_WINDOW_MANAGER_BUILD_FIREBOLT_WM_EXTENSION)
        message("Building rdkwindowmanager firebolt_wm extension") 
        add_definitions("-DRDK_WINDOW_MANAGER_BUILD_FIREBOLT_WM_EXTENSION")
        include_directories(${PROJECT_SOURCE_DIR}/extensions/firebolt_wm/include)
        add_subdirectory(extensions/firebolt_wm)
        set(RDK_WINDOW_MANAGER_TESTAPP_LINK_LIBRARIES ${RDK_WINDOW_MANAGER_TESTAPP_LINK_LIBRARIES} rdkwmextfireboltwm_shared)
    endif (RDK_WINDOW_MANAGER_BUILD_FIREBOLT_WM_EXTENSION)
endif (RDK_WINDOW_MANAGER_BUILD_EXTENSIONS)

add_library(rdkwindowmanager_shared SHARED ${RDK_WINDOW_MANAGER_SOURCES})
set_target_properties(rdkwindowmanager_shared PROPERTIES OUTPUT_NAME rdkwindowmanager)
target_link_libraries(rdkwindowmanager_shared ${RDK_WINDOW_MANAGER_LINK_LIBRARIES})

if (RDK_WINDOW_MANAGER_VNC_SERVER)
target_link_libraries(rdkwindowmanager_shared 
      secure_wrapper
      ${GLIB_GIO_LIBRARIES}
      ${GLIB_GIO_UNIX_LIBRARIES}
      ${GLIB_GOBJECT_LIBRARIES}
      ${GLIB_LIBRARIES}
      ${LIBSOUP_LIBRARIES})
endif (RDK_WINDOW_MANAGER_VNC_SERVER)

if (RDK_WINDOW_MANAGER_BUILD_APP)
    message("Building rdkwindowmanager executable")
    add_executable(rdkwindowmanager src/main.cpp)
    add_dependencies(rdkwindowmanager rdkwindowmanager_shared)
    target_link_libraries(rdkwindowmanager ${RDK_WINDOW_MANAGER_LINK_LIBRARIES} rdkwindowmanager_shared -lpthread)
endif (RDK_WINDOW_MANAGER_BUILD_APP)

if (RDK_WINDOW_MANAGER_BUILD_TEST_APP)
    add_definitions("-DRDK_WINDOW_MANAGER_BUILD_TEST_APP")
    if (RDK_WINDOW_MANAGER_BUILD_TEST_APP_WITH_OPENGL)
	    add_definitions("-DRDK_WINDOW_MANAGER_BUILD_TEST_APP_WITH_OPENGL")
    endif(RDK_WINDOW_MANAGER_BUILD_TEST_APP_WITH_OPENGL)
    message("Building rdkwindowmanagertest executable")
    add_executable(rdkwindowmanagertest tests/testmain.cpp)
    add_dependencies(rdkwindowmanagertest rdkwindowmanager_shared ${RDK_WINDOW_MANAGER_TESTAPP_LINK_LIBRARIES})
    target_link_libraries(rdkwindowmanagertest rdkwindowmanager_shared ${RDK_WINDOW_MANAGER_TESTAPP_LINK_LIBRARIES} -lpthread)

    message("Building rdkwmtest executable")
    add_executable(rdkwmtest tests/testrdkwm.cpp ${RDK_WINDOW_MANAGER_LOGGER_SOURCE})
    add_dependencies(rdkwmtest ${RDK_WINDOW_MANAGER_TESTAPP_LINK_LIBRARIES})
    target_link_libraries(rdkwmtest ${RDK_WINDOW_MANAGER_TESTAPP_LINK_LIBRARIES} 
    ${RDK_WINDOW_MANAGER_LINK_LIBRARIES} -lcurl)
endif (RDK_WINDOW_MANAGER_BUILD_TEST_APP)

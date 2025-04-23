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

cmake_minimum_required(VERSION 3.10)
project(FireboltWML1Test)

set(CMAKE_CXX_STANDARD 11)

# Find GTest package, which includes GMock
find_package(GTest REQUIRED)

# Fetch and include GoogleTest (GTest and GMock)
include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/609281088cfefc76f9d0ce82e1ff6c30cc3591e5.zip
)
FetchContent_MakeAvailable(googletest)

file(GLOB TESTS *.cpp)

add_executable(${PROJECT_NAME}
    ${TESTS}
    ../../../extensions/firebolt_wm/src/firebolt_wm.cpp
    ../../../extensions/firebolt_wm/src/firebolt_wm_protocol.c
	../common/src/test_Firebolt_Extension.cpp
    ../mocks/wayland-server.cpp
    ../mocks/logger.cpp
    ../mocks/compositorcontroller.cpp
    ../mocks/westeros-compositor.cpp
)

set_source_files_properties(
    ${TESTS}
    PROPERTIES COMPILE_FLAGS "-fexceptions"
)

target_include_directories(${PROJECT_NAME}
    PRIVATE
    ../../../extensions/firebolt_wm/include
	../common/inc
    ../mocks
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
    GTest::gtest
    GTest::gtest_main
    GTest::gmock
    GTest::gmock_main
)

add_test(
    NAME ${PROJECT_NAME}
    COMMAND ${PROJECT_NAME}
)

install(TARGETS ${PROJECT_NAME} DESTINATION bin)

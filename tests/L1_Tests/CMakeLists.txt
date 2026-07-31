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

include_directories(${CMAKE_SOURCE_DIR}/mocks)

# Include l1tests.cmake to generate empty headers
include(l1tests.cmake)

# Add the unit_tests for firebolt_wm directory
add_subdirectory(firebolt_wm_tests)

# Add the unit_tests for firebolt_shell directory
add_subdirectory(firebolt_shell_tests)

# Add the unit_tests for firebolt_surface directory
add_subdirectory(firebolt_surface_tests)

# Add the unit_tests for the split-screen POC
add_subdirectory(split_screen_tests)

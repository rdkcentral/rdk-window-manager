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

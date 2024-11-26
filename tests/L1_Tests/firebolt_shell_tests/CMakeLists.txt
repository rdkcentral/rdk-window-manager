cmake_minimum_required(VERSION 3.10)
project(FireboltShellL1Test)

set(CMAKE_CXX_STANDARD 11)

# Find GTest package, which includes GMock
find_package(GTest REQUIRED)

# Fetch and include GoogleTest (GTest and GMock) if necessary
include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/609281088cfefc76f9d0ce82e1ff6c30cc3591e5.zip
)
FetchContent_MakeAvailable(googletest)

# Define source files for the test executable
file(GLOB TESTS *.cpp)

# Define the test executable
add_executable(${PROJECT_NAME}
    ${TESTS}
    ../../../extensions/firebolt_shell/src/firebolt_shell.cpp
    ../../../extensions/firebolt_shell/src/firebolt_shell_protocol.c
	../common/src/test_Firebolt_Extension.cpp
    ../mocks/wayland-server.cpp
    ../mocks/logger.cpp
    ../mocks/compositorcontroller.cpp
    ../mocks/westeros-compositor.cpp
)

# Set compiler flags if needed
set_source_files_properties(
    ${TESTS}
    PROPERTIES COMPILE_FLAGS "-fexceptions"
)

# Include directories
target_include_directories(${PROJECT_NAME}
    PRIVATE
    ../../../extensions/firebolt_shell/include
    ../../../extensions/firebolt_wm/include
	../common/inc
    ../mocks
)

# Link libraries
target_link_libraries(${PROJECT_NAME}
    PRIVATE
    GTest::gtest
    GTest::gtest_main
    GTest::gmock
    GTest::gmock_main
)

# Add the test to CTest
add_test(
    NAME ${PROJECT_NAME}
    COMMAND ${PROJECT_NAME}
)

# Installation instructions (if applicable)
install(TARGETS ${PROJECT_NAME} DESTINATION bin)

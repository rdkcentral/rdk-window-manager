# RDK Window Manager - Configuration & Build

## 1. Overview

This document covers the build system, configuration options, environment variables, and deployment for the RDK Window Manager.

---

## 2. Build System

### Prerequisites

| Dependency | Description |
|------------|-------------|
| **CMake** | Version 2.8 or higher |
| **C++14 Compiler** | GCC or Clang with C++14 support |
| **Westeros** | Wayland compositor library |
| **Essos** | EGL/OpenGL ES abstraction |
| **OpenGL ES 2.0** | Graphics rendering |
| **Wayland** | Display server protocol libraries |
| **libpng** | PNG image loading |
| **libjpeg** | JPEG image loading |
| **zlib** | Compression library |

### Optional Dependencies

| Dependency | Required For |
|------------|--------------|
| **GLIB/GIO** | VNC Server support |
| **LibSoup** | VNC Server support |
| **Boost** | VNC Server support |

---

## 3. CMake Configuration

### Basic Build

```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
```

### Build with Options

```bash
cmake .. \
    -DRDK_WINDOW_MANAGER_BUILD_APP=ON \
    -DRDK_WINDOW_MANAGER_BUILD_EXTENSIONS=ON \
    -DRDK_WINDOW_MANAGER_BUILD_TEST_APP=ON \
    -DRDK_WINDOW_MANAGER_LOGGER=ON
```

### CMake Options Reference

#### Core Options

| Option | Default | Description |
|--------|---------|-------------|
| `RDK_WINDOW_MANAGER_BUILD_APP` | ON | Build the main executable |
| `RDK_WINDOW_MANAGER_BUILD_KEY_METADATA` | OFF | Enable key metadata support |
| `RDK_WINDOW_MANAGER_BUILD_HIDDEN_SUPPORT` | OFF | Enable hidden surface support |
| `RDK_WINDOW_MANAGER_BUILD_FORCE_1080` | ON | Force 1080p/720p resolution control |
| `RDK_WINDOW_MANAGER_BUILD_FORCE_ANIMATE` | OFF | Force animation mode |
| `RDK_WINDOW_MANAGER_BUILD_EXTERNAL_APPLICATION_SURFACE_COMPOSITION` | ON | External surface composition |
| `RDK_WINDOW_MANAGER_BUILD_KEYBUBBING_TOP_MODE` | ON | Key bubbling to topmost |
| `RDK_WINDOW_MANAGER_BUILD_ENABLE_KEYREPEATS` | OFF | Enable key repeat by default |

#### Extension Options

| Option | Default | Description |
|--------|---------|-------------|
| `RDK_WINDOW_MANAGER_BUILD_EXTENSIONS` | ON | Build all Wayland extensions |
| `RDK_WINDOW_MANAGER_BUILD_FIREBOLT_SURFACE_EXTENSION` | ON | Build Firebolt Surface extension |
| `RDK_WINDOW_MANAGER_BUILD_FIREBOLT_SHELL_EXTENSION` | ON | Build Firebolt Shell extension |
| `RDK_WINDOW_MANAGER_BUILD_FIREBOLT_WM_EXTENSION` | ON | Build Firebolt WM extension |

#### Test & Debug Options

| Option | Default | Description |
|--------|---------|-------------|
| `RDK_WINDOW_MANAGER_BUILD_TEST_APP` | ON | Build test application |
| `RDK_WINDOW_MANAGER_BUILD_TEST_APP_WITH_OPENGL` | ON | Build test app with OpenGL |
| `RDK_WINDOW_MANAGER_LOGGER` | ON | Enable file logging |

#### VNC Server Options

| Option | Default | Description |
|--------|---------|-------------|
| `RDK_WINDOW_MANAGER_VNC_SERVER` | OFF | Enable VNC server support |

#### ERM Options

| Option | Default | Description |
|--------|---------|-------------|
| `BUILD_ENABLE_ERM` | OFF | Enable Essential Resource Manager |

### Path Configuration

| CMake Variable | Description |
|----------------|-------------|
| `RDK_WINDOW_MANAGER_WESTEROS_PLUGIN_FOLDER` | Westeros plugin directory (default: `/usr/lib/plugins/westeros/`) |
| `LIB_PATH` | Library search path (from environment) |
| `INCLUDE_HEADER_DIR` | Include header directory |

---

## 4. CMakeLists.txt Structure

**File:** `CMakeLists.txt`

```cmake
cmake_minimum_required(VERSION 2.8)
project(window_manager)
set(CMAKE_CXX_STANDARD 14)

# Source files
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

# Link libraries
set(RDK_WINDOW_MANAGER_LINK_LIBRARIES 
    -lz -lessos -lEGL -lGLESv2 
    -lwayland-client -lwesteros_compositor 
    -lpthread -ljpeg -lpng16 -lwayland-egl
)

# Shared library
add_library(rdkwindowmanager_shared SHARED ${RDK_WINDOW_MANAGER_SOURCES})
set_target_properties(rdkwindowmanager_shared PROPERTIES OUTPUT_NAME rdkwindowmanager)
target_link_libraries(rdkwindowmanager_shared ${RDK_WINDOW_MANAGER_LINK_LIBRARIES})

# Executable
if (RDK_WINDOW_MANAGER_BUILD_APP)
    add_executable(rdkwindowmanager src/main.cpp)
    add_dependencies(rdkwindowmanager rdkwindowmanager_shared)
    target_link_libraries(rdkwindowmanager ${RDK_WINDOW_MANAGER_LINK_LIBRARIES} 
                          rdkwindowmanager_shared -lpthread)
endif()
```

---

## 5. Preprocessor Definitions

The following preprocessor macros are defined based on CMake options:

| Macro | Enabled By | Purpose |
|-------|------------|---------|
| `RDK_WINDOW_MANAGER_ENABLE_KEY_METADATA` | `RDK_WINDOW_MANAGER_BUILD_KEY_METADATA` | Key metadata in events |
| `RDK_WINDOW_MANAGER_ENABLE_HIDDEN_SUPPORT` | `RDK_WINDOW_MANAGER_BUILD_HIDDEN_SUPPORT` | Hidden surface support |
| `RDK_WINDOW_MANAGER_ENABLE_FORCE_1080` | `RDK_WINDOW_MANAGER_BUILD_FORCE_1080` | Resolution override |
| `RDK_WINDOW_MANAGER_ENABLE_FORCE_ANIMATE` | `RDK_WINDOW_MANAGER_BUILD_FORCE_ANIMATE` | Animation mode |
| `RDK_WINDOW_MANAGER_ENABLE_EXTERNAL_APPLICATION_SURFACE_COMPOSITION` | `RDK_WINDOW_MANAGER_BUILD_EXTERNAL_APPLICATION_SURFACE_COMPOSITION` | External composition |
| `RDK_WINDOW_MANAGER_ENABLE_KEYBUBBING_TOP_MODE` | `RDK_WINDOW_MANAGER_BUILD_KEYBUBBING_TOP_MODE` | Key bubbling |
| `RDK_WINDOW_MANAGER_ENABLE_KEYREPEATS` | `RDK_WINDOW_MANAGER_BUILD_ENABLE_KEYREPEATS` | Key repeats |
| `RDK_WINDOW_MANAGER_BUILD_EXTENSIONS` | `RDK_WINDOW_MANAGER_BUILD_EXTENSIONS` | Extensions support |
| `RDK_WINDOW_MANAGER_LOGGER` | `RDK_WINDOW_MANAGER_LOGGER` | File logging |
| `RDK_WINDOW_MANAGER_LOGFILE` | - | Log file path (`/opt/logs/rdkwindowmanager.log`) |
| `RDK_WINDOW_MANAGER_VNC_SERVER` | `RDK_WINDOW_MANAGER_VNC_SERVER` | VNC server |
| `RDK_WINDOW_MANAGER_VNC_SERVER_PORT` | - | VNC port (5900) |
| `ENABLE_ERM` | `BUILD_ENABLE_ERM` | ERM support |

---

## 6. Environment Variables

### Runtime Configuration

| Variable | Type | Default | Description |
|----------|------|---------|-------------|
| `RDK_WINDOW_MANAGER_LOG_LEVEL` | string | - | Log level (Debug, Information, Warn, Error, Fatal) |
| `RDK_WINDOW_MANAGER_FRAMERATE` | int | 40 | Target frame rate (FPS) |
| `RDK_WINDOW_MANAGER_LOW_MEMORY_THRESHOLD` | double | 200 | Low RAM threshold (MB) |
| `RDK_WINDOW_MANAGER_CRITICALLY_LOW_MEMORY_THRESHOLD` | double | 100 | Critical RAM threshold (MB) |
| `RDK_WINDOW_MANAGER_SWAP_MEMORY_INCREASE_THRESHOLD` | double | 50 | Swap increase threshold (MB) |
| `RDK_WINDOW_MANAGER_KEY_INITIAL_DELAY` | int | 500 | Key repeat initial delay (ms) |
| `RDK_WINDOW_MANAGER_KEY_REPEAT_INTERVAL` | int | 100 | Key repeat interval (ms) |
| `RDK_WINDOW_MANAGER_SET_GRAPHICS_720` | string | - | Set to "1" to force 720p |

### Example Usage

```bash
export RDK_WINDOW_MANAGER_LOG_LEVEL=Debug
export RDK_WINDOW_MANAGER_FRAMERATE=60
export RDK_WINDOW_MANAGER_LOW_MEMORY_THRESHOLD=150
./rdkwindowmanager
```

---

## 7. Build Output

### Libraries

| Output | Type | Description |
|--------|------|-------------|
| `librdkwindowmanager.so` | Shared | Core window manager library |
| `librdkwmextfireboltsurface.so` | Shared | Firebolt Surface client library |
| `librdkwmextfireboltshell.so` | Shared | Firebolt Shell client library |
| `librdkwmextfireboltwm.so` | Shared | Firebolt WM client library |

### Westeros Plugins

| Output | Install Location | Description |
|--------|-----------------|-------------|
| `wstplugin_rdkwmfireboltsurface.so` | `lib/plugins/westeros/` | Firebolt Surface plugin |
| `wstplugin_rdkwmfireboltshell.so` | `lib/plugins/westeros/` | Firebolt Shell plugin |
| `wstplugin_rdkwmfireboltwm.so` | `lib/plugins/westeros/` | Firebolt WM plugin |

### Executables

| Output | Description |
|--------|-------------|
| `rdkwindowmanager` | Main window manager executable |
| `rdkwindowmanagertest` | Basic test application (if enabled) |
| `rdkwmtest` | Window manager test suite (if enabled) |

---

## 8. Installation

### Default Install Locations

```bash
make install
```

| Component | Location |
|-----------|----------|
| Main executable | System PATH |
| Shared libraries | `/usr/lib/` |
| Westeros plugins | `/usr/lib/plugins/westeros/` |

### Custom Install Prefix

```bash
cmake .. -DCMAKE_INSTALL_PREFIX=/opt/rdkwindowmanager
make install
```

---

## 9. Build Scripts

### build_rdkwindowmanager.sh

**File:** `scripts/build_rdkwindowmanager.sh`

Helper script for building the window manager with common configurations.

### build_dependencies.sh

**File:** `build_dependencies.sh`

Script for building required dependencies (Westeros, Essos, etc.).

---

## 10. File System Configuration

### Input Devices Configuration

The window manager reads input device configuration from a JSON file. See `src/linuxinput.cpp` for the configuration format.

### Splash Screen

The window manager checks for a splash screen file at startup:

```cpp
#define RDK_WINDOW_MANAGER_SPLASH_SCREEN_FILE_CHECK "/tmp/.rdkwindowmanagersplash"
```

### Resolution Override Files

```cpp
// Force 720p if this file exists
std::ifstream file720("/tmp/rdkwindowmanager720");
```

---

## 11. Logging Configuration

### Log Levels

```cpp
enum LogLevel { 
    Debug,        // Verbose debug information
    Information,  // General information
    Warn,         // Warnings
    Error,        // Errors
    Fatal         // Fatal errors
};
```

### Log File Location

When `RDK_WINDOW_MANAGER_LOGGER` is enabled:

```cpp
#define RDK_WINDOW_MANAGER_LOGFILE "/opt/logs/rdkwindowmanager.log"
```

### Setting Log Level

```cpp
// Programmatically
Logger::setLogLevel("Debug");

// Or via environment variable
export RDK_WINDOW_MANAGER_LOG_LEVEL=Debug
```

---

## 12. Cross-Compilation

For embedded targets, set the appropriate toolchain file:

```bash
cmake .. \
    -DCMAKE_TOOLCHAIN_FILE=/path/to/toolchain.cmake \
    -DCMAKE_SYSROOT=/path/to/sysroot
```

### Required Toolchain Variables

| Variable | Description |
|----------|-------------|
| `CMAKE_C_COMPILER` | C compiler path |
| `CMAKE_CXX_COMPILER` | C++ compiler path |
| `CMAKE_SYSROOT` | Target system root |
| `LIB_PATH` | Target library path |
| `INCLUDE_HEADER_DIR` | Target header path |

---

## 13. Troubleshooting Build Issues

### Common Issues

| Issue | Solution |
|-------|----------|
| Missing Westeros headers | Set `INCLUDE_HEADER_DIR` to Westeros include path |
| Missing libraries | Set `LIB_PATH` to library directory |
| Extension build fails | Ensure Wayland development packages are installed |
| VNC build fails | Install GLIB, GIO, and LibSoup development packages |

### Debug Build

```bash
cmake .. -DCMAKE_BUILD_TYPE=Debug
make VERBOSE=1
```

---
description: Coding guidelines for the RDK Window Manager repository.
applyTo: "**/*.cpp,**/*.h,**/CMakeLists.txt,**/*.cmake"
---

# Instruction Summary
  1. [Runtime Logging](#runtime-logging)
  2. [Pointer and Handle Safety](#pointer-and-handle-safety)
  3. [External API Error Handling](#external-api-error-handling)
  4. [Thread Safety for Shared State](#thread-safety-for-shared-state)
  5. [Event Emission Rules](#event-emission-rules)
  6. [Client Name Canonicalization](#client-name-canonicalization)
  7. [Environment and Config Parsing](#environment-and-config-parsing)
  8. [CMake Feature Gating](#cmake-feature-gating)
  9. [Tests for Public Behavior](#tests-for-public-behavior)

### Runtime Logging
### Requirement

Use `RdkWindowManager::Logger::log(...)` for runtime code in `src/`. Log with the appropriate level and include enough context (client/display/surface IDs) to diagnose field issues.

- `Error`/`Fatal`: operation failed and behavior changed.
- `Warn`: degraded but recoverable behavior.
- `Information`/`Debug`: state transitions and diagnostics.

Avoid `std::cout`/`printf` in runtime paths. They are only acceptable in test tools under `tests/`.

### Example

```cpp
if (!WstCompositorAddModule(compositor, extensionPath.c_str())) {
    Logger::log(LogLevel::Error,
        "failed to load extension for display %s: %s",
        mDisplayName.c_str(),
        extensionPath.c_str());
    return false;
}
```

### Incorrect Example

```cpp
std::cout << "failed to load extension" << std::endl;
```

### Pointer and Handle Safety
### Requirement

For new code, use `nullptr` (not `NULL`) and guard raw external handles (`WstCompositor*`, `wl_*`, `FILE*`) before use. After destroying ownership-backed handles, reset them to `nullptr`.

### Example

```cpp
if (nullptr == mWstContext) {
    Logger::log(LogLevel::Error, "westeros compositor context is null");
    return false;
}

WstCompositorDestroy(mWstContext);
mWstContext = nullptr;
```

### External API Error Handling
### Requirement

Always check return values for external APIs (Westeros, Essos, Wayland, GL, POSIX). On failures, log the error detail and return a clear failure path (`false`, early return, or safe fallback).

### Example

```cpp
if (0 != chown(displaySocket.c_str(), ownerId, gid)) {
    Logger::log(LogLevel::Error, "failed to change owner for %s: %s",
        displaySocket.c_str(), strerror(errno));
    return false;
}
```

### Incorrect Example

```cpp
chown(displaySocket.c_str(), ownerId, gid);
return true;
```

### Thread Safety for Shared State
### Requirement

When reading or writing shared state (listener maps, application state, pending key vectors, firebolt listener maps), protect access with the correct mutex. Keep lock scope minimal and avoid calling external callbacks while holding locks unless required for consistency.

### Example

```cpp
std::lock_guard<std::mutex> locker(mInputLock);
mInputListeners.emplace(tag, std::move(listener));
```

### Event Emission Rules
### Requirement

Emit lifecycle events only on state transitions. Do not emit duplicate events for unchanged state.

Examples:
- `onVisible` only when `visible` changes `false -> true`
- `onHidden` only when `visible` changes `true -> false`
- focus and connection events should follow the same transition-only pattern

### Client Name Canonicalization
### Requirement

For all compositor lookups and map keys based on client names, canonicalize using `standardizeName(...)` before compare/store operations.

### Example

```cpp
std::string stdClientName = standardizeName(client);
if (it->name == stdClientName) {
    return true;
}
```

### Environment and Config Parsing
### Requirement

Environment-driven settings must be validated and bounded before use.

- Parse safely (`atoi`/`std::stod` guarded by checks).
- Reject invalid or non-positive values when positive values are required.
- Keep deterministic defaults when values are absent or invalid.

### Example

```cpp
int fps = atoi(value);
if (fps > 0) {
    gCurrentFramerate = fps;
}
```

### CMake Feature Gating
### Requirement

New optional capabilities must be feature-gated in CMake and compile definitions.

- Add an `option(RDK_WINDOW_MANAGER_BUILD_<FEATURE> ...)`.
- Guard source inclusion and `add_definitions(-D...)` consistently.
- Default new options to `OFF` unless always-on is required for current product behavior.

### Example

```cmake
option(RDK_WINDOW_MANAGER_BUILD_MY_FEATURE "RDK_WINDOW_MANAGER_BUILD_MY_FEATURE" OFF)
if (RDK_WINDOW_MANAGER_BUILD_MY_FEATURE)
  add_definitions("-DRDK_WINDOW_MANAGER_BUILD_MY_FEATURE")
  list(APPEND RDK_WINDOW_MANAGER_SOURCES src/myfeature.cpp)
endif()
```

### Tests for Public Behavior
### Requirement

Any change to public API behavior (Thunder-facing APIs, Firebolt extensions, input routing, focus/visibility/event flow) must include tests.

- Add or update L1 tests under `tests/L1_Tests/`.
- For integrations not suitable for L1 mocks, update `rdkwmtest` coverage in `tests/testrdkwm.cpp`.
- Include negative paths and event-order assertions where applicable.

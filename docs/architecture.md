# RDK Window Manager - Architecture Overview

## 1. High-Level Purpose & Architecture

### Role in RDK Infrastructure

The RDK Window Manager serves as the **central window management and composition system** for RDK-based devices. It acts as the bridge between:

- **Applications** (Native, Lightning, HTML) requiring display surfaces
- **Hardware Graphics** (OpenGL ES 2.0, EGL)
- **System Services** (Thunder plugins, input devices)

### Primary Responsibilities

| Responsibility | Description |
|----------------|-------------|
| **Display Management** | Create and manage Wayland displays for applications |
| **Window Composition** | Composite multiple application surfaces with z-ordering, opacity, and transformations |
| **Input Routing** | Route keyboard, pointer, and touch events to focused applications |
| **Focus Management** | Control application focus and visibility states |
| **Resource Management** | Monitor and manage graphics resources and memory |

### What RDK Window Manager Does NOT Do

- **Application Lifecycle Management** - Handled by Thunder/WPEFramework
- **Media Playback** - Handled by dedicated media pipelines
- **Network Operations** - Outside scope of window management
- **Persistent Storage** - No direct storage management

---

## 2. Architectural Overview

### System Context Diagram

```mermaid
graph TB
    subgraph "External Systems"
        Thunder[Thunder Framework]
        Apps[Applications]
        HW[Hardware/GPU]
        Input[Input Devices]
    end
    
    subgraph "RDK Window Manager"
        RWMAPI[Window Manager API]
        Core[Core Engine]
        Extensions[Wayland Extensions]
    end
    
    Thunder <-->|Plugin API| RWMAPI
    Apps <-->|Wayland Protocol| Extensions
    Core <-->|EGL/OpenGL| HW
    Input -->|Events| Core
```

### Major Components

```mermaid
classDiagram
    class RdkWindowManager {
        +initialize()
        +run()
        +update()
        +draw()
        +deinitialize()
        +seconds()
        +milliseconds()
        +microseconds()
    }
    
    class CompositorController {
        +initialize()
        +createDisplay()
        +moveToFront()
        +moveToBack()
        +setFocus()
        +getFocused()
        +onKeyPress()
        +onKeyRelease()
        +draw()
        +update()
    }
    
    class RdkCompositor {
        #mWstContext: WstCompositor*
        #mWidth: uint32_t
        #mHeight: uint32_t
        #mOpacity: double
        #mVisible: bool
        +createDisplay()
        +draw()
        +onKeyPress()
        +onKeyRelease()
        +setPosition()
        +setSize()
        +setOpacity()
        +setVisible()
    }
    
    class RdkCompositorNested {
        +createDisplay()
    }
    
    class EssosInstance {
        -mEssosContext: EssCtx*
        +instance()
        +initialize()
        +update()
        +onKeyPress()
        +onKeyRelease()
        +resolution()
    }
    
    RdkWindowManager --> CompositorController
    RdkWindowManager --> EssosInstance
    CompositorController --> RdkCompositor
    RdkCompositor <|-- RdkCompositorNested
```

### Component Interactions

```mermaid
sequenceDiagram
    participant Main
    participant RWM as RdkWindowManager
    participant CC as CompositorController
    participant EI as EssosInstance
    participant RC as RdkCompositor
    participant Westeros
    
    Main->>RWM: initialize()
    RWM->>EI: instance()->initialize()
    EI->>EI: Create Essos context
    RWM->>CC: initialize()
    
    Main->>RWM: run()
    loop Main Loop (40 FPS default)
        RWM->>CC: update()
        RWM->>CC: draw()
        CC->>RC: draw()
        RC->>Westeros: WstCompositorComposeEmbedded()
        RWM->>EI: update()
    end
```

---

## 3. Layer Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                        Thunder Plugin API                        │
│              (CreateDisplay, SetFocus, GetApps, etc.)           │
├─────────────────────────────────────────────────────────────────┤
│                     CompositorController                         │
│         (Static interface for compositor management)             │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────────────┐  │
│  │ RdkCompositor│  │ EssosInstance│  │  Wayland Extensions  │  │
│  │   (Nested)   │  │  (Singleton) │  │ (Shell/Surface/WM)   │  │
│  └──────────────┘  └──────────────┘  └──────────────────────┘  │
│                                                                  │
├─────────────────────────────────────────────────────────────────┤
│                     RdkWindowManager Core                        │
│              (Main loop, timing, initialization)                 │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────────────┐  │
│  │   Westeros   │  │    Essos     │  │   OpenGL ES 2.0      │  │
│  │  Compositor  │  │   Library    │  │   (Rendering)        │  │
│  └──────────────┘  └──────────────┘  └──────────────────────┘  │
│                                                                  │
├─────────────────────────────────────────────────────────────────┤
│                     Hardware / Kernel                            │
│              (DRM/KMS, GPU, Input devices)                       │
└─────────────────────────────────────────────────────────────────┘
```

---

## 4. Data Flow

### Input Event Flow

```mermaid
flowchart LR
    A[Input Device] --> B[EssosInstance]
    B --> C[CompositorController::onKeyPress]
    C --> D{Key Intercept?}
    D -->|Yes| E[Intercept Handler]
    D -->|No| F{Key Listener?}
    F -->|Yes| G[Listener Handler]
    F -->|No| H[Focused Compositor]
    H --> I[RdkCompositor::onKeyPress]
    I --> J[Westeros WstCompositorKeyEvent]
    J --> K[Application]
```

### Rendering Pipeline

```mermaid
flowchart TB
    A[Main Loop] --> B[glClear]
    B --> C[CompositorController::draw]
    C --> D[Iterate Compositors by Z-Order]
    D --> E{Virtual Display?}
    E -->|Yes| F[drawFbo - Render to FBO]
    E -->|No| G[drawDirect - Direct Render]
    F --> H[FrameBufferRenderer::draw]
    G --> I[WstCompositorComposeEmbedded]
    H --> J[Composite FBO texture]
    I --> J
    J --> K[EssosInstance::update]
    K --> L[eglSwapBuffers]
```

---

## 5. Threading Model

```mermaid
flowchart TB
    subgraph "Main Thread"
        ML[Main Loop]
        ML --> Update[Update]
        ML --> Draw[Draw]
        ML --> Essos[Essos Update]
    end
    
    subgraph "Application Threads"
        AT1[App Launch Thread 1]
        AT2[App Launch Thread 2]
    end
    
    subgraph "Extension Threads"
        FBE[Firebolt WM Event Worker]
    end
    
    ML -.->|Creates| AT1
    ML -.->|Creates| AT2
    ML -.->|Creates| FBE
```

### Thread Safety

| Component | Thread Safety Mechanism |
|-----------|------------------------|
| `CompositorController` | Static methods, global mutex for compositor list |
| `RdkCompositor` | `mApplicationMutex` (recursive), `mInputLock`, `mStateChangeLock` |
| `EssosInstance` | Singleton pattern, single-threaded access |
| `FireboltWindowManager` | `mContextLock`, `mQueueMutex` with condition variable |

---

## 6. Memory Management

### Compositor Lifecycle

```mermaid
stateDiagram-v2
    [*] --> Created: createDisplay()
    Created --> Running: Application connects
    Running --> Suspended: suspend()
    Suspended --> Running: resume()
    Running --> Terminated: kill() / disconnect
    Terminated --> Deleted: cleanup
    Deleted --> [*]
```

### Resource Ownership

| Resource | Owner | Cleanup |
|----------|-------|---------|
| `WstCompositor*` | `RdkCompositor` | Destructor calls `WstCompositorDestroy` |
| `FrameBuffer` | `RdkCompositor` (shared_ptr) | Automatic via shared_ptr |
| `CompositorInfo` | `gCompositorList` / `gTopmostCompositorList` | Removed on termination |
| `FireboltSurfaceInfo` | `RdkCompositor::mFireboltSurfaces` | Vector clear on destructor |

---

## 7. Error Handling Strategy

```cpp
// Pattern used throughout the codebase
bool CompositorController::someOperation(const std::string& client) {
    CompositorListIterator it;
    if (!getCompositorInfo(client, it)) {
        Logger::log(LogLevel::Error, "Client not found: %s", client.c_str());
        return false;
    }
    
    // Perform operation
    return true;
}
```

### Error Categories

| Category | Handling |
|----------|----------|
| Client not found | Return `false`, log error |
| Westeros errors | Check return values, log via `WstCompositorGetLastErrorDetail` |
| OpenGL errors | Not explicitly checked (performance consideration) |
| Memory allocation | Standard C++ exception handling |

---

## 8. Configuration Points

See [Configuration & Build](./configuration-build.md) for detailed configuration options.

| Configuration | Type | Description |
|---------------|------|-------------|
| `RDK_WINDOW_MANAGER_FRAMERATE` | Environment | Target FPS (default: 40) |
| `RDK_WINDOW_MANAGER_LOG_LEVEL` | Environment | Logging verbosity |
| `RDK_WINDOW_MANAGER_LOW_MEMORY_THRESHOLD` | Environment | RAM warning threshold (MB) |
| Build options | CMake | Feature toggles (extensions, VNC, etc.) |

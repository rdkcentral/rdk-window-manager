# RDK Window Manager - API Reference

## 1. Overview

This document provides a complete API reference for the RDK Window Manager public interfaces.

---

## 2. RdkWindowManager Namespace

**File:** `include/rdkwindowmanager.h`

### Functions

| Function | Description |
|----------|-------------|
| `void initialize()` | Initialize all subsystems (Essos, key mapping, compositor controller) |
| `void run()` | Start the main event loop (blocking) |
| `void update()` | Perform a single update cycle |
| `void draw()` | Perform a single draw cycle |
| `void deinitialize()` | Cleanup resources (currently empty) |
| `double seconds()` | Get monotonic time in seconds |
| `double milliseconds()` | Get monotonic time in milliseconds |
| `double microseconds()` | Get monotonic time in microseconds |

---

## 3. CompositorController Class

**File:** `include/compositorcontroller.h`

### Display Management

```cpp
// Initialize the compositor controller
static void initialize();

// Create a new display/compositor for a client
static bool createDisplay(
    const std::string& client,           // Client identifier
    const std::string& displayName,      // Wayland display name
    uint32_t displayWidth = 0,           // Display width (0 = screen size)
    uint32_t displayHeight = 0,          // Display height (0 = screen size)
    bool virtualDisplayEnabled = false,  // Enable FBO rendering
    uint32_t virtualWidth = 0,           // Virtual display width
    uint32_t virtualHeight = 0,          // Virtual display height
    bool topmost = false,                // Add to topmost list
    bool focus = false,                  // Set focus immediately
    int32_t ownerId = 0,                 // Owner ID for grouping
    int32_t groupId = 0                  // Group ID
);

// Terminate a client
static bool kill(const std::string& client);

// Get list of all clients
static bool getClients(std::vector<std::string>& clients);

// Get compositor instance
static std::shared_ptr<RdkCompositor> getCompositor(const std::string& displayName);
```

### Z-Order Management

```cpp
// Move client to front (highest z-order)
static bool moveToFront(const std::string& client);

// Move client to back (lowest z-order)
static bool moveToBack(const std::string& client);

// Move client behind another client
static bool moveBehind(const std::string& client, const std::string& target);

// Get z-order of client
static bool getZOrder(const std::string& client, int32_t& zorder);

// Set z-order of client
static bool setZorder(const std::string& client, int32_t zorder);

// Set/get topmost client
static bool setTopmost(const std::string& client, bool topmost, bool focus = false);
static bool getTopmost(std::string& client);
```

### Focus Management

```cpp
// Set focus to client
static bool setFocus(const std::string& client);

// Get currently focused client
static bool getFocused(std::string& client);
```

### Window Properties

```cpp
// Bounds (position and size)
static bool getBounds(const std::string& client, 
                     uint32_t& x, uint32_t& y, 
                     uint32_t& width, uint32_t& height);
static bool setBounds(const std::string& client, 
                     uint32_t x, uint32_t y, 
                     uint32_t width, uint32_t height);

// Visibility
static bool getVisibility(const std::string& client, bool& visible);
static bool setVisibility(const std::string& client, bool visible);

// Opacity (0-100)
static bool getOpacity(const std::string& client, unsigned int& opacity);
static bool setOpacity(const std::string& client, unsigned int opacity);

// Scale
static bool getScale(const std::string& client, double& scaleX, double& scaleY);
static bool setScale(const std::string& client, double scaleX, double scaleY);

// Hole punch (video passthrough)
static bool getHolePunch(const std::string& client, bool& holePunch);
static bool setHolePunch(const std::string& client, bool holePunch);

// Crop region
static bool getCrop(const std::string& client, 
                   int32_t& cropX, int32_t& cropY, 
                   int32_t& cropWidth, int32_t& cropHeight);
static bool setCrop(const std::string& client, 
                   int32_t cropX, int32_t cropY, 
                   int32_t cropWidth, int32_t cropHeight);

// Scale to fit bounds
static bool scaleToFit(const std::string& client, 
                      int32_t x, int32_t y, 
                      uint32_t width, uint32_t height);
```

### Virtual Display

```cpp
// Get virtual resolution
static bool getVirtualResolution(const std::string& client, 
                                uint32_t& virtualWidth, 
                                uint32_t& virtualHeight);

// Set virtual resolution
static bool setVirtualResolution(const std::string& client, 
                                uint32_t virtualWidth, 
                                uint32_t virtualHeight);

// Enable/disable virtual display
static bool enableVirtualDisplay(const std::string& client, bool enable);
static bool getVirtualDisplayEnabled(const std::string& client, bool& enabled);
```

### Key Interception

```cpp
// Add key intercept
static bool addKeyIntercept(
    const std::string& client,    // Client to receive intercept
    const uint32_t& keyCode,      // Key code to intercept
    const uint32_t& flags,        // Modifier flags
    const bool& focusOnly,        // Only when focused
    const bool& propagate         // Pass to app after intercept
);

// Remove key intercept
static bool removeKeyIntercept(
    const std::string& client,
    const uint32_t& keyCode,
    const uint32_t& flags
);

// Remove all key intercepts
static bool removeAllKeyIntercepts();
```

### Key Listeners

```cpp
// Add key listener
static bool addKeyListener(
    const std::string& client,
    const uint32_t& keyCode,
    const uint32_t& flags,
    std::map<std::string, RdkWindowManagerData>& listenerProperties
);
// Properties: "activate" (bool), "propagate" (bool)

// Add native key listener
static bool addNativeKeyListener(
    const std::string& client,
    const uint32_t& keyCode,
    const uint32_t& flags,
    std::map<std::string, RdkWindowManagerData>& listenerProperties
);

// Remove key listeners
static bool removeKeyListener(const std::string& client, 
                             const uint32_t& keyCode, 
                             const uint32_t& flags);
static bool removeNativeKeyListener(const std::string& client, 
                                   const uint32_t& keyCode, 
                                   const uint32_t& flags);
static bool removeAllKeyListeners();
```

### Key Generation

```cpp
// Inject key to focused compositor
static bool injectKey(const uint32_t& keyCode, const uint32_t& flags);

// Generate key for specific client
static bool generateKey(const std::string& client, 
                       const uint32_t& keyCode, 
                       const uint32_t& flags, 
                       std::string virtualKey = "");

// Generate key with duration (long press)
static bool generateKey(const std::string& client, 
                       const uint32_t& keyCode, 
                       const uint32_t& flags, 
                       std::string virtualKey, 
                       double duration);
```

### Input Events

```cpp
// Process key events (called by EssosInstance)
static void onKeyPress(uint32_t keycode, uint32_t flags, 
                      uint64_t metadata, bool physicalKeyPress = true);
static void onKeyRelease(uint32_t keycode, uint32_t flags, 
                        uint64_t metadata, bool physicalKeyPress = true);

// Process pointer events
static void onPointerMotion(uint32_t x, uint32_t y);
static void onPointerButtonPress(uint32_t keyCode, uint32_t x, uint32_t y);
static void onPointerButtonRelease(uint32_t keyCode, uint32_t x, uint32_t y);

// Input control
static bool ignoreKeyInputs(bool ignore);
static bool enableInputEvents(const std::string& client, bool enable);

// Get last key info
static bool getLastKeyPress(uint32_t& keyCode, 
                           uint32_t& modifiers, 
                           uint64_t& timestampInSeconds);
```

### Key Repeat

```cpp
// Configure key repeat
static void setKeyRepeatConfig(bool enabled, 
                              int32_t initialDelay, 
                              int32_t repeatInterval);
static bool enableKeyRepeats(bool enable);
static bool getKeyRepeatsEnabled(bool& enable);
```

### Screen Resolution

```cpp
// Get screen resolution
static bool getScreenResolution(uint32_t& width, uint32_t& height);

// Set screen resolution
static bool setScreenResolution(uint32_t width, uint32_t height);
```

### Event Listeners

```cpp
// Per-client event listeners
static bool addListener(const std::string& client, 
                       std::shared_ptr<RdkWindowManagerEventListener> listener);
static bool removeListener(const std::string& client, 
                          std::shared_ptr<RdkWindowManagerEventListener> listener);

// Global event listener
static void setEventListener(std::shared_ptr<RdkWindowManagerEventListener> listener);

// Trigger event
static bool onEvent(RdkCompositor* eventCompositor, const std::string& eventName);

// Send custom event
static bool sendEvent(const std::string& eventName, 
                     std::vector<std::map<std::string, RdkWindowManagerData>>& data);
```

### Firebolt Extension Listeners

```cpp
static bool addFireboltExtensionListener(
    const std::string& client, 
    std::shared_ptr<FireboltExtensionEventListener> listener
);
static bool removeFireboltExtensionListener(
    const std::string& client, 
    std::shared_ptr<FireboltExtensionEventListener> listener
);
static bool onFireboltExtensionEvent(
    RdkCompositor* eventCompositor, 
    const std::string& eventName
);
```

### Inactivity Management

```cpp
// Enable/disable inactivity reporting
static void enableInactivityReporting(bool enable);

// Set inactivity interval (minutes)
static void setInactivityInterval(double minutes);

// Reset inactivity timer
static void resetInactivityTime();

// Get current inactivity time (minutes)
static double getInactivityTimeInMinutes();
```

### Cursor Management

```cpp
static bool showCursor();
static bool hideCursor();
static bool setCursorSize(uint32_t width, uint32_t height);
static bool getCursorSize(uint32_t& width, uint32_t& height);
```

### Logging

```cpp
static bool setLogLevel(const std::string level);
static bool getLogLevel(std::string& level);
```

### Screenshot

```cpp
static bool screenShot(uint8_t*& data, uint32_t& size);
```

### AV Blocking (ERM)

```cpp
static bool setAVBlocked(std::string callsign, bool blockAV);
static bool getBlockedAVApplications(std::vector<std::string>& apps);
static bool isErmEnabled();
```

### Firebolt Surface Management

```cpp
// Convert surface to Firebolt surface
static bool getFireboltSurface(const std::string& client, 
                               int surfaceId, 
                               uint32_t type);

// Surface properties
static bool setFireboltSurfaceZorder(const std::string& client, 
                                    int surfaceId, 
                                    int zOrder);
static bool setFireboltSurfaceName(const std::string& client, 
                                  int surfaceId, 
                                  const std::string& surfaceName);
static bool setFireboltSurfaceOpacity(const std::string& client, 
                                     int surfaceId, 
                                     double opacity);
static bool setFireboltSurfaceBounds(const std::string& client, 
                                    int surfaceId, 
                                    int32_t x, int32_t y, 
                                    uint32_t width, uint32_t height);
static bool setFireboltSurfaceCrop(const std::string& client, 
                                  int surfaceId, 
                                  int32_t sx, int32_t sy, 
                                  uint32_t swidth, uint32_t sheight);
static bool setFireboltSurfaceVisibility(const std::string& client, 
                                        int surfaceId, 
                                        bool visible);
static bool fireboltSurfaceDestroy(const std::string& client, int surfaceId);
static bool getSurfaceInfo(const std::string& client, 
                          int surfaceId, 
                          FireboltSurfaceInfo& si);
```

### Rendering Control

```cpp
static bool enableDisplayRender(const std::string& client, bool enable);
static bool renderReady(const std::string& client);
```

### VNC Server

```cpp
static bool startVncServer();
static bool stopVncServer();
```

### Main Loop

```cpp
static bool draw();
static bool update();
```

---

## 4. RdkCompositor Class

**File:** `include/rdkcompositor.h`

### Constructor/Destructor

```cpp
RdkCompositor();
virtual ~RdkCompositor();
```

### Display Creation (Pure Virtual)

```cpp
virtual bool createDisplay(
    const std::string& displayName,
    const std::string& clientName,
    uint32_t width, uint32_t height,
    bool virtualDisplayEnabled,
    uint32_t virtualWidth, uint32_t virtualHeight,
    int32_t ownerId, int32_t groupId
) = 0;
```

### Rendering

```cpp
void draw(bool& needsHolePunch, RdkWindowManagerRect& rect, bool drawOverlays);
```

### Input Handling

```cpp
void onKeyPress(uint32_t keycode, uint32_t flags, uint64_t metadata);
void onKeyRelease(uint32_t keycode, uint32_t flags, uint64_t metadata);
void onPointerMotion(uint32_t x, uint32_t y);
void onPointerButtonPress(uint32_t keyCode, uint32_t x, uint32_t y);
void onPointerButtonRelease(uint32_t keyCode, uint32_t x, uint32_t y);
```

### Position & Size

```cpp
void setPosition(int32_t x, int32_t y);
void position(int32_t& x, int32_t& y);
void setSize(uint32_t width, uint32_t height);
void size(uint32_t& width, uint32_t& height);
```

### Visual Properties

```cpp
void setOpacity(double opacity);
void opacity(double& opacity);
void setVisible(bool visible);
void visible(bool& visible);
void setScale(double scaleX, double scaleY);
void scale(double& scaleX, double& scaleY);
void setAnimating(bool animating);
void setHolePunch(bool holePunchEnabled);
void holePunch(bool& holePunchEnabled);
void setCrop(int32_t cropX, int32_t cropY, int32_t cropWidth, int32_t cropHeight);
void crop(int32_t& cropX, int32_t& cropY, int32_t& cropWidth, int32_t& cropHeight);
```

### Virtual Display

```cpp
void getVirtualResolution(uint32_t& virtualWidth, uint32_t& virtualHeight);
void setVirtualResolution(uint32_t virtualWidth, uint32_t virtualHeight);
void enableVirtualDisplay(bool enable);
bool getVirtualDisplayEnabled();
```

### Application Lifecycle

```cpp
void closeApplication();
void launchApplication();
void setApplication(const std::string& application);
RdkWindowManager::ApplicationState getApplicationState();
```

### Event Listeners

```cpp
int registerInputEventListener(
    std::function<void(const RdkWindowManager::InputEvent&)> listener
);
void unregisterInputEventListener(int tag);

int registerStateChangeEventListener(
    std::function<void(uint32_t)> listener
);
void unregisterStateChangeEventListener(int tag);
```

### Other Methods

```cpp
void ownerId(int32_t& ownerId);
void keyMetadataEnabled(bool& enabled);
void setKeyMetadataEnabled(bool enable);
void displayName(std::string& name) const;
bool isKeyPressed();
void enableInputEvents(bool enable);
bool getInputEventsEnabled() const;
void setFocused(bool focused);
```

### Firebolt Surface Methods

```cpp
bool convertToFireboltSurface(int surfaceId, SurfaceType surfaceType);
bool setFireboltSurfaceZOrder(int surfaceId, int zOrder);
bool setFireboltSurfaceOpacity(int surfaceId, double opacity);
bool setFireboltSurfaceBounds(int surfaceId, int32_t x, int32_t y, 
                              uint32_t width, uint32_t height);
bool setFireboltSurfaceCrop(int surfaceId, int32_t sx, int32_t sy, 
                            uint32_t swidth, uint32_t sheight);
bool setFireboltSurfaceVisibility(int surfaceId, bool visible);
bool setFireboltSurfaceName(int surfaceId, const std::string& surfaceName);
bool fireboltSurfaceDestroy(int surfaceId);
bool hasOverlays();
bool hasCompositor(WstCompositor* compositor);
bool getSurfaceInfo(int surfaceId, FireboltSurfaceInfo& surfaceInfo);
bool setOwner(int ownerId, int32_t groupId);
bool enableDisplayRender(bool enable);
void setFirstFrameRendered(bool enable);
bool renderReady();
```

---

## 5. EssosInstance Class

**File:** `include/essosinstance.h`

### Singleton Access

```cpp
static EssosInstance* instance();
```

### Initialization

```cpp
void initialize(bool useWayland);
void initialize(bool useWayland, uint32_t width, uint32_t height);
void configureKeyInput(uint32_t initialDelay, uint32_t repeatInterval);
```

### Input Events

```cpp
void onKeyPress(uint32_t keyCode, unsigned long flags, uint64_t metadata);
void onKeyRelease(uint32_t keyCode, unsigned long flags, uint64_t metadata);
void onPointerMotion(uint32_t x, uint32_t y);
void onPointerButtonPress(uint32_t keyCode, uint32_t x, uint32_t y);
void onPointerButtonRelease(uint32_t keyCode, uint32_t x, uint32_t y);
void onDisplaySizeChanged(uint32_t width, uint32_t height);
```

### Main Loop

```cpp
void update();  // Process events and swap buffers
```

### Resolution

```cpp
void resolution(uint32_t& width, uint32_t& height);
void setResolution(uint32_t width, uint32_t height);
```

### Key Repeat

```cpp
void setKeyRepeats(bool enable);
void keyRepeats(bool& enable);
void ignoreKeyInputs(bool ignore);
```

### AV Blocking

```cpp
bool setAVBlocked(std::string app, bool blockAV);
void getBlockedAVApplications(std::vector<std::string>& appsList);
bool isErmEnabled();
```

---

## 6. Data Types

### ApplicationState

**File:** `include/application.h`

```cpp
enum class ApplicationState {
    Unknown,
    Running,
    Suspended,
    Stopped
};
```

### SurfaceType

**File:** `include/rdkwindowmanagertypes.h`

```cpp
enum SurfaceType {
    Standard = 1,      // Normal application surface
    Video = 2,         // Video playback surface
    Popup = 3,         // Popup/dialog surface
    Notification = 4   // Notification overlay
};
```

### LogLevel

**File:** `include/logger.h`

```cpp
enum LogLevel {
    Debug,
    Information,
    Warn,
    Error,
    Fatal
};
```

### InputEvent

**File:** `include/inputevent.h`

```cpp
struct InputEvent {
    uint32_t deviceId;
    uint32_t timestampMs;
    enum Type { InvalidEvent, KeyEvent, TouchPadEvent, SliderEvent } type;
    
    union Details {
        struct Key {
            int code;
            enum State { Pressed, Released, VirtualPress, VirtualRelease } state;
        } key;
        
        struct TouchPad {
            int x, y;
            enum State { Down, Up, Click } state;
        } touchpad;
        
        struct Slider {
            int x;
            enum State { Down, Up } state;
        } slider;
    } details;
};
```

### FireboltSurfaceInfo

**File:** `include/rdkcompositor.h`

```cpp
struct FireboltSurfaceInfo {
    WstCompositor* westerosCompositor;
    int surfaceId;
    SurfaceType surfaceType;
    int32_t x, y;
    uint32_t width, height;
    int32_t sx, sy;
    uint32_t swidth, sheight;
    double opacity;
    bool visible;
    int zOrder;
    std::string name;
};
```

### ClientInfo

**File:** `include/compositorcontroller.h`

```cpp
struct ClientInfo {
    int32_t x, y;
    uint32_t width, height;
    double sx, sy;
    double opacity;
    int32_t zorder;
    bool visible;
    int32_t cropX, cropY;
    int32_t cropWidth, cropHeight;
    int32_t ownerId;
};
```

### RdkWindowManagerData

**File:** `include/rdkwindowmanagerdata.h`

```cpp
class RdkWindowManagerData {
public:
    // Constructors for all types
    RdkWindowManagerData();
    RdkWindowManagerData(bool data);
    RdkWindowManagerData(int8_t data);
    RdkWindowManagerData(int32_t data);
    RdkWindowManagerData(int64_t data);
    RdkWindowManagerData(uint8_t data);
    RdkWindowManagerData(uint32_t data);
    RdkWindowManagerData(uint64_t data);
    RdkWindowManagerData(float data);
    RdkWindowManagerData(double data);
    RdkWindowManagerData(std::string data);
    RdkWindowManagerData(void* data);
    
    // Type conversion methods
    bool toBoolean() const;
    int32_t toInteger32() const;
    uint32_t toUnsignedInteger32() const;
    double toDouble() const;
    std::string toString() const;
    // ... etc
    
    // Assignment operators
    RdkWindowManagerData& operator=(bool value);
    RdkWindowManagerData& operator=(int32_t value);
    // ... etc
    
    std::type_index dataTypeIndex();
};
```

---

## 7. Event Listener Interfaces

### RdkWindowManagerEventListener

**File:** `include/rdkwindowmanagerevents.h`

```cpp
class RdkWindowManagerEventListener {
public:
    virtual void onApplicationConnected(const std::string& client) {}
    virtual void onApplicationDisconnected(const std::string& client) {}
    virtual void onApplicationTerminated(const std::string& client) {}
    virtual void onReady(const std::string& client) {}
    virtual void onUserInactive(double minutes) {}
    virtual void onKeyEvent(uint32_t keyCode, uint32_t flags, bool keyDown) {}
    virtual void onSizeChangeComplete(const std::string& client) {}
    virtual void onApplicationVisible(const std::string& client) {}
    virtual void onApplicationHidden(const std::string& client) {}
    virtual void onApplicationFocus(const std::string& client) {}
    virtual void onApplicationBlur(const std::string& client) {}
};
```

### FireboltExtensionEventListener

**File:** `include/rdkwindowmanagerevents.h`

```cpp
class FireboltExtensionEventListener {
public:
    virtual void on_focus(const char* clientName) {}
    virtual void on_blur(const char* clientName) {}
    virtual void client_connected(const char* clientName) {}
    virtual void client_disconnected(const char* clientName) {}
};
```

---

## 8. Logger Class

**File:** `include/logger.h`

```cpp
class Logger {
public:
    static void log(LogLevel level, const char* format, ...);
    static void setLogLevel(const char* loglevel);
    static void logLevel(std::string& level);
    static void enableFlushing(bool enable);
    static bool isFlushingEnabled();
    
    #ifdef RDK_WINDOW_MANAGER_LOGGER
    static void setLogFile(const std::string& filename);
    #endif
};
```

---

## 9. FrameBuffer Class

**File:** `include/framebuffer.h`

```cpp
class FrameBuffer {
public:
    FrameBuffer(int width, int height);
    ~FrameBuffer();
    
    int width() const;
    int height() const;
    GLuint texture() const;
    
    void bind();
    void unbind();
};
```

---

## 10. Event Constants

**File:** `include/rdkwindowmanagerevents.h`

```cpp
// Application events
const std::string RDK_WINDOW_MANAGER_EVENT_APPLICATION_CONNECTED;
const std::string RDK_WINDOW_MANAGER_EVENT_APPLICATION_DISCONNECTED;
const std::string RDK_WINDOW_MANAGER_EVENT_APPLICATION_TERMINATED;
const std::string RDK_WINDOW_MANAGER_EVENT_APPLICATION_FIRST_FRAME;
const std::string RDK_WINDOW_MANAGER_EVENT_APPLICATION_VISIBLE;
const std::string RDK_WINDOW_MANAGER_EVENT_APPLICATION_HIDDEN;
const std::string RDK_WINDOW_MANAGER_EVENT_APPLICATION_FOCUS;
const std::string RDK_WINDOW_MANAGER_EVENT_APPLICATION_BLUR;

// Other events
const std::string RDK_WINDOW_MANAGER_EVENT_USER_INACTIVE;
const std::string RDK_WINDOW_MANAGER_EVENT_KEY;
const std::string RDK_WINDOW_MANAGER_EVENT_SIZE_CHANGE_COMPLETE;

// Firebolt extension events
const std::string RDK_WINDOW_MANAGER_FIREBOLT_EXTENTION_EVENT_ON_FOCUS;
const std::string RDK_WINDOW_MANAGER_FIREBOLT_EXTENTION_EVENT_ON_BLUR;
const std::string RDK_WINDOW_MANAGER_FIREBOLT_EXTENSION_EVENT_CLIENT_CONNECTED;
const std::string RDK_WINDOW_MANAGER_FIREBOLT_EXTENSION_EVENT_CLIENT_DISCONNECTED;
```

---

## 11. Application MIME Types

**File:** `include/application.h`

```cpp
#define RDK_WINDOW_MANAGER_APPLICATION_MIME_TYPE_NATIVE "application/native"
#define RDK_WINDOW_MANAGER_APPLICATION_MIME_TYPE_DAC_NATIVE "application/dac.native"
#define RDK_WINDOW_MANAGER_APPLICATION_MIME_TYPE_HTML "application/html"
#define RDK_WINDOW_MANAGER_APPLICATION_MIME_TYPE_LIGHTNING "application/lightning"
```

---

## 12. Return Values

Most API functions return `bool`:
- `true` - Operation succeeded
- `false` - Operation failed (client not found, invalid parameters, etc.)

Check the log output for detailed error information when operations fail.

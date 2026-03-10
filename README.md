# RDK Window Manager

The window manager is responsible for creating Wayland displays,
application composition, managing windows, and input / focus handling.

## Overview

- **The RDKWindowManager component has dependencies on Westeros, Essos, and OpenGL ES 2.0.  RDKWindowManager uses Westeros to create a Wayland surface or display for applications to connect itself to. RDKWindowManager provides a set of APIs for the system or applications to control the display and the positioning of the application windows on screen. ​**
- **Separates window manager from application infrastructure on RDK platforms​**
- **It  is going to be an open-source component and would be available for the RDK ​**
- **The window manager runs inside a new Thunder plugin​**

## Thunder Interfaces
### APIs supported
- CreateDisplay — Creates display with given parameters.

- GetApps — Get the list of active clients.

- AddKeyIntercept — Register a key intercept for a specific key code and client.

- AddKeyIntercepts — Register multiple key intercepts in one call.

- RemoveKeyIntercept — Remove a key intercept.

- AddKeyListener — Register listeners for specific keys.

- RemoveKeyListener — Remove listeners for specific keys.

- InjectKey — Simulate a key press event with optional modifiers.

- GenerateKey — Generate a key event for specified keys and client.

- EnableInactivityReporting — Enable or disable inactivity reporting.

- SetInactivityInterval — Set the inactivity interval.

- ResetInactivityTime — Reset the inactivity timer.

- EnableKeyRepeats — Enable or disable key repeats.

- GetKeyRepeatsEnabled — Get the key repeats enabled status.

- IgnoreKeyInputs — Ignore key inputs.

- EnableInputEvents — Enable key input events for specified clients.

- KeyRepeatConfig — Configure key repeat parameters.

- SetFocus — Set focus to an app by ID.

- SetVisible — Set visibility of a client or app instance.

- RenderReady — Get the first-frame rendered status of an app.

- EnableDisplayRender — Enable or disable Wayland display rendering.

### Events supported
- OnUserInactivity — Notifies how long the user has been inactive (in minutes).

- OnDisconnected — Notifies when an application is disconnected.

- OnReady — Notifies when the first frame event is received for a client or app instance.

- OnConnected — Notifies when an application is connected.

- OnVisible — Notifies when an application becomes visible.

- OnHidden — Notifies when an application is hidden.

- OnFocus — Notifies when an application comes into focus.

- OnBlur — Notifies when an application loses focus (blurred).

## Wayland interfaces
Below are the Firebolt shell, surface, and wm APIs available using Wayland & Westeros Extensions.

## Interface: firebolt_shell
### Requests supported
- get_firebolt_surface - create a firebolt shell surface from a surface

### Events supported
- firebolt_video_surface_id - sent in reply to a get_firebolt_surface request for video surfaces

## Interface: firebolt_surface
### Requests supported
- destroy - destroy the firebolt_surface
- set_name - set the name of the firebolt_surface
- set_visible - set the visibility of the surface
- set_bounds - set the surface bounds
- set_crop - set the cropping of the surface within the surface
- set_zorder - set the relative z-order of the surface
- set_opacity - set surface opacity

### Events supported
- None

## Interface: firebolt_wm
### Requests supported
- set_properties — Updates various surface properties such as position, size, render size, opacity, z-order, visibility, and cropping for an app or group.

- create — Creates a new surface with default position, size, opacity, and visibility.

- create_with_bounds — Creates a new surface with specified position and size in pixel screen coordinates.

- create_with_properties — Creates a new surface with detailed properties: position, size, display size, opacity, z-order, visibility, cropping, and focus state.

- destroy — Destroys the specified display surface.

- set_client_bounds — Sets the client window bounds for rendering, without changing the Wayland display size.

- set_client_display_bounds — Sets and changes the size of a client’s Wayland display.

- set_client_focus — Sets the specified client to be the focused app.

- get_properties — Retrieves the current surface properties for the specified client or group.

- get_focused_client — Returns the ID of the currently focused client.

- get_clients — Returns a list of all active client IDs.

- set_owner — Assigns an owner ID to the specified app, typically to manage grouping or lifecycle control.

- get_owner — Retrieves the owner ID for the specified app.

### Events supported
- client_properties — Provides updated properties for a client, including position, size, opacity, z-order, visibility, cropping, and texture usage.

- focused_client — Sent in reply to a get_focused_client request. Returns the focused client ID. An empty string means no client is focused.

- clients — Sent in reply to a get_clients request. Returns a comma-separated list of client IDs.

- client_connected — Sent when an app is connected to a Wayland display.

- client_disconnected — Sent when an app is disconnected from a Wayland display.

- client_owner — Sent in response to a get_owner request. Returns the owner ID for a given app or client.
=======
# rdk-window-manager
Window Manager for RDK video and entertainment devices

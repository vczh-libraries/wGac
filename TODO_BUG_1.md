# Bug 1 — Default to the platform window frame

## Goal

In a normal, non-hosted wGac application, `ThemeName::Window` must use
`SystemFrameWindow` by default and a newly created `INativeWindow` must report:

```cpp
window->IsCustomFrameModeEnabled() == false
```

The platform frame must provide the title bar, close/minimize/maximize controls,
and resize border, subject to the other `INativeWindow` frame properties. This
must also work on a Wayland compositor that does not provide server-side
decorations.

Hosted mode is not part of this change. It must retain its framework-managed
custom-frame behavior.

## Confirmed analysis

There are two direct causes and one related provider-contract defect.

1. [WGacShared/gac_include.h](WGacShared/gac_include.h) sets
   `darkskin::Theme::PreferCustomFrameWindow` from
   `WaylandDisplay::PreferCustomFrameWindow()`. On GNOME/Mutter, where
   `zxdg_decoration_manager_v1` is unavailable or does not grant server-side
   decoration, this selects the GacUI custom template. iGac instead sets this
   preference to `false` for non-hosted applications.
2. [WGacNativeWindow.cpp](WGac/WGacNativeWindow.cpp) initializes
   `customFrameMode` from the same compositor probe. `DisableCustomFrameMode()`
   even refuses to report `false` when the decoration manager is unavailable.
   The compositor's decoration response can also rewrite the flag. This mixes
   two different facts:
   - whether GacUI requested its own custom template; and
   - whether the platform frame is drawn by the compositor or by a platform
     client-side decoration library.
3. [WGacController.cpp](WGac/WGacController.cpp) returns a forced-custom
   configuration for non-main windows. The current non-hosted
   FullControlTest path does not apply that configuration—the working
   `Customized Frame` checkbox demonstrates that it is not the direct cause of
   this regression. It is nevertheless an interface-contract defect:
   `GuiNativeWindow.h` requires a normal provider to return
   `NativeWindowFrameConfig::Default` for both main and non-main windows.
   Hosted mode has a separate controller that applies its own restrictions.

The existing setters for `Border`, `TitleBar`, `SizeBox`, `MinimizedBox`, and
`MaximizedBox` only update booleans. They do not update any native decoration.

A raw `xdg_toplevel` is not sufficient as the fallback. The
[xdg-decoration specification](https://wayland.app/protocols/xdg-decoration-unstable-v1)
states that a server-side request is only a preference: the compositor can
select client-side decoration, and a client must self-decorate when no
negotiation exists. The current probe therefore cannot guarantee a platform
frame. The [xdg-shell specification](https://wayland.app/protocols/xdg-shell)
defines the underlying toplevel role and configure lifecycle.

## Required fix

### 1. Make the GacUI-facing policy match iGac

- In non-hosted mode, set `PreferCustomFrameWindow` to `false` without making
  it depend on compositor decoration negotiation.
- Initialize a normal `WGacNativeWindow` with `customFrameMode == false`.
- Make `EnableCustomFrameMode()` and `DisableCustomFrameMode()` record the mode
  requested by GacUI immediately.
- `IsCustomFrameModeEnabled()` must report that requested GacUI mode. A
  compositor negotiation callback must never change it.
- Return `NativeWindowFrameConfig::Default` from both ordinary wGac frame-config
  getters.

The Windows DarkSkin default is intentionally not the reference here. Follow
iGac: normal `ThemeName::Window` uses the platform frame, while an explicitly
selected `CustomFrameWindow` still uses GacUI's template.

### 2. Add a real Wayland platform-frame fallback

Use `libdecor` for every normal Wayland toplevel. From the GacUI abstraction's
point of view, libdecor is the native/platform frame even when it draws
desktop-integrated client-side decorations:

- when the compositor grants server-side decoration, libdecor uses it;
- otherwise its GTK/Cairo plugin supplies the platform title bar, buttons, and
  resize border;
- the `wl_surface` owned by GacUI remains the application-content surface, so
  client-size calculations do not include decoration extents.

Create one libdecor context for the display and one `libdecor_frame` for every
normal `WGacNativeWindow`. Integrate its dispatch requirements with the existing
Wayland event loop. libdecor must be the sole owner of a normal window's
`xdg_surface`, `xdg_toplevel`, decoration listener, configure acknowledgement,
and role destruction; wGac must not create or acknowledge a second normal
surface role. Use libdecor's xdg getters where popup parenting or another
existing integration needs the underlying objects.

Route configure, close, map/unmap/recreation, title, parent, minimum content
size, maximize, minimize, and content commits through the libdecor frame.
Popups, tooltips, and menus must remain undecorated `xdg_popup`s.

Use frame visibility to switch dynamically:

- `customFrameMode == false` and a visible border/title bar: show the libdecor
  frame;
- `customFrameMode == true`: request that all libdecor/compositor decoration
  disappear so only the GacUI template is visible;
- `customFrameMode == false` with a borderless configuration: hide the platform
  frame without falsely reporting custom-frame mode.

`libdecor_frame_set_visibility(false)` is the normal switching mechanism, but
xdg-decoration v1 only makes a client-side request a hint after server-side
decoration has been selected. Do not assume that this call alone guarantees
suppression. The chosen lifecycle/role strategy must be verified to enforce
the one-frame invariant on supported compositors.

Whenever the effective configuration requests a frame, exactly one GacUI or
platform frame must be visible. An explicitly borderless configuration may
correctly show neither. If no usable libdecor plugin is available, startup
must fail with a clear diagnostic or another real platform-frame fallback must
be supplied; silently using a no-decoration fallback while reporting
`customFrameMode == false` is not acceptable.

Handle failures from `libdecor_new()`, `libdecor_decorate()`, and the
asynchronous `libdecor_interface::error` callback. A null/failed context or
frame, plugin load failure, or compositor-compatibility failure must take the
same clear-failure or functional-fallback path; never continue with a requested
platform frame and no working frame object.

Destroy all `libdecor_frame` objects before destroying the shared libdecor
context, and destroy the context before `wl_display_disconnect`.

The current decoration capability probe and
`prefer_custom_frame_window` state should be removed once libdecor owns this
negotiation. Do not retain them as a second source of truth.

### 3. Apply all `INativeWindow` frame properties

Centralize the effective platform-frame state and update it from every setter:

- `Border` and `TitleBar` control whether a titled platform frame is visible.
  It is acceptable to tie title-bar visibility to the bordered/titled state,
  as iGac does.
- `SizeBox` controls interactive resize capability and size constraints.
- `MinimizedBox` controls the libdecor minimize capability.
- `MaximizedBox` must be honored where the platform API allows it.
- Title, parent, minimum client size, and current size state must remain
  synchronized after repeated custom/system-frame switches. The close control
  and normal close callback must remain functional.
- `IconVisible` may be documented as unsupported if the selected Wayland frame
  backend has no equivalent, as long as the getter and documentation are
  consistent.

libdecor does not expose an independent public capability for hiding only the
maximize button; its maximize affordance is tied to resize/window-manager
capabilities. Test this explicitly. Do not silently disable resizing to hide
maximize when `SizeBox == true`; document the Wayland limitation if it cannot
be represented faithfully.

Expected update targets are:

- `WGacShared/gac_include.h`
- `WGacShared/CMakeLists.txt`
- `WGac/Wayland/WaylandDisplay.h`
- `WGac/Wayland/WaylandDisplay.cpp`
- `WGac/WGacController.cpp`
- `WGac/WGacNativeWindow.h`
- `WGac/WGacNativeWindow.cpp`
- `README.md`
- `README_CN.md`

Add the development package and a runtime libdecor plugin to the documented
Linux prerequisites. Do not modify `Import/`, generated `Apps/` files, or
GacUI sources for this task.

## Verification

All verification must be performed by the implementer without asking the user
to select a screenshot rectangle.

1. Stage any intentional new source files so the clean script cannot delete
   them, then build all targets with `./build.sh --rebuild`.
2. Launch `./test.sh --app:simple --unblock`.
   - Verify the automation endpoint remains responsive.
   - Inspect the created native window in a debugger or a temporary diagnostic
     and prove `IsCustomFrameModeEnabled() == false`; remove diagnostics before
     committing.
   - Use the XDG Desktop Portal `Screenshot` method with
     `interactive=false`, following
     `../Tools/Copilot/Guidelines/Running-ComputerUse.md`.
   - Verify one platform title bar, close/minimize/maximize controls, and a
     resize border are visible, and that the dark GacUI title bar is absent.
   - Verify the requested logical client size is unchanged by decoration
     extents.
3. Launch non-hosted FullControlTest. Through `/Controls` and `/IO`, open the
   Window Manager's `Open New Window` window and uncheck `Customized Frame`.
   Verify the same window gains a working platform frame rather than becoming
   borderless.
4. Toggle `Border`, `TitleBar`, `SizeBox`, `MinimizedBox`, and `MaximizedBox`
   one at a time. Capture an unattended full-desktop screenshot after each
   state and verify every representable property. Record any documented
   libdecor limitation rather than claiming unsupported behavior works.
   Explicitly test startup with the libdecor runtime plugin unavailable and
   require the documented clear failure or functional fallback.
5. Repeatedly switch `Customized Frame` off/on/off. Verify exactly one frame is
   visible at every step. Preserve the content `wl_surface`, logical client
   size, application state, and visible continuity without unintended
   resize/flicker; an internal role recreation is acceptable only if Wayland
   requires it and these observable invariants hold. Interactive custom-frame
   actions belong to Bug 2.
6. Exercise the platform frame with genuine desktop input, not MiniHTTP:
   title drag, edge resize, maximize/restore, minimize/restore, and close.
7. Repeat the default-frame test on the current GNOME Wayland session, because
   the no-server-side-decoration path is the essential regression. If a second
   compositor advertising `zxdg_decoration_manager_v1` is available, exercise
   it too and verify that server-side negotiation does not create a double
   frame.
8. Run hosted FullControlTest as a regression. Its virtual windows must retain
   the existing forced custom frames.
9. Because the display loop changes, run the native remote-renderer `/RPT` and
   `/FCT` workflows from `../GacUI/DebugRemoteProtocolWithNativeRenderer.md`,
   including replacement, takeover, and clean shutdown.
10. Stop every test process, core, and renderer; confirm ports are released,
    run `git diff --check`, and audit that no imported/generated file changed.

The task is not complete if it merely makes `IsCustomFrameModeEnabled()` return
false while leaving a borderless raw `xdg_toplevel`.

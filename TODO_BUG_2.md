# Bug 2 — Make custom-frame hit tests perform native window actions

## Goal

In non-hosted FullControlTest, the Window Manager can open a real subwindow and
switch its `Customized Frame` option at runtime. When the custom frame is
enabled:

- dragging the GacUI title bar must invoke a compositor-owned window move;
- dragging any sizing border/corner must invoke the matching compositor-owned
  resize;
- the GacUI minimize, maximize/restore, and close areas must perform their
  native window actions.

When the custom frame is disabled, Bug 1 must supply the working platform frame.
Hosted virtual-window behavior must not change.

## Confirmed analysis

The GacUI side is already complete:

- `FullControlTest/Resource.xml` changes `ControlThemeName` between
  `CustomFrameWindow` and `SystemFrameWindow`.
- `GuiWindow::SetNativeWindowFrameProperties()` calls
  `EnableCustomFrameMode()` or `DisableCustomFrameMode()`.
- DarkSkin's window template assigns `AssociatedHitTestResult` for the title,
  eight resize zones, icon, client area, and the three caption buttons.
- `GuiGraphicsHost::HitTest()` exposes the result through
  `INativeWindowListener::HitTest()`.
- `PerformHitTest()` is the required helper for combining multiple listeners.

[WGacNativeWindow.cpp](WGac/WGacNativeWindow.cpp) currently forwards ordinary
mouse movement and button events only. No wGac code calls `PerformHitTest()`,
maps a title/border to `xdg_toplevel_move()`/`xdg_toplevel_resize()`, or maps
caption-button hit tests to the existing window operations. The selected
custom template is therefore visible but inert.

Windows translates the same callback results to non-client `HT*` values so the
OS performs move/resize, and performs custom caption-button actions on release.
iGac evaluates the callback in native mouse handling and performs the
corresponding Cocoa operations. Hosted GacUI already has separate virtual
window hit-test handling; this task must not duplicate it.

The [xdg-shell specification](https://wayland.app/protocols/xdg-shell) imposes
an important constraint: interactive `move` and `resize` must be requested in
direct response to a real user action using that exact event's seat and serial.
The compositor may take pointer focus during the operation and later reports
the new size through configure events.

## Required fix

### 1. Add one native hit-test path

Add a `WGacNativeWindow` helper that evaluates:

```cpp
PerformHitTest(From(listeners), NativePoint(x, y))
```

Use it for a physical normal toplevel only when custom-frame mode is enabled.
Do not query hosted application state from the platform provider. Hosted
non-main virtual windows naturally return `NoDecision` through
`GuiHostedController::HitTest()` and remain handled internally. Continue
forwarding ordinary mouse callbacks so GacUI hover/pressed visuals and control
events remain correct.

Bug 3 also needs this result for resize cursors. Keep one shared helper and one
coordinate conversion. Bug 2 owns the result-to-libdecor-edge action mapping;
Bug 3 must continue using GacUI's existing `GetCursorFromHitTest()` rather than
duplicating cursor mapping here.

### 2. Preserve the correct Wayland serial

Split the ambiguous `WaylandSeat::last_pointer_serial` state:

- retain a pointer-enter serial for cursor requests, owned by Bug 3;
- retain the exact current pointer-button press serial for this task's
  move/resize requests;
- retain the latest eligible triggering user-action serial for popup grabs,
  clipboard/data-device requests, and other APIs that may be initiated by
  event kinds wGac currently supports, such as pointer or keyboard.

Record actual event order for the general user-action serial. Do not choose it
by numerically comparing Wayland serial values, which may wrap, and preserve
the existing clipboard/input-service behavior during the refactor.

Make the button serial available to `WGacNativeWindow::OnMouseButton()` as part
of the synchronous callback or an unambiguous accessor. Capture it before
calling application listeners so re-entrant code cannot replace it.

Before an interactive move/resize request, require a non-null seat, a nonzero
current press serial, pointer focus on this window, a mapped normal libdecor
frame, the left button, and `customFrameMode == true`.

Bug 1 must be completed first. Use `libdecor_frame_move()` and
`libdecor_frame_resize()` on that same frame so libdecor remains the single
toplevel owner. Do not create or bypass it with a second raw `xdg_toplevel`.

### 3. Map title and border presses

On a real left-button press:

| GacUI hit-test result | Wayland action |
| --- | --- |
| `Title` | interactive move |
| `BorderLeft` | resize `LEFT` |
| `BorderRight` | resize `RIGHT` |
| `BorderTop` | resize `TOP` |
| `BorderBottom` | resize `BOTTOM` |
| `BorderLeftTop` | resize `TOP_LEFT` |
| `BorderRightTop` | resize `TOP_RIGHT` |
| `BorderLeftBottom` | resize `BOTTOM_LEFT` |
| `BorderRightBottom` | resize `BOTTOM_RIGHT` |

Start a title move only when `TitleBar` is enabled. Start a border resize only
when `SizeBox` is enabled and the window is `Restored`; do not send one while
maximized. Do nothing for `BorderNoSizing`, `Client`, `Icon`, or `NoDecision`.

Do not emulate a move by changing `posX`/`posY`, and do not emulate resize by
repeatedly calling `SetBounds()`. Wayland does not expose global toplevel
placement; the compositor must own the grab, movement, and anchored edge. Apply
the resulting configure sizes through the existing native configure path.

### 4. Map caption-button releases

On a left-button release over the matching custom caption area, perform exactly
one action:

| GacUI hit-test result | Action |
| --- | --- |
| `ButtonMinimum` | `ShowMinimized()` when `MinimizedBox` is enabled |
| `ButtonMaximum` | `ShowRestored()` if maximized, otherwise `ShowMaximized()`, when `MaximizedBox` is enabled |
| `ButtonClose` | the normal close path (`Hide(true)`/`RequestClose`) |

Keep these actions on release, matching Windows and iGac. Caption actions do
not require a Wayland serial. They require the current custom-frame mode,
applicable box flag, and a matching recorded caption-button press.

On every new left press, first clear the previous caption target, then record
only `ButtonMinimum`, `ButtonMaximum`, or `ButtonClose`. On left release,
consume and clear the recorded value before performing at most one matching
action. Clear it on pointer/focus loss, hide, destroy, and custom-frame disable
so a compositor-owned grab that withholds the release cannot leave stale
state. Closing must still honor the normal `BeforeClosing` cancellation flow.

Do not keep a manual drag state after submitting an xdg/libdecor interactive
request. The compositor may remove pointer focus and is not required to return
the release event to the surface.

Expected update targets are:

- `WGac/WGacNativeWindow.h`
- `WGac/WGacNativeWindow.cpp`
- `WGac/Wayland/WaylandSeat.h`
- `WGac/Wayland/WaylandSeat.cpp`

Coordinate the shared serial names and hit-test helper with Bug 3. Do not modify
GacUI, `Import/`, generated `Apps/`, or protocol-generated source files. Title
double-click maximize and a title-bar context menu are outside this bug.

## Verification

The test must use genuine desktop pointer input for the native actions.
MiniHTTP `/IO` injects events at the GacUI listener layer, bypasses
`WaylandSeat::pointer_button()`, and has no compositor-issued pointer serial.
It may be used to arrange the UI, but it cannot prove this bug fixed. AT-SPI
semantic `do_action()` has the same limitation.

1. Stage any intentional new source files so the clean script cannot delete
   them, run `./build.sh --rebuild`, and start non-hosted FullControlTest with
   `WAYLAND_DEBUG=client`, retaining stderr as protocol evidence.
2. Use `/Controls` and `/IO` only to select Window Manager, click
   `Open New Window`, toggle the configuration checkboxes, identify native
   window IDs, and shut down cleanly.
3. Capture the full desktop through the portal `Screenshot` method with
   `interactive=false`; do not ask the user to select a rectangle.
4. Generate genuine desktop pointer events autonomously. On GNOME, first try
   `Atspi.generate_mouse_event()` with `abs`, `b1p`, intermediate `abs` motion,
   and `b1r`, and require it to return success and produce a real
   `wl_pointer.button` event. If the session does not support that path, use an
   already permitted RemoteDesktop/uinput input source or a nested compositor
   test harness. Never fall back to a MiniHTTP click and call it native
   verification.
5. With `Customized Frame` checked:
   - drag the title bar and prove the global window position changed in
     before/after unattended screenshots;
   - require both evidence that `libdecor_frame_move()` was called and the
     underlying interactive move request in the protocol log;
   - drag all eight edge/corner zones, prove the reported client size changed,
     and require each exact resize-edge request plus resizing configure events;
   - verify the compositor keeps the opposite resize edge anchored.
6. Click the custom maximize button once and verify maximized state. Take a
   fresh screenshot, recompute the button's desktop coordinates, then click it
   again and verify restored state. Confirm dimensions and the corresponding
   Wayland requests.
7. On disposable subwindows, click custom minimize and close. Minimize must
   remove only that subwindow from the desktop; close must remove it while the
   main window and automation endpoint remain responsive.
8. Verify caption press/release matching with genuine pointer input:
   - press outside a caption button, move over it, and release;
   - press a caption button, drag outside it, and release.
   Neither sequence may minimize, maximize, or close the window.
9. Turn `MinimizedBox` off and prove the custom minimize area cannot minimize.
   Turn `MaximizedBox` off and prove the custom maximize area cannot change
   size state.
10. Turn `SizeBox` off and verify border drags issue no resize request. Turn
   `TitleBar` off and verify the former title region issues no move request.
11. Uncheck `Customized Frame`. After Bug 1, a working platform frame must
   appear on the same window. Recheck it and verify custom-frame actions work
   again without recreating or losing the application window.
12. Run hosted FullControlTest as a regression. Dragging a virtual subwindow
    must move only that virtual window and must never issue a native move for
    the host toplevel.
13. Reject the result for a protocol error, wrong resize edge, stale-serial
    ignored operation, duplicated caption action, manual user input, or a
    remaining test process/port.
14. Run `git diff --check` and verify that `Import/`, generated `Apps/`, and all
    other out-of-scope files remain unchanged.

Directional hover cursors are verified by Bug 3, not by this task.

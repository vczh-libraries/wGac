# Bug 3 — Implement composition and custom-frame cursors on Wayland

## Goal

The visible pointer must follow the deepest applicable
`GuiGraphicsComposition`:

- editable text uses I-beam;
- ordinary controls use Arrow unless they request another cursor;
- links and other explicitly styled compositions use their requested cursor;
- a custom-frame sizing edge/corner uses the matching directional resize
  cursor and restores the composition cursor when the pointer leaves it.

The correct cursor must be applied on every pointer focus transition. Cursor
behavior must remain correct after clicking and after switching between custom
and platform frames.

## Confirmed analysis

GacUI already resolves composition cursors correctly.
`GuiGraphicsHost::MouseMoving()` finds the deepest visible composition, calls
`GetRelatedCursor()`, and calls `INativeWindow::SetWindowCursor()`, falling back
to `GetDefaultSystemCursor()`. `GetCursorFromHitTest()` already maps all eight
resize hit-test results to `SizeWE`, `SizeNS`, `SizeNWSE`, or `SizeNESW`.

wGac drops that contract in several places:

1. [WGacResourceService.cpp](WGac/Services/WGacResourceService.cpp) returns
   `nullptr` for every `GetSystemCursor()` call. Its cache is also sized as
   `SmallWaiting + 1`, which is one element, instead of
   `INativeCursor::SystemCursorCount`.
2. `WGacNativeWindow::SetWindowCursor()` only stores the pointer and never sends
   a Wayland cursor request.
3. [WGacCursor.h](WGac/WGacCursor.h) and
   [WGacCursor.cpp](WGac/WGacCursor.cpp) contain an unused
   `libwayland-cursor` theme/surface renderer. It is not an `INativeCursor`, is
   never instantiated, and has no connection to the resource service or
   window.
4. Physical pointer movement never evaluates the native listener hit test, so
   custom-frame resize zones cannot override the composition cursor.
5. `WaylandSeat::last_pointer_serial` is overwritten by both
   `wl_pointer.enter` and `wl_pointer.button`, then used by
   `wl_pointer_set_cursor`.

The last item is a protocol violation. The
[core Wayland specification](https://wayland.app/protocols/wayland)
requires `wl_pointer.set_cursor` to use the latest `wl_pointer.enter` serial;
any other serial is ignored. It also says the pointer image is undefined on
enter, so a client must explicitly apply the appropriate cursor then. This
explains both the failure to switch and the apparently permanent I-beam: it can
be stale cursor state inherited from the previously focused surface.

## Required fix

### 1. Provide real `INativeCursor` resources

Create one cached, non-null `INativeCursor` object for every value from zero
through `INativeCursor::SystemCursorCount - 1`. Each object must:

- return `true` from `IsSystemCursor()`;
- return its exact `SystemCursorType`;
- remain valid for the resource service's lifetime.

`GetDefaultSystemCursor()` must always return the cached Arrow object. Validate
the enum range before indexing the cache.

Keep these logical resource objects separate from the per-seat cursor renderer
and its one reusable cursor surface. The existing `WGacCursor` may be
renamed/refactored into that renderer, with a small `WGacSystemCursor` class
representing GacUI identity.

Map all current GacUI types:

- `SmallWaiting` and `LargeWaiting`
- `Arrow`
- `Cross`
- `Hand`
- `Help`
- `IBeam`
- `SizeAll`
- `SizeNESW`
- `SizeNS`
- `SizeNWSE`
- `SizeWE`

Use canonical Xcursor names with the existing legacy fallbacks. If a named
shape is missing, fall back to Arrow rather than returning null or retaining a
stale image. Match the platform convention used by Windows:
`SmallWaiting` is progress/app-starting (`progress`, then
`left_ptr_watch`) and `LargeWaiting` is wait/busy (`wait`, then `watch`).
Respect `XCURSOR_THEME`, `XCURSOR_SIZE`, output/buffer scale, and surface-local
hotspot scaling. Either schedule animated waiting frames correctly or
deliberately use a stable first frame; do not leave unused animation state that
appears functional.

The optional cursor-shape protocol is outside this bug. The existing
`libwayland-cursor` dependency is sufficient and avoids adding generated
protocol bindings.

### 2. Separate cursor and user-action serials

Replace `last_pointer_serial` with purpose-specific state:

- `pointerEnterSerial`, written only by `wl_pointer.enter` and used only by
  cursor shape/surface requests;
- the exact current pointer-button press serial, used by Bug 2 move/resize;
- a separately ordered latest eligible user-action serial for popup grabs,
  clipboard/data-device operations, and event kinds wGac currently supports,
  such as pointer or keyboard.

Do not determine the latest user action by numerically comparing unrelated
Wayland serial fields; serials may wrap. Record event order directly and
preserve `GetLastInputSerial()`/clipboard behavior during the refactor.

Only call `wl_pointer_set_cursor` while this seat's pointer is focused on the
target wGac surface, and always use the latest enter serial. A later click must
not invalidate cursor changes by overwriting that serial.

On every pointer enter, after storing the new enter serial, apply that window's
effective cursor explicitly. `SetWindowCursor()` must apply immediately if its
window currently owns pointer focus; otherwise it stores the composition
cursor for the next enter. It must call one centralized
`ApplyEffectiveCursor()` path that prefers the current border override over the
new composition value. `SetWindowCursor()` must not clobber a resize cursor
while a physical pointer still hovers over that border.

`SetWindowCursor(nullptr)` or an `INativeCursor` not created by this resource
service must fall back safely to Arrow. Do not crash on a cast or retain a stale
cursor.

### 3. Keep composition and border cursors separate

Store two logical values in `WGacNativeWindow`:

- the application/composition cursor set by `GuiGraphicsHost`;
- a transient custom-frame border override.

For real pointer enter/motion:

1. forward `MouseMoving` so GacUI updates the composition cursor;
2. when custom-frame mode and `SizeBox` are enabled, use the framework's
   existing `PerformHitTest(From(listeners), point)` through the same wGac
   coordinate/evaluation wrapper used by Bug 2;
3. call `GetCursorFromHitTest()` and apply the directional override for the
   eight sizing zones;
4. otherwise clear the override and restore the current composition cursor, or
   default Arrow.

Never replace the stored composition cursor with a border cursor. iGac keeps a
separate `borderOverrideCursor` specifically to avoid a restoration
desynchronization in hosted mode.

Clear a stale override on pointer leave, custom-frame disable, `SizeBox`
disable, maximize, teardown, and after an interactive compositor resize when
necessary. In platform-frame mode, wGac controls only the application-content
surface cursor; the platform decoration backend established by Bug 1 owns its
decoration cursors. During an active platform resize, the compositor may take
pointer focus and display its own cursor.

When pointer capability is removed/recreated, clear pointer focus, enter
serial, and renderer state before destroying the old `wl_pointer`. A destroyed
popup must not synthesize parent focus with its old focus surface/enter serial;
wait for a real parent enter or otherwise keep the target surface and serial
consistent, then reapply the parent's cursor. These paths must not use a stale
serial after popup destruction.

Load the cursor theme image at wGac's current effective output scale, set the
cursor surface buffer scale, and convert hotspot coordinates back to
surface-local units. Adding per-surface output membership and live multi-output
scale transitions is outside this cursor-switching bug.

Expected update targets are:

- `WGac/WGacCursor.h`
- `WGac/WGacCursor.cpp`
- `WGac/Services/WGacResourceService.h`
- `WGac/Services/WGacResourceService.cpp`
- `WGac/Wayland/WaylandSeat.h`
- `WGac/Wayland/WaylandSeat.cpp`
- `WGac/Wayland/WaylandDisplay.h`
- `WGac/Wayland/WaylandDisplay.cpp`
- `WGac/WGacNativeWindow.h`
- `WGac/WGacNativeWindow.cpp`

Own one reusable cursor renderer/surface per seat (wGac currently has one seat)
and destroy it before `wl_shm` and compositor dependencies. Coordinate the
shared serial and wGac hit-test wrapper with Bug 2. Do not modify GacUI,
`Import/`, generated `Apps/`, or generated protocol files.

## Verification

No cursor result may be inferred from a successful launch alone.

1. Stage any intentional new source files so the clean script cannot delete
   them, build all targets, and launch non-hosted FullControlTest.
2. Verify in a debugger or a temporary diagnostic that:
   - every `SystemCursorType` returns a distinct, non-null cached object with
     the correct reported type;
   - default cursor is the cached Arrow;
   - every applied effective cursor type is recorded correctly.
   Remove diagnostics before committing.
3. Use `/Controls` to locate representative controls and `/IO` with
   `!MouseMove` to exercise the GacUI-to-`SetWindowCursor()` path:
   - ordinary button/client area → Arrow;
   - editable text box → I-beam;
   - a hand-cursor composition, if present → Hand;
   - horizontal and vertical splitters → the requested sizing type.
   Require transitions in both directions, not just the initial image. First
   place the genuine desktop pointer over that native window if a real
   `wl_pointer.set_cursor` request is expected; without physical pointer focus,
   debugger-observed setter/effective-type evidence is the expected result.
4. For physical custom-frame hover, arrange a non-hosted Window Manager
   subwindow through MiniHTTP, then use a genuine desktop pointer source as
   described in `TODO_BUG_2.md`. MiniHTTP movement bypasses the Wayland input
   callback and cannot prove the border override.
5. Move over all eight sizing zones and verify the four directional classes:
   left/right → WE, top/bottom → NS, left-top/right-bottom → NWSE, and
   right-top/left-bottom → NESW. Move back to a button and editable text and
   verify Arrow/I-beam restoration.
6. Capture the full desktop without a rectangle prompt. Prefer a compositor
   screenshot API that includes the pointer. If the portal's
   `interactive=false` screenshot omits the cursor, pair it with debugger/type
   evidence instead of claiming the bitmap proves the shape.
7. Run with `WAYLAND_DEBUG=client`. After each `wl_pointer.enter`, verify every
   `wl_pointer.set_cursor` uses that enter serial, including after a button
   event. Move between the main window, subwindow, popup, and back; each entered
   surface must immediately select its own cursor with no inherited I-beam.
8. Verify the frame-state matrix:
   - custom frame plus `SizeBox` enabled has resize overrides;
   - `SizeBox` disabled, maximized, and any already-reported non-resizable
     backend state do not;
   - platform-frame mode preserves composition cursors in the client area;
   - repeated custom/system switches restore the right cursor.
9. Test a missing cursor name/theme fallback and, when available, start on an
   output scale greater than one. The cursor must remain visible, correctly
   sized, and correctly hotspotted.
10. Run hosted FullControlTest as a restoration regression. Also run at least
    one native remote-renderer `/RPT` or `/FCT` cursor smoke so a renderer DOM
    cursor/hit test reaches the real Wayland cursor.
11. Stop all test processes, cores, and renderers; confirm ports are released,
    run `git diff --check`, and audit repository scope.

Bug 3 owns cursor objects and effective hover selection. Bug 2 owns the actual
move, resize, minimize, maximize, and close actions.

# Bug 4 — Remove the remote window-creation handshake workaround

## Goal

Remove `WaitForControllerConnectBeforeWindowCreation` and its stored
`waitForControllerConnectBeforeWindowCreation` flag from GacUI's generic remote
protocol.

Creating an `INativeWindow` must remain non-blocking with respect to
`ControllerConnect`. The first operation that actually requires renderer state,
such as a screen query, bounds query, or `Show()`, must wait for and process the
connection correctly.

The result must support:

- an external renderer whose transport connects before its native window is
  ready;
- a direct or in-process renderer that sends `ControllerConnect` later from
  `GuiMain`;
- Windows, macOS, Wayland, and GacJS without renderer-specific startup policy
  in `IGuiRemoteProtocolConfig`;
- renderer replacement and takeover with complete state reconstruction.

This is a GacUI remote-protocol correction exposed by wGac. It is not a
Wayland-provider behavior change.

## Confirmed analysis

### The flag was added for a real startup failure

Commit `cf9700e37174585fc43d29fca2123daae55702f9`, titled
`Support Wayland native renderer workflow`, added two related changes:

1. `GuiRemoteController::EnsureControllerConnected()` changed from one
   submit/cycle attempt to a loop that processes remote events before
   submitting messages.
2. `WaitForControllerConnectBeforeWindowCreation()` was added to
   `IGuiRemoteProtocolConfig`, and `GuiRemoteController::CreateNativeWindow()`
   conditionally called `EnsureControllerConnected()` before creating the
   logical remote window.

The concrete failures observed while starting the Wayland remote renderer were:

```text
GuiRemoteProtocolFilter::ProcessResponses()
Messages sending to IGuiRemoteProtocol should be all responded.
```

and, in another failing startup:

```text
GuiRemoteRendererSingle::RequestWindowNotifySetClientSize(...)
This function cannot be called before RequestWindowNotifySetBounds.
```

These errors were consequences of connection ordering, not a Wayland window
API requirement.

### Transport connection is not protocol readiness

The core and renderer currently follow this sequence:

1. `RemotingTest_Core::WaitForRenderer()` returns when the network/channel
   server accepts a renderer client.
2. The renderer's `StartClient()` returns from `WaitForServer()`.
3. The renderer initializes its platform controller and creates its native
   renderer window.
4. `GuiRemoteRendererSingle::Opened()` sends `ControllerConnect`.
5. The core's UI thread processes that event, installs the renderer client ID,
   requests font and screen configuration, and initializes remote window and
   rendering state.

Step 1 does not imply that steps 4 and 5 have happened.

wGac made this gap reliably visible because `SetupRawWGacRenderer()` constructs
the native Wayland controller after the transport has connected.
`WaylandDisplay::Connect()` performs registry round trips and decoration
discovery before `GuiRawMain()` can create, register, and show the renderer
window. Windows and Cocoa normally reach `Opened()` more quickly. GacJS sends
`ControllerConnect` almost immediately after receiving its client ID. Those
timing differences do not define different remote-protocol contracts.

The flag is not actually Wayland-specific. The literal `true` is passed from
the common templated `RemotingTest_Core::StartServer()`, so the current core
enables it for named pipe, Windows HTTP, and MiniHTTP renderers. A current
GacJS client connected to that core is also subject to the core-side wait even
though GacJS has no corresponding switch.

### The old ensure operation did not ensure a connection

Before `cf9700e37`, `EnsureControllerConnected()` did this only once:

1. call `Submit()`;
2. call `RunOneCycle()`;
3. return whether or not `ControllerConnect` had arrived.

This order is unsafe for an asynchronous initial connection:

- `GuiRemoteProtocolCoreChannel` does not set `rendererClientId` until its
  `ControllerConnect` package is processed;
- window property setters can queue protocol state before that happens;
- `GuiRemoteProtocolCoreChannel::Submit()` consumes the pre-renderer batch and
  returns while the renderer ID is still `-1`;
- only the following `RunOneCycle()` gets a chance to process the connection;
- if the event has not arrived yet, the caller continues with default screen
  state or issues a synchronous request that cannot receive a response.

The pre-renderer discard must not automatically be changed to indefinite
packet retention. The remote controller keeps authoritative window/rendering
state and deliberately reconstructs it for a new renderer. Retaining arbitrary
incremental or render-diff packages across disconnect and replacement could
replay stale work or grow without bound.

The defensible part of `cf9700e37` is the new
`EnsureControllerConnected()` behavior:

- loop until connected or stopped;
- call `RunOneCycle()` first;
- let `ProcessRemoteEvents()` process `ControllerConnect`;
- install the renderer ID before submitting initialization requests.

### Waiting in `CreateNativeWindow()` is redundant and harmful

Operations that require renderer data already establish the correct boundary:

- `GuiRemoteController` screen bounds, client bounds, and scaling getters call
  `EnsureControllerConnected()`;
- `GuiRemoteWindow::RequestGetBounds()` calls it before its synchronous
  request;
- `GuiRemoteWindow::ShowWithSizeState()` calls it before sending `Show`;
- `GuiRemoteController::Run()` eventually calls `window->Show()`.

Creating and configuring the logical window before the renderer connects is
supported by the existing state model:

- setters retain the current title, frame flags, sizing flags, minimum size,
  capture state, and bounds;
- `GuiRemoteWindow::OnControllerConnect()` reconstructs state for a renderer
  that connects while the application is running;
- `GuiRemoteController::Run()` submits the complete retained window state when
  the renderer connected before the application entered the run loop;
- graphics, image, resource, hot-key, and DOM state have their own connection
  reconstruction paths.

Therefore, after retaining the corrected events-first loop, forcing the same
wait from `CreateNativeWindow()` adds no required synchronization. It instead:

- exposes a test-harness startup decision through the generic
  `IGuiRemoteProtocolConfig` interface;
- makes logical window creation capable of waiting forever for a renderer;
- requires a default-`false` escape hatch because direct/in-process protocols
  may intentionally send `ControllerConnect` later from `GuiMain`;
- applies an external-renderer workaround globally rather than expressing a
  protocol invariant.

### A wGac-only change is not a correct fix

`WGacNativeWindow::Show()` already raises `Opened()` synchronously after it
reaches the normal-window show path. wGac could reduce the failing interval by
pre-initializing Wayland before connecting MiniHTTP, but it cannot close the
race: the core can always resume after transport acceptance and before the
protocol event is processed.

Do not:

- add a sleep or startup delay to either process;
- make wGac report `Opened()` before the renderer window is actually ready;
- send a synthetic early `ControllerConnect`;
- fork the common native-renderer startup code solely to win a scheduling race;
- change ordinary wGac window lifecycle semantics to compensate for the core.

The framework must handle a delayed renderer deterministically.

## Required fix

### 1. Remove the protocol policy flag

In the owning GacUI repository:

- remove `IGuiRemoteProtocolConfig::WaitForControllerConnectBeforeWindowCreation()`;
- remove the forwarding overrides from both
  `GuiRemoteProtocolCombinator` specializations;
- remove the core-channel boolean field;
- remove the boolean constructor argument from
  `GuiRemoteProtocolCoreChannel`;
- remove the core-channel override that returns the boolean;
- remove the conditional wait from
  `GuiRemoteController::CreateNativeWindow()`;
- remove the literal `true` from
  `RemotingTest_Core::StartServer()`;
- update every constructor call to the restored signature.

Do not replace the flag with another renderer-, platform-, or transport-name
check. `CreateNativeWindow()` must not depend on connection timing.

Expected upstream source targets are:

- `GacUI/Source/PlatformProviders/Remote/GuiRemoteProtocol_Shared.h`
- `GacUI/Source/PlatformProviders/Remote/GuiRemoteProtocol_Channel_Json.h`
- `GacUI/Source/PlatformProviders/Remote/GuiRemoteProtocol_Channel_Json.cpp`
- `GacUI/Source/PlatformProviders/Remote/GuiRemoteController.cpp`
- `GacUI/Test/GacUISrc/RemotingTest_Core/GuiMain.cpp`

### 2. Preserve the actual connection-order fix

Keep `EnsureControllerConnected()` as a real readiness boundary:

- it must continue until `controllerConnected` or a real stop condition;
- it must process queued remote events before submitting messages;
- `ControllerConnect` must install the current renderer client ID before its
  callback sends font, screen, bounds, or state-reconstruction requests;
- synchronous screen and window requests must never proceed with an
  unprocessed connection;
- no busy loop may be introduced; retain bounded yielding between cycles.

Audit the stop path while adding the delayed-connection test. If a transport
that disconnects before sending `ControllerConnect` can leave the loop running
forever, propagate the existing transport/channel stop state to the controller.
Do not solve that case with an arbitrary short timeout or by restoring the
window-creation flag.

Do not blindly preserve all packages submitted while no renderer exists.
First-connect, disconnect, and replacement semantics must remain explicit:
authoritative state is reconstructed, while stale incremental batches are not
replayed into a replacement renderer.

### 3. Add deterministic delayed-connection coverage

Extend `GacUI/Test/GacUISrc/UnitTest/TestRemote_Startup.cpp` or add an equivalent
remote-startup unit fixture.

Use a controllable `IGuiRemoteEventProcessor` or asynchronous channel fixture
that withholds `ControllerConnect` for multiple `ProcessRemoteEvents()` calls.
Do not use wall-clock timing as the assertion mechanism.

The test must prove all of the following:

1. `CreateNativeWindow()` returns before `ControllerConnect` is delivered.
2. Pre-connect changes to title, client size/bounds, custom-frame mode, frame
   boxes, minimum client size, and another ordinary window property are
   retained.
3. The first screen query, bounds query, or `Show()` pumps the delayed event and
   waits until the controller is initialized.
4. Font and screen requests are routed only after the renderer ID is active.
5. No response ID remains unresolved and no renderer request arrives in an
   invalid order.
6. The renderer receives the final retained window state, followed by valid
   bounds/client-size and rendering state.
7. A direct protocol that calls `OnControllerConnect()` later from `GuiMain`
   still starts, proving that window creation contains no hidden wait.
8. Disconnect-before-connect exits through the documented stop path instead
   of hanging, if that lifecycle is supported by the fixture.

Also retain or extend replacement coverage to prove that discarding stale
pre-replacement increments and reconstructing the complete current state still
works.

### 4. Regenerate and import; never patch snapshots directly

After the upstream source and test change:

1. Run GacUI's Codepack workflow to regenerate:
   - `GacUI/Release/GacUI.h`
   - `GacUI/Release/GacUI.cpp`
2. Confirm the generated release diff matches the source removal.
3. In wGac, run `./import.sh` to refresh the committed import.
4. Confirm only the corresponding generated sections changed in:
   - `wGac/Import/GacUI.h`
   - `wGac/Import/GacUI.cpp`

Do not edit `wGac/Import` manually. No change is expected in
`WGacNativeWindow`, `WaylandDisplay`, or the wGac renderer entry point for this
bug.

If the suspicious-item note in `GacUI/ToDo/1.4.0.1.md` is still present, remove
or mark it resolved in the upstream commit.

## Verification

No visual judgment or interactive screenshot selection is required for this
bug. Verify protocol state, automation output, process lifecycle, and logs.

1. Before building, run a source audit:

   ```bash
   rg -n "WaitForControllerConnectBeforeWindowCreation|waitForControllerConnectBeforeWindowCreation" \
       GacUI wGac
   ```

   After source regeneration and import, there must be no implementation,
   declaration, constructor argument, or call left. A historical task document
   may mention the removed name.
2. Build and run the GacUI unit tests on Linux:

   ```bash
   (
     cd GacUI/Test/Linux/UnitTest
     ../../../.github/Ubuntu/build.sh
     ./Bin/UnitTest
   )
   ```

   Require the deterministic delayed-connection test, the existing startup
   tests, async channel tests, and replacement tests to pass.
3. Build the portable remote core and all wGac targets:

   ```bash
   (
     cd GacUI/Test/Linux/RemotingTest_Core
     ../../../.github/Ubuntu/build.sh
   )
   (
     cd wGac
     ./build.sh --rebuild
   )
   ```
4. Perform a deliberately delayed Linux startup without committing diagnostic
   code. Start the core first, then pause the renderer in a debugger after
   transport connection but before `SetupRawWGacRenderer()` reaches
   `GuiRemoteRendererSingle::Opened()`. Hold it long enough for the core to
   reach its readiness boundary, then resume it.
   - logical window creation must not deadlock;
   - startup must complete after `ControllerConnect`;
   - neither of the two original errors may appear;
   - both processes must remain responsive.
5. Run the complete Linux `/MiniHttp` native-renderer workflows from
   `GacUI/DebugRemoteProtocolWithNativeRenderer.md` for both `/RPT` and `/FCT`.
   - require valid core `/Controls` and renderer `/Dom`;
   - use renderer `/IO` so input and rendering cross the protocol;
   - verify title, bounds/client size, frame mode, visible controls, and
     rendering state after startup;
   - do not accept `Queued` alone without the expected state change.
6. With the core still running, verify renderer replacement and takeover.
   Preserve application state across both operations, require the new renderer
   to reconstruct the current DOM and window configuration, and require the
   old renderer to detach without a fatal prompt or retry loop.
7. Repeat startup at least 20 times with varied temporary pauses before
   `ControllerConnect`. Reject any missing response, invalid request ordering,
   hang, early process exit, or frozen automation endpoint.
8. Exercise GacJS against the updated core. Its immediate JavaScript
   `ControllerConnect` path must still start without any special core flag.
   Verify one control-tree/DOM interaction and clean shutdown.
9. On available Windows and macOS builders, compile and run their native
   renderer workflows:
   - Windows: `/Pipe`, `/Http`, and `/MiniHttp`;
   - macOS: `/MiniHttp`.

   At minimum, require startup, one renderer-side input round trip, replacement,
   and clean shutdown. This catches constructor-signature or shared-protocol
   regressions that a Linux-only build cannot see.
10. Stop every core, renderer, debugger, and browser helper; confirm ports
    `8888` and `8889` are released. Run `git diff --check` in both GacUI and
    wGac, and audit that no unrelated, generated application, or Wayland
    provider file changed.

The task is not complete if it merely moves the wait to another early startup
function or makes wGac connect faster. A renderer delayed after transport
acceptance must remain a supported, deterministic case.

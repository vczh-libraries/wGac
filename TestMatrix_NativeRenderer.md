# Test Matrix Card 2026-10-21T11:19:05.560Z

## Test Matrix

| Test Item | 1st |
| --- | --- |
| [Linux][Test_CppTest_Rvm][`/MiniHttp`] | PASS |
| [Linux][`/RPT`][`/MiniHttp`] | PASS |
| [Linux][`/FCT`][`/MiniHttp`] | PASS |
| [Linux][`/RVMT`][`/MiniHttp`] | PASS |
| [Linux][`/RVMT`][`/MiniHttp /Cli:<path>`] | PASS |
| [Linux][FCT standalone] | PASS; compositor-resize limitation below |
| [Linux][FCT hosted] | PASS |
| [Linux][Import / resource sync / build] | PASS: import, sync and build (dependency rootfs) |
| [Linux][Native service and Raw Clock unit tests] | PASS: 13/13 cases across five binaries |
| [Linux][Native Touching / TouchingList acquisition] | |

## Issues Found and Fix

## Observations and limitations

- Native Touching/TouchingList acquisition is excluded: wGac does not provide native touch input. Synthetic Core input cannot establish hardware support. Raw Clock and ordinary input remain in scope. iGac/macOS is not run on this Linux host.

- The host build failed because the Pango include directory is absent. The prerequisite script already includes `libpango1.0-dev`; this is an incomplete host installation, not a missing script dependency. The same official build passed in the existing Ubuntu dependency rootfs.

- Standalone/hosted FCT: two lists, two editor markers and tab retention, local shortcuts, all five mouse buttons and Alt/Super plus Ctrl/Shift, movement/double-click and both wheel axes, layout subpages and palette/state retention, responsive modal selection/open/close and Raw Clock hide/show/theme/data updates. Hosted modal required moving it left within the physical client before widening; measured 240/350/440/600 by 320 plus 600 by 220/520 and reverse cycles showed all four appearances. Standalone automation cannot start compositor resizing without a real pointer serial. Raw painter unit tests separately confirm pixels and data-only repaint.
- Compositor-injected Ctrl+Shift+Alt+Super+Q while a separate GNOME Terminal was active produced exactly `You pressed Ctrl+Shift+Alt+Win+Q!` in the hosted native app. This is distinct from renderer IO.

- Native RPT: label activation, local shortcuts, five mouse buttons/modifiers, grid add/clear, document modal, disconnect/reconnect and live takeover preserved state, confirmed normal close. Fresh fatal run retained exact fatal text and rejected ordinary input until renderer Exit. Normal renderer Exit asks Core to close; renderer termination was used for the detach/reconnect check.

- Native remote FCT: list/editor continuity, all five mouse buttons and modifier combinations, both local shortcuts, layout/palette retention, modal widths 240/350/440/600 and heights 220/320/450 with reverse cycles, normal close/reopen, Raw Clock updates/hide/show/palette, detach/reconnect and live renderer takeover, normal shutdown.
- All three native RVM rows: initial and edited greetings, network second-host rejection, normal requester/host teardown, idle loss and loss after suspending the accepted host during an edit. Remote renderers retained exactly `RemotingTest_RvmHost disconnected.` until Exit; standalone requester terminated nonzero as specified.

- Final official import, resource synchronization and rootfs-backed build all passed again; the five freshly built native unit executables again passed 13/13 cases. Existing default empty point callbacks satisfy the native interfaces; no additional native acquisition stubs were needed.

# Test Matrix Card 2026-09-05 Linux TUI port

## Test Matrix

| Test Item | 1st |
| --- | --- |
| [Linux][Test_CppTest_Rvm][`/MiniHttp`] | 2026-09-05 01:48 PDT |
| [Linux][`/RPT`][`/MiniHttp`] | 2026-09-05 01:18:32 PDT (fixed) |
| [Linux][`/FCT`][`/MiniHttp`] | 2026-09-05 01:36:57 PDT |
| [Linux][`/RVMT`][`/MiniHttp`] | 2026-09-05 01:43 PDT |
| [Linux][`/RVMT`][`/MiniHttp /Cli:<path>`] | 2026-09-05 01:47 PDT |

## Issues Found and Fix

### RPT, first run: renderer startup

An early renderer `/Dom` GET deterministically terminated the Wayland renderer
with `vl::Error`. GDB caught JSON conversion of an uninitialized
`WindowSizingConfig::sizeState`: main-window registration had not populated the
cached sizing configuration before automation became available. Fixed the
owning GacUI renderer's registration boundary and added a startup-DOM regression.
The regression reproduced the unsupported-enum exception before the fix and
passed afterward. Regenerated GacUI and wGac imports; immediate startup polling
and the retained Core-authored fatal path now pass. Normal RPT operations,
renderer replacement/takeover, and application-controlled shutdown also pass.

### Additional standard FullControlTest check

The standard (non-hosted) local showcase terminated with `vl::Error` immediately
after closing the Ctrl+Q dialog. GDB traced the failure to native-window ID
validation during a control-tree dump. Shared multiwindow automation checked
control visibility rather than `GuiControlHost::GetOpening()`, so it could include
a closed dialog after its native window had been destroyed but before deferred
C++ deletion. Fixed the shared visibility filter, retained strict wGac pointer
validation, and added hidden/open/closed/destroyed-window regression coverage.
After regenerating imports and rebuilding, both standard and hosted local
FullControlTest passed list operations, bracket/brace and rich-text typing,
retained editor contents, both shortcut dialogs, the complete mouse/modifier
checks, and clean application-controlled exit.
The simple Hello World app also exposed its expected control tree and exited
cleanly through automation.

## Coverage

All five Linux matrix rows passed. RPT/FCT included feature workflows, separate
Alt/Super readouts, all five mouse buttons and double clicks, movement, both wheel
axes, renderer replacement/takeover, retained state, and normal shutdown. RVM
covered host ownership, replacement, network/stdio operation, child cleanup,
and both idle-host-loss and delivery-acknowledgement-loss fatal paths. Fatal
checks required the exact Core-authored error, retained renderer DOM, rejection
of ordinary input, and a working `!Exit`.

Input was delivered through the renderer's MiniHTTP automation service and
verified against application controls and the active rendering DOM; this is not
a claim of physical Wayland keyboard/mouse coverage. The SOP's Windows global
hotkey check is not applicable on Linux. Final GacUI tests passed 88/88 files and
1,729/1,729 cases, including both new automation regressions.

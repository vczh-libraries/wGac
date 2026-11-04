# Test Matrix Card 2026-11-04T06:38:46.156Z

## Test Matrix

| Test Item | 1st |
| --- | --- |
| Build / import / resource sync | 2026-11-04T10:41:55.567Z (passed) |
| Native provider unit tests | 2026-11-04T10:41:55.567Z (passed; lifecycle/raw 5/5) |
| Desktop global shortcuts / consent | 2026-11-04T10:49:00Z (passed; activation and consent cancellation) |
| Local Hello World | 2026-11-04T09:15:54.589Z (passed after configure-order fix) |
| Local FCT standard | 2026-11-04T10:07:49.789Z (passed; accepted-file result fix verified) |
| Local FCT hosted | 2026-11-04T09:31:16.289Z (passed) |
| Native Touching / TouchingList |  |
| [Linux][Test_CppTest_Rvm][`/MiniHttp`] | 2026-11-04T08:01:34.922Z (passed) |
| [Linux][`/RPT`][`/MiniHttp`] | 2026-11-04T10:44:16Z (passed; three final repetitions) |
| [Linux][`/FCT`][`/MiniHttp`] | 2026-11-04T10:43:20.793Z (fixed; passed) |
| [Linux][`/RVMT`][`/MiniHttp`] | 2026-11-04T08:01:34.922Z (passed) |
| [Linux][`/RVMT`][`/MiniHttp /Cli:<path>`] | 2026-11-04T08:01:34.922Z (passed) |

## Observations and Limitations

- Linux verification used GNOME Shell 50.1 on Wayland and libdecor 0.2.5. Native wGac touch acquisition is excluded by the job; the retained touch row remains blank. Linux results do not establish macOS coverage. Renderer automation, actual desktop keyboard input and terminal input are recorded separately.
- Full GacUI unit suite passed 95/95 files and 1965/1965 cases, including Unicode event and TUI provider regressions. CodePack, wGac import, resource sync and the official build passed against the rebased source. Evidence: `<GacUI repo>/.github/TaskLogs/rp-20261103/gacui-unit.log` and `wgac-visibility-*.log`.
- AsyncService, ImageService, GlobalShortcuts and RenderingLifecycle suites passed. RawElement passed standard/hosted 4/4 each after upstream integration; the final standard run passed 5/5 including the new repeated-visibility regression. Evidence: `<GacUI repo>/.github/TaskLogs/rp-20261103/Test_*.log` and `native-visibility-test.log`.
- Hello World displayed its expected title, 480×320 client area and welcome label, then closed with native Alt+F4 and exit 0. Its final Wayland trace commits the first content buffer with the configure acknowledgment, without fresh Mutter geometry or premature-content warnings. Evidence: `<GacUI repo>/.github/TaskLogs/rp-20261103/native-hello-configure-final.log`.
- Standard and hosted FCT passed lists, independent Unicode editor markers, local shortcuts, five-button modifier checks, vertical rail and palette retention, nine responsive sizes, scrolling/account selection/three close actions, live Raw Clock pixels/data and normal exit 0. Standard-window resizing used real GNOME input; the hosted viewport was enlarged before testing 680-pixel children. Evidence: `<GacUI repo>/.github/TaskLogs/rp-20261103/native-fct-standard-results.json` and `native-fct-hosted-results.json`.
- Final remote FCT passed the same full workflow, ticking Raw Clock data and pixels before/after palette replacement, renderer replacement and live takeover, and normal Core/renderer exit 0. Both replacement renderers repeated the real desktop global shortcut and extended-button checks. Evidence: `<GacUI repo>/.github/TaskLogs/rp-20261103/native-fct-remote-results.json` and `native-fct-remote-visibility-final2.log`.
- Native RPT passed Home, both local shortcuts and Super labels, five buttons with modifier combinations, movement/double-click/both wheel axes, all nine grid cells/clear, Document modal, renderer replacement/takeover and confirmed File close. Core and renderers exited 0. Desktop global Q passed with Kitty focus explicitly established, including after replacement/takeover. One earlier run ended with an unhandled Core error; its complete LLDB repeat and three further final-build repetitions passed, so the isolated failure was not reproduced in five attempts. Its cause is not attributed to the separately confirmed FCT renderer bug. Evidence: `<GacUI repo>/.github/TaskLogs/rp-20261103/native-rpt-final-1.log`, `native-rpt-final-2.log` and `native-rpt-final-3.log`.
- Native RPT fatal handling preserved exact `This is a fatel error!`, nonzero Core termination and the retained window/DOM/labels; renderer cleanup exited 0. Four button hover colors changed when the fatal button was clicked, while the scene remained intact.
- All nine native RVMT combinations of standalone/network/stdio and normal/idle-host-loss/blocked-RPC-host-loss passed. Repeated RPC included CJK and supplementary characters; network admission retained the original host; renderer replacement/takeover preserved RPC. Normal requesters/renderers exited 0 and stdio children were reaped. Lost hosts caused prompt nonzero termination with exact `RemotingTest_RvmHost disconnected.` Evidence: `<GacUI repo>/.github/TaskLogs/rp-20261103/native-rvm-*.json`.
- Actual desktop global Q activated standard and remote FCT/RPT while another terminal held focus, with matching portal Activated signals. Concurrent fresh TUI F8 and native FCT Q registrations both worked. A separate temporary app identity displayed the real GNOME shortcut-consent dialog with the exact Q chord; AT-SPI clicked Cancel, and GNOME returned portal response 2. The global Q action stayed inactive, ordinary local Ctrl+Q and both ten-item lists still worked, and FCT exited normally with code 0. Existing consent was preserved and the temporary desktop file was removed. Evidence: `<GacUI repo>/.github/TaskLogs/rp-20261103/native-portal-consent.png`, `native-portal-consent-atspi.json`, `native-portal-cancel-results.json` and `portal-live.log`.

## Issues Found and Fix

- A Wayland trace and Mutter journal confirmed premature decoration commits before the first configure acknowledgment. The native window now defers initial libdecor state/visibility until configure and suppresses premature decoration commit callbacks. The fresh Hello trace and RenderingLifecycle regression passed.
- Standard FCT reproduced canceled-file result loss: accepted alpha.txt disappeared after reopening/canceling and refreshing the palette. Both FCT and TUI pages now copy accepted filenames. Fresh native verification retained alpha.txt after two canceled pickers and Aurora/Default template rebuilds, then exited 0. The independent TUI regression also passed. Evidence: `<GacUI repo>/.github/TaskLogs/rp-20261103/native-file-copy-final.json`.
- Remote FCT twice raised `Text is only allowed on first update.` LLDB observed 14 renderer Opened callbacks in one unchanged visible window. SetActivate calls Show, which repeated Opened and restarted the remote connection. Show now ignores repeated requests while visible. Repeated Show/ShowDeactivated/SetActivate and hide/reopen transitions passed the new regression; the full remote FCT workflow and three final RPT repetitions passed after the official rebuild.
- The clock harness now reads baseline time after capturing its first pixels and then requires both a later time and changed pixels. A prior pre-capture time could accept an already-painted tick and compare the same frame twice. This was a verification correction, not a product fix.

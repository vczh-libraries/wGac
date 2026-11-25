# Test Matrix Card 2026-11-25T10:34:24.109Z

## Environment

| Field | Value |
| --- | --- |
| Platform / application / commit / build configuration | Ubuntu 26.04.1 / GNOME 50 / Test_TuiControlTest / Debug x64; sync/build and 2019/2019 unit suite passed |
| Terminal name / version / profile / viewport sizes | GNOME Terminal 3.58.0, VTE 0.84.0, xterm-256color; 80×25, 100×30, 120×40 PTY replay. Kitty 0.45.0 comparison at 80×25 and 120×40 |
| Input methods / desktop availability | Owned PTY raw-byte replay and cell capture; Kitty-generated keyboard input and owned-window pixel capture. AT-SPI/XTest attempts delivered no native keys; GNOME GlobalShortcuts portal present |

Input revisions: GacUI `6f192cacc`, wGac `ea438a1`, GacJS `db7e1bdd`. Evidence: `<wGac repo>/build/rpxplat-20261125`.

## Test Matrix

| Test Item | 1st |
| --- | --- |
| Build / resource generation / import / sync | 2026-11-25T11:54:55.354Z |
| Unit tests | 2026-11-25T10:48:33.040Z |
| Platform launch and terminal/input capabilities | 2026-11-25T11:37:34.447Z |
| Startup, platform services and teardown | 2026-11-25T11:37:34.506Z |
| Follow-up regression checks | 2026-11-25T12:29:40Z |
| List / TextList | 2026-11-25T11:04:18.337Z |
| List / ListView | 2026-11-25T11:04:18.373Z |
| List / TreeView | 2026-11-25T11:03:46.996Z |
| List / BindableDataGrid | 2026-11-25T11:33:05.163Z |
| Refresh List / TextList | 2026-11-25T10:55:42.686Z |
| Refresh List / BindableTextList | 2026-11-25T10:55:51.088Z |
| Refresh List / ListView | 2026-11-25T10:56:07.785Z |
| Refresh List / BindableListView | 2026-11-25T10:56:27.918Z |
| Refresh List / TreeView | 2026-11-25T10:57:10.823Z |
| Refresh List / BindableTreeView | 2026-11-25T10:57:18.425Z |
| Refresh List / BindableDataGrid | 2026-11-25T10:56:48.048Z |
| Layout / Repeat / RepeatStack | 2026-11-25T10:58:24.803Z |
| Layout / Repeat / RepeatFlow | 2026-11-25T10:58:46.379Z |
| Layout / Repeat / SharedSize (RepeatFlow) | 2026-11-25T10:59:07.954Z |
| Layout / Repeat / SharedSize (TextList) | 2026-11-25T10:59:29.551Z |
| Layout / Responsive | 2026-11-25T11:02:49.599Z |
| Control / Document Editor (Toolstrip) | 2026-11-25T12:29:40Z |
| Control / TextBox / TextBox | 2026-11-25T11:24:02.675Z |
| Control / TextBox / TextBox (No Tab) | 2026-11-25T11:39:41.061Z |
| Control / TextBox / Document | 2026-11-25T11:39:40.941Z |
| Control / TextBox / Document (No Tab) | 2026-11-25T12:00:44.688Z |
| Control / Embedded Controls | 2026-11-25T11:41:19.519Z |
| Misc / Localization | 2026-11-25T11:41:19.589Z |
| Compact dialog and form layout checks | 2026-11-25T12:29:40Z |
| Misc / Dialogs / MessageDialog | 2026-11-25T11:08:26.809Z |
| Misc / Dialogs / ColorDialog | 2026-11-25T12:29:40Z |
| Misc / Dialogs / FontDialog | 2026-11-25T11:15:42.516Z |
| Misc / Dialogs / Open/Save FileDialog | 2026-11-25T12:29:40Z |
| Misc / DatePicker | 2026-11-25T11:35:56.051Z |
| Window Manager | 2026-11-25T12:29:40Z |
| Live palette and clipping regression | 2026-11-25T11:36:16.902Z |
| Exit | 2026-11-25T11:29:45.517Z |
| Packaged tutorial checks (when requested) |  |
| Linux/macOS legacy-terminal and Kitty comparison (when applicable) | 2026-11-25T12:13:04Z |
| TUI F8 portal registration and cleanup | 2026-11-25T12:15:56Z |
| Native terminal keyboard/mouse delivery and outside-focus global activation |  |
| Standalone Alt overlays and SGR mouse Super |  |
| Mouse4/Mouse5 terminal delivery |  |
| Live showcase interval-eight configuration |  |

## Observations and Limitations

Linux only. Packaged Windows tutorial checks are outside this job and remain blank. Timestamps cover the available checks described below; unavailable input paths have separate blank rows. No earlier verification results are carried forward.

GNOME Terminal was attempted first. Owned PTY replay supplied the live application and cell-buffer evidence. AT-SPI later identified a fresh owned terminal capture window as focused and active, and accepted string and keycode generation, but the capture received zero bytes. XTest also delivered no bytes to the verified Kitty window. Therefore physical keyboard/mouse delivery and outside-focus global activation remain unverified. GNOME ScreenSaver reported `false`; no confirmed desktop lock is claimed. Evidence: `<wGac repo>/build/rpxplat-20261125/native-atspi-input.json`, `<wGac repo>/build/rpxplat-20261125/native-key-capture.json`, and `<wGac repo>/build/rpxplat-20261125/kitty-xtest-ctrl-q.json`.

The desktop GlobalShortcuts portal was present, version 1, with `SupportedCapabilities=15`. A fresh TUI run received response 0 for CreateSession and BindShortcuts with `CTRL+SHIFT+ALT+LOGO+F8`, then ShortcutsChanged with the matching trigger. Normal application exit sent Session.Close. No consent/denial window was exposed through the available desktop interfaces; activation remains unverified. Native FCT's Q binding was separately accepted and cleaned up, as recorded in the native card. Evidence: `<wGac repo>/build/rpxplat-20261125/desktop-evidence/tui-portal-f8-gedbus.vt`.

Concurrent native FCT startup also registered its distinct Q chord and closed normally while the independently owned TUI stayed alive. This establishes concurrent startup/registration only. Evidence: `<wGac repo>/build/rpxplat-20261125/concurrent-fct-tui-result.json` and `<wGac repo>/build/rpxplat-20261125/concurrent-fct-tui-portal.log`.

GNOME Shell Screenshot D-Bus returned `org.freedesktop.DBus.Error.AccessDenied`, and root-window PIL capture failed with X get_image error 8. The owned Kitty X11 window was successfully captured directly and visually inspected: the Pink palette, selected radio and complete legible frame were visible. This does not establish GNOME Terminal appearance or physical input. Standalone Alt overlays and SGR mouse Super are unavailable in the POSIX adapter; locale resource translations work while date/number formatting remains en-US.

Current-build PTY replay evidence: Lists/TextList and ListView, BindableDataGrid, all Refresh List variants, all four Repeat/SharedSize pages, Responsive, TreeView, DatePicker, Window Manager, live palette/clipping, MessageDialog, ColorDialog, FontDialog, Open/Save FileDialog, Localization, compact dialog layouts, Embedded Controls, Toolstrip editor command/clipboard, and TextBox/document marker and resize checks passed where marked above. The reusable probes initially assumed a preselected subpage; focused wrappers were corrected to select the owning tab first. The current Toolstrip private fixture is present at `<wGac repo>/build/rpxplat-20261125/fct-standard-Toolstrip-saved.bin`; after the clipboard handoff, cross-open/reload and RTF+HTML export produced `<wGac repo>/build/rpxplat-20261125/tui-edited-saved.bin`, `<wGac repo>/build/rpxplat-20261125/tui-export.rtf`, and `<wGac repo>/build/rpxplat-20261125/tui-export.html`, with cancellation and palette-retention captures.

Additional SOP audit: a fresh owned legacy run asserted literal TAB geometry at columns 4/4/4/8 (`<wGac repo>/build/rpxplat-20261125/tui-tabs-four-verified-current.json`), exact 45-line wide Unicode paste/selection/clipboard roundtrip (`<wGac repo>/build/rpxplat-20261125/tui-wide-paste-exact.json`), all four TextBox/Document variants including tab policy, selection, undo/redo, resize and retention (`<wGac repo>/build/rpxplat-20261125/tui-textbox-results.json`), and embedded RemoteProtocolTest combo retention after resize/page return plus the exact dismissible `Pretend to be starting!` dialog (`<wGac repo>/build/rpxplat-20261125/tui-embedded-combo-retained.json`, `<wGac repo>/build/rpxplat-20261125/tui-embedded-right-now-dialog.json`). Toolstrip edit/clipboard/undo/redo and font effects, accept/cancel/reopen, locale and both viewport sizes were asserted. Image insert/delete/undo/private reload reached `[Image, 123 x 456]` after deleting the original bitmap (`<wGac repo>/build/rpxplat-20261125/tui-editor-file-image.json`, `<wGac repo>/build/rpxplat-20261125/tui-editor-memory-image.json`). Five fresh Purple→SkyBlue cycles retained those dimensions at 120×40, with ten validating captures (`<wGac repo>/build/rpxplat-20261125/tui-image-purple-*.json`, `<wGac repo>/build/rpxplat-20261125/tui-image-skyblue-*.json`).

All six direct/queued Hide/Close/Stop normal-exit paths passed in fresh runs, including cancellation where applicable, restored termios, and usable shell input. Kitty also exited normally after queued Close, restored all termios settings and ran a same-PTY shell marker. Owned bridges and terminal windows were closed after verification. Evidence: `<wGac repo>/build/rpxplat-20261125/tui-exit-results.json`, `<wGac repo>/build/rpxplat-20261125/kitty-restoration.json`.

The live showcase exposes no tab-interval setting, so interval-eight interactive coverage remains blank. The passing `TUI configured tabs share paragraph and label geometry` case in `<GacUI repo>/Test/GacUISrc/UnitTest/TestTuiProvider.cpp` explicitly iterates intervals four and eight, including CJK/emoji, caret positions, selection styling, alignment, clipping, wrapping, inline objects, labels and ellipsis. `TUI scalar offsets, tab stops, CRLF, wrapping and caret sides` and `TUI registered renderers reuse label paragraphs and layouts` provide separate text/cache coverage. These unit cases supplement the live interval-four editing, undo and clipboard checks.

Compact Message, Color, simple Font, full Font and file dialogs were inspected at 120×40 and 80×25. Ten captured modal layouts passed an independent geometry audit: complete frames, no extra interfield blank rows, exactly one action-gap row, centered message buttons, right-aligned other actions, and Color labels aligned with bordered textbox contents. File launch forms scroll at the smaller viewport. An already-open 74×20 file modal retains its old origin after shrinking; the documented title drag moves it into the viewport. The first drag used incorrect zero/one-based coordinates and no-button motion, so it did not test window movement. Correct held-left SGR motion exposed both actions. Evidence: `<wGac repo>/build/rpxplat-20261125/tui-compact-geometry-audit.json`, `<wGac repo>/build/rpxplat-20261125/tui-file-fresh-80-dialog-settled.json`, and `<wGac repo>/build/rpxplat-20261125/tui-file-resize-after-title-drag-correct.json`.

The moved Open dialog canceled successfully, reopened at 80×25 with retained filename/filter, and the restored 120×40 main frame repainted completely. A separate small Save dialog displayed both Save/Cancel and canceled normally. Evidence: `<wGac repo>/build/rpxplat-20261125/tui-compact-open-cancel-dismissed.json`, `<wGac repo>/build/rpxplat-20261125/tui-compact-open-reopened-80-retained.json`, `<wGac repo>/build/rpxplat-20261125/tui-compact-restored-120-full-frame.json`, and `<wGac repo>/build/rpxplat-20261125/root-tui-save-80-open.json`.

RGB tracker verification inspected VT cell attributes: each track occupies exactly one row, with a distinct one-cell handle and unpainted neighboring rows. All three handles dragged to 0 and 255; focused Left/Right changed 0↔1 and 254↔255. Text entry of 12,34,56, acceptance/reopen and change/cancel retained the expected value. The earlier text-only probe had missed the colored-space handles and is superseded by these assertions. Evidence: `<wGac repo>/build/rpxplat-20261125/root-tui-tracker-geometry.json`, `<wGac repo>/build/rpxplat-20261125/root-tui-tracker-results.json`, and `<wGac repo>/build/rpxplat-20261125/root-tui-final-results.json`. The final application exit was 0 with identical before/after termios; its bridge also exited normally.

Final `<wGac repo>/import.sh` → `<wGac repo>/syncProj.sh` → `<wGac repo>/build.sh` succeeded in order. The full GacUI unit suite passed 2,019 cases across 97 files, including TUI provider/layout regressions. Evidence: `<wGac repo>/build/rpxplat-20261125/final-import.log`, `<wGac repo>/build/rpxplat-20261125/final-sync.log`, `<wGac repo>/build/rpxplat-20261125/final-build.log`, `<wGac repo>/build/rpxplat-20261125/unit-test.log`.

## Issues Found and Fix

No reproducible product defect was established. The retained image failure capture has fewer input events and predates the successful private-reload capture (last input 12:04:56 versus 12:06:12 UTC). It therefore does not record a later dimensions-to-filename regression. Five fresh palette cycles and the smaller-viewport sequence below passed. The earlier single failure remains in the evidence; its underlying cause is not established. Evidence: `<wGac repo>/build/rpxplat-20261125/tui-image-capture-chronology.json`.

The image also retained `[Image, 123 x 456]` through a fresh 120×40 → 80×25 → Purple → SkyBlue → 120×40 sequence with the original bitmap absent. Evidence: `<wGac repo>/build/rpxplat-20261125/tui-image-original-120.json`, `<wGac repo>/build/rpxplat-20261125/tui-image-original-80.json`, `<wGac repo>/build/rpxplat-20261125/tui-image-purple-80.json`, `<wGac repo>/build/rpxplat-20261125/tui-image-skyblue-80.json`, and `<wGac repo>/build/rpxplat-20261125/tui-image-restored-120.json`. The last capture preceded full frame repaint, so it establishes image retention only.

- Current evidence probes originally assumed the owning top-level/subtab was already selected; wrappers were corrected for Refresh, Repeat/Layout, Responsive, TreeView and Grid. This was a harness issue, not a product failure.
- DatePicker was rerun against the current live calendar. Two independent combo selections committed distinct values (`DC1=2026-11-10`, `DC2=2026-11-18`); leap-year February committed `DP1=2024-02-29`; the December-to-January boundary committed `DP1=2024-12-31` then `2025-01-01`. Switching to List and back retained all four readouts (`DP1=2025-01-01`, `DP2=2026-11-25`, `DC1=2026-11-10`, `DC2=2026-11-18`), and 80x25/120x40 resize captures completed. Evidence saves include `tui-date-dc2-distinct`, `tui-date-leap-february-29`, `tui-date-december-31`, `tui-date-january-boundary`, `tui-date-persistence-return`, `tui-date-resize-80x25`, and `tui-date-resize-120x40`.
- Window Manager and live palette probes were rerun from fresh explicitly selected pages with child-window, frame-option, palette, and resize captures.
- Follow-up replay passed expanded DataGrid combo navigation/commit, selected-cell separator attributes, and standard SGR mouse paths; the five-button portion stopped reproducibly at Mouse4 (`TUI expected Mouse4 button down!`). This is retained as an unverified platform capability, not a product fix.
- Window Manager limitation: SGR replay verified ordinary mouse paths, but Mouse4/Mouse5 did not produce application messages in this provider; no native five-button delivery is claimed.

## Terminal Comparisons

### Terminal comparison: local shortcuts and terminal restoration

| Field | Required evidence |
| --- | --- |
| Host / build / configuration | 2026-11-25, Ubuntu 26.04.1/GNOME 50, current synchronized Debug x64 Test_TuiControlTest; 120×40 → 80×25 → 120×40 |
| Legacy terminal attempted first | GNOME Terminal 3.58.0/VTE 0.84.0, xterm-256color, existing profile. Owned PTY replay worked; native key generation delivered no bytes. Profile shortcut interception and physical modifier release order could not be verified |
| Operation and expected result | Window Manager: Ctrl+Q must show `You pressed Ctrl+Q!`; Ctrl+Alt+Super+Q must show the existing literal `You pressed Ctrl+Alt+Win+Q!`; Ctrl+Alt+Q must not trigger that Super dialog. Dismiss, resize, use queued Close, and verify restored shell input |
| Legacy result | Application behavior passed by explicitly tagged raw-byte replay. Focused AT-SPI string/keycode generation yielded zero captured bytes, so legacy terminal-generated Super delivery remains unavailable |
| Kitty retry after failure | Kitty 0.45.0, forced X11, `--config NONE`, `-o allow_remote_control=yes`, software GL. `kitten @ send-key` exercised the actual Kitty encoder: Ctrl+Alt+Q emitted `ESC[113;7u` without the Super dialog; Ctrl+Alt+Super+Q emitted `ESC[113;15u` and the exact dialog. Ctrl+Q also passed. Raw pointer replay selected Pink; owned-window pixels showed that palette. Both resize dimensions, normal exit 0, termios equality and same-PTY shell input passed |
| Conclusion | Kitty-generated shortcuts passed and distinguished Alt from Alt+Super. Legacy terminal-generated and physical keyboard comparison remains unavailable. Raw PTY replay, Kitty encoder commands and displayed-window capture are separate evidence; none establishes outside-focus global activation |
| Evidence / cleanup | `<wGac repo>/build/rpxplat-20261125/kitty-check.log`, `<wGac repo>/build/rpxplat-20261125/kitty-final.log`, `<wGac repo>/build/rpxplat-20261125/kitty-input.json`, `<wGac repo>/build/rpxplat-20261125/kitty-pink.png`, `<wGac repo>/build/rpxplat-20261125/kitty-restoration.json`. Owned Kitty, bridge and application closed normally |

The first Kitty attempt omitted remote-control permission and therefore created no socket. Retrying with the explicit option above succeeded; the earlier timeout was a setup error, not evidence that Kitty could not run. Evidence: `<wGac repo>/build/rpxplat-20261125/desktop-evidence/kitty-forced-x11.log`.

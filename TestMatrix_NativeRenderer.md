# Test Matrix Card 2026-11-16T16:19:01.369Z

Input revisions: GacUI `78b86996`, wGac `485a57a4`, GacJS `3f89bfc4`. Evidence directory: <wGac repo>/build/rpxplat-20261116.

## Test Matrix

| Test Item | 1st |
| --- | --- |
| [Linux][Test_CppTest_Rvm][`/MiniHttp`] | 2026-11-16T16:38:51.458Z |
| [Linux][`/RPT`][`/MiniHttp`] | 2026-11-16T16:36:22.880Z |
| [Linux][`/FCT`][`/MiniHttp`] | 2026-11-16 (passed; full walkthrough plus fresh lifecycle run) |
| [Linux][`/RVMT`][`/MiniHttp`] | 2026-11-16T16:42:33.895Z |
| [Linux][`/RVMT`][`/MiniHttp /Cli:<command>`] | 2026-11-16T16:44:07.651Z |

## Local applications and build

| Test Item | 1st |
| --- | --- |
| CodePack / import / sync / build | 2026-11-16 (passed; final import, sync and full build also passed) |
| Native service unit tests | 2026-11-16T16:32:54.183Z |
| Local Hello World | 2026-11-16T16:34:09.966Z |
| Local FullControlTest standard | 2026-11-16 (fixed; service checks passed, desktop limitations below) |
| Local FullControlTest hosted | 2026-11-16 (passed; service input, desktop limitations below) |
| Native touch acquisition / Touching / TouchingList |  |

## Observations and Limitations

Linux only. Native touch rows are excluded because wGac has no native touch provider. Raw Clock and mouse/keyboard checks remain in scope. Windows, macOS and Wasm are outside this run. No earlier results are carried forward.

## Completed observations

The official import/sync/build passed. Native service suites passed: AsyncService 2, ImageService 2, GlobalShortcuts 4, RenderingLifecycle 2, ClipboardService 3, RawElement standard 8 and hosted 8 cases. Hello World exposed its expected title/label and exited normally through automation.

Native RPT completed Home, exact local-shortcut dialogs and Super labels, all five mouse buttons with modifiers, move/double-click/both wheel axes, three-row grid add/clear, document modal, replacement and takeover, repeated input checks, cancel/accept File close, and the fresh intentional fatal case. Core and renderers exited normally for normal shutdown; the intentional fatal preserved the exact DOM error and rejected normal input until exact `!Exit`. Evidence directory: `<wGac repo>/build/rpxplat-20261116/`; corresponding files are `unit-*.log`, `hello-controls.json`, `rpt-native.log`, `native-rpt-results.json`, and `native-rpt-fatal.json`.

Desktop screenshot service returned `AccessDenied: Screenshot is not allowed`. AT-SPI could inspect and focus the owned GNOME Terminal, but generated keyboard input produced no terminal bytes. Desktop-delivered global shortcut activation and physical input remain unverified; automation input is recorded separately.

Local RVM over MiniHTTP and remote RVM with native network and native stdio hosts passed exact greeting/RPC changes, normal shutdown, idle host loss and blocked-RPC host loss. Remote runs also passed renderer replacement/takeover; network runs rejected a second host while the accepted host remained usable. Stdio normal shutdown reaped its child. Every native renderer host-loss case retained exactly `RemotingTest_RvmHost disconnected.` in its fatal DOM. The stdio harness was adjusted to wait for the queued character to enter RPC before probing blocked Controls; no product change was needed. Logs and structured evidence: `<wGac repo>/build/rpxplat-20261116/rvm-{local,native}-*`.

## Issues Found and Fix

### Native clipboard failure

Two service-driven Ribbon attempts selected `(0,0)-(0,5)` (`GacUI`), copied, moved to the document end and pasted; the document stayed unchanged. The native clipboard writer created a Wayland source, but its reader depended entirely on an incoming compositor offer; `ContainsDocument` was also unimplemented. The test session had no usable desktop input serial. Fixed and rebuilt; detailed results follow below. Evidence: `<wGac repo>/build/rpxplat-20261116/fct-selection-controls.json`, `<wGac repo>/build/rpxplat-20261116/fct-context-controls.json`.

The clipboard fix now keeps a process-local snapshot of text, rich document and image data, supplies text/image fallbacks for rich copies, and tracks a unique Wayland offer identity so a foreign owner retires local data. A copy is readable locally even without a compositor input serial. Three focused ClipboardService tests and the official full build passed; rebuilt standard Ribbon and Toolstrip passed exact clipboard edits, formatting, images, private reload with the source image removed, and actual RTF/HTML image exports. Native document XML omits natural image dimensions unless overridden; the HTML export verifies the decoded 123×456 size.

Standard FCT also passed layout-page and palette retention, selected document/range retention, compact responsive-modal data and eight-account menu, repeated fresh 240×320 openings with OK/Cancel and restored owner input, and continued Raw Clock updates after palette changes. Service-driven native resize did not change the OS modal dimensions; wide/short native modal appearances and native title-bar close remain unavailable in this desktop session. Detailed logs are `fct-standard-*.log` in the evidence directory.

The unit-test snapshot audit resolved element references in every changed frame. File-dialog rendered trees were identical despite identity/cache and asynchronous frame-number changes. Supplementary-character differences were only UTF-32 versus UTF-16 caret offsets. The run patch and semantic audit were retained in the evidence directory; platform-specific snapshot churn was restored.

### Delayed tooltip protocol failure

Repeated standard Ribbon-gallery runs exited normally while the main window was still open. GDB showed `libdecor_dispatch` returning `-32` (`EPIPE`), and peeking the unread Wayland error recovered exactly `xdg_surface destroyed before constructed`. `CreateXdgSurface` allocated a child xdg surface before checking whether its popup parent still had a role; when the menu had closed, cleanup destroyed the unconstructed surface and the compositor disconnected the application. A focused live-compositor regression now exercises a delayed tooltip with an unmapped menu parent before the ordering fix. Evidence: `fct-standard-exit-debug.log`, `fct-standard-protocol-debug.log`, and the gallery attempt logs in the evidence directory.

The new compositor regression failed against the original implementation and passes after moving the parent-role check before child xdg-surface allocation. The official build and both RenderingLifecycle cases pass. Five consecutive rebuilt gallery walkthroughs passed, including all eight styles, repeat application, scrolling and sample menus. This fix is separate from the clipboard fix.

Standard FCT completed normal queued Close. The external X11 clipboard owner replaced the local rich copy with exact Unicode text. TUI private saves cross-opened in both native GUI editors with identical full document XML, including the complete style catalog and three embedded images. Standard and hosted additional commands cover equivalent edit/toolbar operations, link submenu/ribbon changes and removal, exact Preview URL activation, read-only rejection, five context separators, viewport-edge menus and Alignment expansion. Hosted responsive cycling reached actual widths 240, 350, 440 and 644 (the physical viewport bounded the requested 680), with all four intended appearances, independent list scrolling, short/tall heights, shrink/grow repetition, reachable footer and title-bar close.

AT-SPI sees the native application with zero accessible children; the GlobalShortcuts portal reports version 1 but no showing consent dialog was accessible. Desktop screenshot access was denied. Native desktop global activation, OS window resize and physical input remain unavailable; service and hosted-window results do not establish those paths.

Native remote FCT completed both rich editors, files/images, gallery, all equivalent command groups, layout/palettes and responsive cycles. A fresh lifecycle run passed replacement and concurrent takeover with full document continuity, repeated local shortcuts/five-button modifier checks and normal queued Close of Core/renderer. Clear preserves the list sequence counter: after takeover Add produced 10–19, correctly retaining state; the harness originally expected 0–9 and was corrected. Evidence: `fct-native-matrix.log`, `native-fct-lifecycle.log` and `native-fct-matrix-result.json`.

Final `./import.sh`, `./syncProj.sh` and `./build.sh` all completed successfully in that order. The rebuilt ClipboardService (3/3) and RenderingLifecycle (2/2) regressions passed. GacUI Debug x64 passed 97/97 files and 2008/2008 cases; generated Linux test inputs now include the existing editable-editor test file. Final build and regression logs use the `-final.log` suffix in the evidence directory.

The final read-only command pass succeeded in rebuilt standard native, hosted native, native remote and Firefox surfaces. Preview and Selectable reject hyperlink edit/removal and Ribbon style application without changing the document; returning to Editable restores both operations. Every run closed normally. Native popup Escape is addressed to the popup window; hosted/browser outside dismissal uses a visible main-window control. Evidence: `readonly-matrix.log` and `fct-*-readonly-style.json` in the evidence directory.

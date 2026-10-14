# Test Matrix Card 2026-10-14T13:54:35.543Z

Scope: complete current Linux TUI SOP; native terminal input and byte replay are recorded separately. Times use UTC.

## Test Matrix

| Test Item | 1st | Evidence / limitations |
| --- | --- | --- |
| Import / sync / build | 2026-10-14T16:11:15.530Z (fixed) | Final CodePack/import/sync/official build passed after the DarkSkin include, Wayland draw-lifetime and TUI portal-diagnostic fixes. Build used a disposable Ubuntu container with the unchanged documented prerequisites. |
| Unit tests | 2026-10-14T16:11:15.530Z | GacUI: 89/89 cases in five focused files (TUI, EasyLayout, hosted windows and Alt navigation). Native: 9/9 cases, including draw-buffer lifetime and quiet TUI portal denial. |
| Native terminal input | 2026-10-14T16:07:06.512Z | GNOME Terminal 3.58/VTE 0.84 passed compositor-generated Ctrl+Q; local Super failed with 1b11. Kitty 0.45 passed the same chord with CSI 113;15u. Real portal F8 and concurrent CppTest global Q passed. See terminal comparison; physical input and displayed fidelity remain unverified. |
| Terminal byte replay | 2026-10-14T16:07:06.512Z | CSI/SGR replay and VT cell capture covered every available showcase page, local decoder shortcuts, three mouse buttons, modifier readouts, both wheel axes and 120x40/80x25/restored layout. This is separate from terminal-generated host delivery. |
| Linux and macOS launch | 2026-10-14T16:07:06.512Z | Linux foreground launch and fresh 100x30/80x25 initial bounds passed. --unblock, --hosted and renderer-only --port were rejected. iGac script mirror was pushed and syntax-checked; macOS execution was not performed on Linux, as permitted by the user. |
| Rules for every operation | 2026-10-14T16:07:06.513Z | Checks used visible targets and settled state, mouse/keyboard paths, page-return retention and CJK/supplementary content. Harness retries corrected partial VT frames, stale modal bounds and wheel targets outside resized list interiors. Inherited fake-service and terminal limits are recorded below. |
| Startup, platform services and teardown | 2026-10-14T16:11:15.530Z (fixed) | Fresh 100x30/80x25 bounds, hosted children/modals, external X11 clipboard, async file enumeration, interior/final caret blink and real global shortcuts passed. Actual consent denial preserved the final binary's frame; denied global activation stayed inactive. Closing while consent was pending and normal approved shutdown restored full termios and shell input. |
| Follow-up regression checks | 2026-10-14T16:06:15.598Z | Live checks covered sorting/header resize, every grid separator cell, editor arrow/selection and combo navigation, TAB columns 4/4/4/8 plus wide characters, literal-TAB clipboard, long rich/plain paste with scrolling and undo/redo, and interior/final caret blinking. Interval eight, caption variants, queued popup/modal palette refresh and wide-glyph clipping also passed focused provider tests. POSIX standalone Alt overlays are unavailable. |
| List / TextList | 2026-10-14T14:30:09.289Z | Fresh 0–9, 0–19, positional odd/even removals and continued counters passed across fresh runs. Text/Check/Radio, independent item checks, exclusive Dummy radios, source rotation, keyboard selection and resize retention passed. |
| List / ListView | 2026-10-14T15:12:13.632Z | All 29 source filenames matched both panes after widening File columns; six requested views remained Detail. Mouse/keyboard selection, scrolling, source rotation, header resize and 80/120-column relayout passed. |
| List / TreeView | 2026-10-14T15:16:07.452Z | Exact 6/4/4/4 children matched both panes. All roots expanded/collapsed, keyboard navigation, source rotation and resizing passed using settled disclosure glyphs. |
| List / BindableDataGrid | 2026-10-14T15:16:38.248Z | Five records, all five editors, three-state sorting/ties, exact 3/4/2/5 default filter results and changed date bound passed. Rotation retained edits; six fallback views and DataGrid, resize and per-cell separator backgrounds passed. |
| Refresh List / TextList | 2026-10-14T14:34:36.779Z | Programmatic/direct checking and model readback passed. |
| Refresh List / BindableTextList | 2026-10-14T14:34:53.324Z | Name/title mapping retained model check state and Read agreed under both mappings. |
| Refresh List / ListView | 2026-10-14T14:37:23.328Z | Main/subitem mutation, all four DataColumns projections and header collection states passed. Detail columns are checked separately from the projection status. |
| Refresh List / BindableListView | 2026-10-14T14:38:22.080Z | Name/title, live subitem binding, all four projections and header reinsertion passed. |
| Refresh List / TreeView | 2026-10-14T14:39:22.335Z | Root/child live mutation, collapsed mutation and expansion retention passed. |
| Refresh List / BindableTreeView | 2026-10-14T14:39:25.795Z | Both mappings, live/collapsed mutation and retained child hierarchy passed. |
| Refresh List / BindableDataGrid | 2026-10-14T14:40:06.898Z | Mapping, live subitem binding, projections, column collection and File cell editing passed. |
| Layout / Repeat / RepeatStack | 2026-10-14T14:41:04.473Z | Complete reset/add/remove/clear/continued-counter sequence passed on this independent model. Additional batches scrolled to thirty-nine; surrounding scroll/tracker controls were exercised, and only the horizontal tracker changed progress. |
| Layout / Repeat / RepeatFlow | 2026-10-14T14:41:11.787Z | Complete reset/add/remove/clear/continued-counter sequence passed on this independent model. Forty labels wrapped at 120/80 columns, retained order and reached the last item; natural button widths differed with text length. |
| Layout / Repeat / SharedSize (RepeatFlow) | 2026-10-14T14:41:19.095Z | Complete reset/add/remove/clear/continued-counter sequence passed on this independent model. Forty long labels shared uniform widths and wrapped; clear/reset to short labels reduced the shared width. |
| Layout / Repeat / SharedSize (TextList) | 2026-10-14T14:41:26.406Z | Complete reset/add/remove/clear/continued-counter sequence passed on this independent model. Longer labels, right anchoring, selection, scrolling and resized state were retained. |
| Layout / Responsive | 2026-10-14T14:42:48.190Z | View/Stack/Group boundaries 4/7/4 and migrated editor marker passed; resize is not an automatic-level test. |
| Control / Document Editor (Toolstrip) | 2026-10-14T14:43:45.634Z | Editing/clipboard/undo, four styles, RGB foreground/background and cancellation, four alignments, three modes, hyperlink create/edit/cancel/remove, textual image insert/delete/undo, all four explanatory file callbacks and retained TextBox continuity passed. File callbacks do not persist documents. |
| Control / TextBox / TextBox | 2026-10-14T14:45:17.787Z | Independent single/multiline edits, CJK and supplementary text, clipboard/undo, literal TAB at interval four, font request round trip and tab retention passed. |
| Control / TextBox / TextBox (No Tab) | 2026-10-14T14:45:21.814Z | TAB/Shift+TAB focus traversal, independent text replacement and Home insertion passed without TAB text insertion. |
| Control / TextBox / Document | 2026-10-14T14:47:07.397Z | Three independent document editors, selection/cut/undo and literal TAB insertion passed. |
| Control / TextBox / Document (No Tab) | 2026-10-14T14:47:09.736Z | TAB traversed all three editors and distinct content remained independent. |
| Control / Embedded Controls | 2026-10-14T16:05:48.641Z | Heading/table and inline combo checked; RemoteProtocolTest selection survived scrolling, 80/120-column resizing and page return. Exact RIGHT NOW modal opened and closed with retained combo state. |
| Misc / Localization | 2026-10-14T14:49:33.377Z | en-US/zh-CN resources and all three dollar-delimited sentences switched; POSIX number/date formatting limitation remains explicit. |
| Compact dialog and form layout checks | 2026-10-14T16:04:06.036Z | Message, RGB, simple/full Font and Open/Save forms/dialogs checked in English and Chinese at 120x40, 80x25 and restored size. Actions/blank rows, centered message buttons, aligned fields and one-row RGB trackers passed. Clipped retained dialogs were moved into the smaller viewport; Chinese multiline missing-file validation and nested color prompts remained usable. |
| Misc / Dialogs / MessageDialog | 2026-10-14T14:51:01.746Z | Seven button sets, all nine outputs, five icons, three default positions and edited CJK/supplementary message passed. |
| Misc / Dialogs / ColorDialog | 2026-10-14T14:55:47.466Z | RGB 12/34/56 accept/reopen and changed-value cancellation passed; preview and numeric readout agreed. Drag, keyboard steps and 0/255 bounds passed; RGB trackers measured 38 cells by one row. Both locales and viewport sizes passed. |
| Misc / Dialogs / FontDialog | 2026-10-14T14:58:06.299Z | Simple/full controls, four retained effects, nested RGB color acceptance, sample bold/underline/color and cancellation passed. Terminal font metrics remain unchanged. |
| Misc / Dialogs / Open/Save FileDialog | 2026-10-14T15:04:20.409Z | Six fields, async enumeration, folder/filter navigation, exact single/multiple paths, empty/missing-file/missing-directory validation, create/overwrite cancel/accept and default-extension selection passed. Pickers select paths only; inert options remain documented. |
| Misc / DatePicker | 2026-10-14T15:08:54.616Z | Four independent values, leap-day February 2024, year boundary, month/year keyboard navigation, popup selection/dismissal, resize and page retention passed. |
| Window Manager | 2026-10-14T15:31:02.723Z | Child/grandchild ownership, move/resize/overlap/clipping and normal close passed. Enabled frame options and disabled main options were checked; caption variants also passed provider regressions. Local decoder shortcuts and actual portal F8 passed, including concurrent native CppTest global Q. SGR Left/Middle/Right, motion, double-click, Alt/modifiers and both wheel axes passed; Super and Mouse4/5 are unavailable in SGR. |
| Live palette and clipping regression | 2026-10-14T15:34:41.667Z | All six palettes and SkyBlue return passed with exactly one selected radio and emitted RGB, using mouse and keyboard at 120/80/restored widths. Existing children retained positions; hidden grid/tree/editor state, column resize, nonzero document viewport, selection and undo/redo survived. Provider regressions cover queued refresh with modal/date popups, clipping and wide-glyph repair. |
| Exit | 2026-10-14T15:14:26.631Z | All four direct/queued Hide/Close paths passed canceled-then-accepted fresh runs; both Stop paths passed separate fresh runs. All six exited 0, restored full termios and accepted shell input after dialogs. |

## Terminal comparison

GNOME Terminal 3.58.0 / VTE 0.84.0, default profile: compositor-generated Ctrl+Q produced terminal byte `11` and the exact local shortcut dialog. Ctrl+Alt+Super+Q produced `1b 11` and no Super shortcut dialog. This is an observed legacy host-delivery failure; the successful Kitty retry is recorded below. Compositor-generated input is recorded separately from physical keyboard input and PTY byte replay.

The actual GlobalShortcuts portal path delivered Ctrl+Shift+Alt+Super+F8 while the separate task-owned focus-probe terminal had focus. The TUI displayed exact `You pressed Ctrl+Shift+Alt+Win+F8!`; its normal dialog close restored input. Desktop consent was already retained for this application identity.

Kitty 0.45.0 with `--config NONE` passed the same compositor-generated local Ctrl+Alt+Super+Q check: it emitted `1b 5b 31 31 33 3b 31 35 75` (CSI 113;15u) and displayed exact `You pressed Ctrl+Alt+Win+Q!`. Both runs used the same binary and 120-by-40 viewport.

Kitty stopped through the visible Stop TUI action with exit code 0. The complete child PTY termios state matched its pre-launch state before the outer-terminal wrapper restored anything.

Fresh GNOME-terminal runs at 100×30 and 80×25 matched their initial PTY bounds. Six independent normal exits restored the full termios state; a subsequent shell in each restored PTY accepted a line and returned SHELL_INPUT_OK.

All six fresh-run raw output traces ended with keyboard-mode pop, SGR/motion-mouse disable, attribute reset, visible cursor and alternate-screen restoration before the shell probe.

With native CppTest and the TUI running concurrently, their distinct real portal global Q and F8 chords both delivered the exact messages while an owned terminal probe had desktop focus.

The long-running legacy TUI also exited through queued Stop with code 0 and full child-PTY termios restored after all page/dialog/palette tests.

## Platform and showcase limits

Physical keyboard/mouse input and displayed font/cursor/color fidelity remain unverified. GNOME Terminal loses local Super; Kitty delivered the tested chord. SGR has no Super bit or Mouse4/5 delivery here, and the requested POSIX keyboard mode cannot show standalone Alt overlays. Interval-eight geometry, short/empty/CJK caption variants and queued refresh with inaccessible modal/popup controls were checked by provider tests rather than invented showcase controls.

POSIX date/number formatting remains en-US while resources translate. The fake color service has no custom-color palette. ReadOnlyCheckBox, DereferenceLinks, NetworkButton and AddToRecent remain inert file-picker options. The four document file callbacks display explanatory messages and do not persist files; file pickers only select paths.

## Issues Found and Fix

### Host development dependencies

The host `build.sh` failed because its cached Pango include directory no longer exists. The documented prerequisite script already includes `libpango1.0-dev`. A disposable Ubuntu 26.04.1 container was prepared with that unchanged script, and the official build passed there. No dependency or build-script source fix is indicated.

### FullControlTest missing generated DarkSkin include

The container build failed at seven `darkskin` references in generated FullControlTest C++. The new vertical tab rail uses DarkSkin templates and palette values. Upstream generated headers include their DarkSkin dependency, but standalone resource generation had only `GacUI.h`. Updated `<wGac repo>/syncProj.sh` to add `Skins/DarkSkin/DarkSkin.h` through `NormalInclude`, alongside the existing TuiSkin handling. Regeneration and the official build passed.

Post-fix import, synchronization and official build succeeded. The same `syncProj.sh` include adjustment and documentation were applied to iGac and pushed as `dc1cd4d`; its macOS scripts were syntax-checked but not run on Linux, as requested.

### Portal denial corrupted the active terminal

Actual GNOME consent denial for a temporary desktop identity wrote a diagnostic to stderr, scrolling the TUI frame and removing its top tabs. The TUI now disables portal console diagnostics while native apps retain them. The added mock-portal regression passed. On the final binary, actual denial left every captured frame line unchanged and emitted no diagnostic; the global F8 chord did not activate, while local Ctrl+Q remained usable.

Three final runs covered denial, stopping while approval was pending, and an approved real portal F8 activation. All exited 0, restored full termios and accepted shell input. GNOME 50 retained its already documented stale consent window after the pending application exited; that window was dismissed with Cancel.

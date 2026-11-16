# Test Matrix Card 2026-11-16T16:19:01.369Z

## Environment

| Field | Value |
| --- | --- |
| Platform / application / commit / build configuration | Ubuntu 26.04.1; TuiControlTest; input revisions below; native Debug x64 |
| Terminal name / version / profile / viewport sizes | GNOME Terminal 3.58.0 / VTE 0.84.0, xterm-256color; Kitty 0.45.0; 80×25, 100×30, 120×40 |
| Input methods / desktop availability | Raw PTY replay; Kitty-generated CSI-u via its owned remote-control socket; AT-SPI/XTest input did not deliver keys |

Input revisions: GacUI `78b86996`, wGac `485a57a4`, GacJS `3f89bfc4`. Evidence directory: <wGac repo>/build/rpxplat-20261116.

## Test Matrix

| Test Item | 1st |
| --- | --- |
| Build / resource generation / import / sync | 2026-11-16 (passed initially and final import/sync/full build) |
| Unit tests | 2026-11-16T16:38:51.440Z |
| Platform launch and terminal/input capabilities | 2026-11-16 (completed; capability limits recorded) |
| Startup, platform services and teardown | 2026-11-16 (passed; replay/provider/clipboard/restoration, native global unavailable) |
| Follow-up regression checks | 2026-11-16 (provider suite and replay; physical input limits recorded) |
| List / TextList | 2026-11-16T17:03:41.362Z |
| List / ListView | 2026-11-16T16:45:13.899Z |
| List / TreeView | 2026-11-16T16:46:32.041Z |
| List / BindableDataGrid | 2026-11-16 (passed; replay and provider separator regression) |
| Refresh List / TextList | 2026-11-16T16:49:24.454Z |
| Refresh List / BindableTextList | 2026-11-16T16:49:36.155Z |
| Refresh List / ListView | 2026-11-16T16:49:52.048Z |
| Refresh List / BindableListView | 2026-11-16T16:50:07.952Z |
| Refresh List / TreeView | 2026-11-16T16:50:32.326Z |
| Refresh List / BindableTreeView | 2026-11-16T16:50:43.496Z |
| Refresh List / BindableDataGrid | 2026-11-16T16:50:26.493Z |
| Layout / Repeat / RepeatStack | 2026-11-16T16:51:59.009Z |
| Layout / Repeat / RepeatFlow | 2026-11-16T16:52:13.953Z |
| Layout / Repeat / SharedSize (RepeatFlow) | 2026-11-16T16:52:28.916Z |
| Layout / Repeat / SharedSize (TextList) | 2026-11-16T16:52:43.895Z |
| Layout / Responsive | 2026-11-16T16:56:12.877Z |
| Control / Document Editor (Toolstrip) | 2026-11-16 (passed; replay, file bytes and cross-process/cross-editor checks) |
| Control / TextBox / TextBox | 2026-11-16T17:05:49.455Z |
| Control / TextBox / TextBox (No Tab) | 2026-11-16T17:05:56.237Z |
| Control / TextBox / Document | 2026-11-16T17:07:07.506Z |
| Control / TextBox / Document (No Tab) | 2026-11-16T17:07:16.541Z |
| Control / Embedded Controls | 2026-11-16T17:28:19.549Z |
| Misc / Localization | 2026-11-16 (translated UI passed; existing en-US formatting limitation) |
| Compact dialog and form layout checks | 2026-11-16T18:05:52.526Z |
| Misc / Dialogs / MessageDialog | 2026-11-16T17:28:15.332Z |
| Misc / Dialogs / ColorDialog | 2026-11-16T17:30:38.866Z |
| Misc / Dialogs / FontDialog | 2026-11-16T17:31:42.276Z |
| Misc / Dialogs / Open/Save FileDialog | 2026-11-16T17:42:33.651Z |
| Misc / DatePicker | 2026-11-16 (passed; independent dates, leap February, month/year boundaries and resize) |
| Window Manager | 2026-11-16 (replay/provider checks passed; native global and extended mouse limitations below) |
| Live palette and clipping regression | 2026-11-16 (replay/cell attributes passed; collection mutation and modal refresh not exposed by showcase) |
| Exit | 2026-11-16T17:00:40.430Z |
| Packaged tutorial checks (when requested) |  |
| Linux/macOS legacy-terminal and Kitty comparison (when applicable) | 2026-11-16 (completed; native desktop input unavailable in both) |

## Observations and Limitations

GacUI native Debug x64: 97/97 test files and 2008/2008 cases passed. GNOME Terminal 3.58.0 / VTE 0.84.0 with xterm-256color, owned foreground PTY, at 120×40 initially. AT-SPI text inspection/focus works; generated AT-SPI q and Ctrl+Q returned success but delivered no bytes. Physical keyboard/mouse and native global activation remain unavailable. GNOME Terminal displayed colors remain unverified; the later Kitty capture is recorded separately. Byte replay and decoded cell evidence continue independently. The default terminal title is set through OSC, while its cells begin with the List tab rail.

Linux only. Packaged Windows tutorial verification was not requested and remains blank. Native terminal delivery, raw byte replay and displayed appearance are recorded separately. No earlier results are carried forward.

## Issues Found and Fix

## Terminal Comparisons

The Linux locale backend deliberately uses `EnUsLocaleImpl` (documented in the README): translated labels, three dollar-escaped sentences and Chinese dialog strings update, while date/time/number/currency formatting retains en-US. The stored date/time and numeric values remain unchanged. This is an existing provider restriction, not credited as full locale formatting support.

Additional byte-replay observations: the grid Birthday editor retained an 8/7/1988 change after leaving the cell; changing its From date to 12/31/1989 left only the 2000 record, and disabling the bound restored the data. Embedded Controls retained RemoteProtocolTest, displayed the exact `Pretend to be starting!` dialog, and remained usable after scrolling at 80×25 and returning to 120×40. Native terminal delivery remains separate.

MessageDialog passed all seven button sets, all nine outputs, every applicable default-button position and all five icons, with representative Chinese buttons and 80×25/120×40 layouts. ColorDialog passed RGB 12/34/56 acceptance/reopen, changed-color cancellation, numeric endpoints and tracker Home/End at 0/255. Simple/full FontDialog passed all four effects, nested color, acceptance/reopen/cancel and both locales/sizes; terminal metrics remain TuiFont size 1. FileDialog passed known-directory single/multiple results, text/XML/all filters, subfolder navigation, empty/missing-path validation, create/overwrite accept/cancel, default extension, Chinese validation, and Open/Save at both viewport sizes. Logs and cell captures use the `tui-{message,color,font,file}-*` prefixes.

The 21 palette checkpoints (all six choices and SkyBlue restoration at 120×40, 80×25 and 120×40) retain exactly one selected radio. VT attributes show the six intended accent backgrounds and unchanged neutral black/gray roles. Retained document/TextBox/grid state and a separately seeded Responsive marker survive palette changes. Child/grandchild movement, overlap, enabled frame toggles, clipping and normal child closes were captured. Tests that match caption Close must distinguish `[x]` checkboxes from title buttons.

The terminal editor passed exact edit/clipboard operations, all four style toggles, formatting cancellation, four alignments, hyperlink cancel/create/activation/removal, three modes and independent retained document/TextBox markers. GUI→TUI→both GUI private roundtrips preserve identical full XML/style catalog and three image payloads after original paths are removed. RTF/HTML exports contain the expected Unicode, format and images; canceled pickers preserve the original file. A fresh Kitty process reopened the TUI private file. HTML text assertions decode `&nbsp;` and ignore formatting tags.

TUI document foreground/background changes were verified in exported HTML (`#0C2238` and `#C8B4A0`). Its own image picker passed cancel, insert, select/delete and undo. After deleting the original BMP, private reload displayed exactly `[Image, 123 x 456]`, including after palette refresh. The full provider suite additionally covers configured tab geometry, clipping/wide-cell repair, grid separator backgrounds, live retained menu/date/tree themes, queued work and canceled/reentrant shutdown; these are unit checks, separate from live terminal input.

### Terminal comparison: GNOME Terminal and Kitty

| Check | GNOME Terminal 3.58.0 / VTE 0.84.0 | Kitty 0.45.0 |
| --- | --- | --- |
| Owned foreground PTY and tested sizes | 80×25, 100×30, 120×40 | 80×25, 120×40 |
| Desktop-generated input attempt | AT-SPI reported success but delivered no bytes | XTest focus succeeded, but test keys delivered no bytes |
| Extended shortcut encoding | Explicit raw CSI-u replay only | Kitty `send-key` generated `ESC[113;5u` and `ESC[113;15u`; exact local dialogs passed |
| Physical keyboard/global shortcut | Unavailable | Unavailable; Kitty remote control is not physical input |
| Displayed color observation | Desktop screenshot denied | Owned X11 window capture shows Pink accent, neutral backgrounds and focused underline/bold |
| Rich private file in fresh process | Original detailed run | Reopened terminal-saved document, three images and Unicode retained |
| Normal teardown and usable shell | All six normal paths passed | Queued Close returned 0; full termios restored and same-PTY shell command passed |

Kitty used a private extracted Ubuntu package/runtime; no host package installation or terminal settings were changed. Evidence: `kitty-*.json`, `kitty-*.log` and `kitty-pink.png` under the evidence directory. The owned capture confirms displayed appearance; it does not establish physical input.

Legacy SGR replay delivered Left/Middle/Right down/up with Shift/Alt/Ctrl combinations, movement and both wheel axes. Mouse4/Mouse5 payloads are rejected by the existing VlppOS decoder, and SGR has no Super mouse modifier; these inputs remain unavailable and are not credited as five-button/Super delivery. The separate Kitty CSI-u key test covers Super keyboard encoding. Repeated button tests must exceed the 500 ms double-click interval when asserting a Down event.

Final live follow-ups verified exact tab stops at columns 4/4/4/8, exactly one U+0009 through clipboard roundtrip, an 80-line Unicode editor scroll to both ends, and a 45-line wide external paste with the final line immediately visible. Full copied text matched exactly. VT cell attributes alternate foreground/background at both an interior caret and the final text position. Selected grid cells across every column and multiple rows retain black backgrounds for every bottom separator, including intersections and the area beyond the final column. The actual expanded Gender popup handled Home/End/arrow/Page keys and Enter. DatePicker crossed December 31 into January 1 while retaining the other three independent dates.

External GTK non-text ownership did not insert text; switching to a UTF-8 owner enabled exact Unicode paste, and GTK read back the full TUI copy. The disabled Paste menu did not handle synthetic Escape, so the harness dismissed it at a confirmed outside location before targeting the editor. The final queued Close after all dialogs and palette changes returned normally, restored the entire termios state and allowed a same-PTY shell command. Both owned terminal bridges were then cleaned up.

Collection mutation of the palette source and explicitly queued refresh while an open modal blocks the radio controls are not exposed by showcase controls. The provider unit suite verifies retained menus/date popups, clipping and theme refresh separately. Caption variants (short, long, empty, CJK, SizeBox toggles) are covered by that suite; live child windows retain the showcase-generated captions. No Windows/macOS or packaged tutorial coverage is claimed.

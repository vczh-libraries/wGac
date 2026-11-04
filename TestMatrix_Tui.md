# Test Matrix Card 2026-11-04T06:38:46.156Z

## Environment

| Field | Value |
| --- | --- |
| Platform / application / commit / build configuration | Linux GNOME Wayland; Test_TuiControlTest; GacUI upstream fc3385c2d plus task fixes; wGac official build |
| Terminal name / version / profile / viewport sizes | GNOME Terminal 3.58 / VTE 0.84, default profile; Kitty 0.45.0, default config; 120×40, 100×30, 80×25 |
| Input methods / desktop availability | Native Mutter keyboard/pointer, IBus Unicode composition, external GTK clipboard; unlocked desktop screenshots, AT-SPI and read-only Kitty captures; no terminal-byte replay |

## Test Matrix

| Test Item | 1st |
| --- | --- |
| Build / resource generation / import / sync | 2026-11-04T06:58:13.551Z (passed) |
| Unit tests | 2026-11-04T07:40:33.645Z (passed) |
| Platform launch and terminal/input capabilities | 2026-11-04T10:38:07.000Z passed with terminal limits |
| Startup, platform services and teardown | 2026-11-04T10:51:28.226Z passed with recorded terminal limits |
| Follow-up regression checks | 2026-11-04T10:38:07.001Z passed with UI limits |
| List / TextList | 2026-11-04T10:28:48.410Z passed |
| List / ListView | 2026-11-04T09:04:11.513Z passed |
| List / TreeView | 2026-11-04T09:11:38.675Z passed |
| List / BindableDataGrid | 2026-11-04T09:29:23.197Z passed |
| Refresh List / TextList | 2026-11-04T08:51:25.432Z passed |
| Refresh List / BindableTextList | 2026-11-04T08:51:31.649Z passed |
| Refresh List / ListView | 2026-11-04T08:51:53.227Z passed |
| Refresh List / BindableListView | 2026-11-04T08:51:58.035Z passed |
| Refresh List / TreeView | 2026-11-04T08:52:02.896Z passed |
| Refresh List / BindableTreeView | 2026-11-04T08:52:07.658Z passed |
| Refresh List / BindableDataGrid | 2026-11-04T08:53:04.588Z passed |
| Layout / Repeat / RepeatStack | 2026-11-04T08:57:00.225Z passed |
| Layout / Repeat / RepeatFlow | 2026-11-04T08:57:03.455Z passed |
| Layout / Repeat / SharedSize (RepeatFlow) | 2026-11-04T08:57:06.663Z passed |
| Layout / Repeat / SharedSize (TextList) | 2026-11-04T08:57:09.918Z passed |
| Layout / Responsive | 2026-11-04T08:04:26.647Z passed |
| Control / Document Editor (Toolstrip) | 2026-11-04T09:50:22.789Z passed |
| Control / TextBox / TextBox | 2026-11-04T08:58:11.184Z passed |
| Control / TextBox / TextBox (No Tab) | 2026-11-04T08:58:13.631Z passed |
| Control / TextBox / Document | 2026-11-04T08:18:55.249Z passed |
| Control / TextBox / Document (No Tab) | 2026-11-04T08:20:00.567Z passed |
| Control / Embedded Controls | 2026-11-04T08:23:58.273Z passed |
| Misc / Localization | 2026-11-04T08:24:35.062Z passed |
| Compact dialog and form layout checks | 2026-11-04T10:34:22.089Z passed with UI limits |
| Misc / Dialogs / MessageDialog | 2026-11-04T08:28:03.416Z passed |
| Misc / Dialogs / ColorDialog | 2026-11-04T08:32:23.662Z passed |
| Misc / Dialogs / FontDialog | 2026-11-04T08:32:09.996Z passed |
| Misc / Dialogs / Open/Save FileDialog | 2026-11-04T10:26:56.851Z (fixed; passed) |
| Misc / DatePicker | 2026-11-04T09:02:24.893Z passed |
| Window Manager | 2026-11-04T10:06:48.100Z (passed with terminal limitations) |
| Live palette and clipping regression | 2026-11-04T09:59:18.312Z passed with UI limitations |
| Exit | 2026-11-04T10:51:28.225Z passed; 6/6 fresh shutdown paths |
| Packaged tutorial checks (when requested) |  |
| Linux/macOS legacy-terminal and Kitty comparison (when applicable) | 2026-11-04T10:38:07.001Z completed; VTE Super limitation preserved |

## Observations and Limitations

- Full GacUI suite passed 95/95 files and 1965/1965 cases. After incorporating upstream direct-message-label commits 062ed2a6c/fc3385c2d, the focused provider suite passed 21/21 and metadata generation/validation exited 0. Final resource generation, import/sync and official native build passed. Evidence is under <GacUI repo>/.github/TaskLogs/rp-20261103/.
- Fresh startup at 100×30 and 80×25 matched the terminal before resizing. Native VTE and Kitty resize checks covered 120×40 → 100×30 → 80×25 → 120×40. Hosted children/grandchildren moved, resized, overlapped and closed while the main app remained responsive; exposed cells repainted. Modal caption close was verified separately from application shutdown.
- All six fresh final-source shutdown runs passed: direct/queued Hide, direct/queued Close and direct/queued Stop. Each Hide/Close first canceled exactly one invocation/query with ready count zero, then accepted the retry. Every run returned exit 0, matched complete pre/post stty state, restored the baseline shell display and accepted native ShellReady input. Provider regressions separately check ready notifications, reentrant close and listener removal. After testing another native application, the terminal was focused and its Exit page confirmed before targeting the queued Stop action.
- TextList covered exact fresh 0–9 datasets, two additions through 19, continuing counters after Clear, both positional-removal results, independent checks/radios, selection and right-only source rotation. ListView covered both 29-record datasets and all six effective-Detail requests. TreeView covered all four roots and 6/4/4/4 children in both panes. Grid checks covered all five editors, sorting/ties/reset, default and changed date filters, source rotations, column resize/scroll and Unicode cell alignment.
- All seven Refresh List pages passed their complete mutation/view/controller cycles, retained selection, scrolling/recycling and narrow repaint. The four independent Repeat models passed controller sequences, 40-item scrolling, tracker extremes, independent scroll controls, variable widths and shared widths. Responsive passed all 4/7/4 explicit levels, boundaries, retained marker and resizing.
- All four TextBox/Document pages preserved independent CJK/supplementary markers, clipboard, selection, undo/redo, font size 1→6→1 and resize/tab state. External GTK clipboard round-trips preserved literal TAB; tab-X positions were 4,4,4,8. No-Tab pages traversed focus. Interval eight is covered by provider tests because the showcase exposes no interval control.
- Toolstrip document checks covered editing/command availability, all four styles, foreground/background accept/cancel/reset, four alignments, Editable/Selectable/Preview, hyperlink create/edit/cancel/activate/remove, image insertion/delete/undo/redo, and two-editor retention. The four file callbacks show the expected format/path and explicitly remain unimplemented; no persistence is claimed. Embedded Controls covered the combo, RIGHT NOW modal, table, keyboard navigation and retained scroll state.
- Localization covered twelve values, three literal-dollar forms and shared locale selection with retained date/time. POSIX formatting remains English. DatePicker retained four independent values, February 29, 2024 and year boundaries through palettes, resizing and repeated popups. Popup dismissal was confirmed by clicking outside; Escape did not dismiss the month popup.
- MessageDialog covered all seven button sets, nine outputs, five icons and valid default positions. The final direct-label template also passed fresh English/Chinese wrapped Unicode messages and centered three-button actions at 80×25 and restored 120×40. The launch form strips literal newlines; provider tests cover explicit blank lines, and file-validation prompts supplied multiline UI coverage.
- Color and simple/full Font dialogs passed accept/reopen/cancel, RGB edits and tracker endpoints/steps, all style flags, nested color dialogs and compact Chinese layout. Color/font/file actions aligned right with one explicit blank row above; textbox/group/list borders and aligned labels were retained. TuiFont and size 1 are intentionally read-only; custom color palette persistence is unsupported.
- Open/Save dialogs covered all fields, filters/index/default extension, async directory enumeration, parent/subfolder navigation, single/multiple paths, empty/missing file/directory validation, disabled multi-selection, create/overwrite prompts and en-US/zh-CN compact layouts. ReadOnlyCheckBox, DereferenceLinks, NetworkButton and AddToRecent flags are inert in this provider.
- Every palette passed mouse and Tab/Enter selection at 120×40, 80×25 and restored size, with exactly one selected radio and SkyBlue restored. Child/grandchild state, two independent document markers and undo/redo, grid edits, expanded trees and date values survived. Border/SizeBox/IconVisible/TitleBar toggles were exercised; disabled CustomizedFrame/MaximizedBox/MinimizedBox stayed unchanged. Child caption editing and queued main-radio refresh behind a modal are not exposed by the showcase; provider tests supplement those inaccessible paths. The hosted caption supports Close only.
- Native Kitty input delivered exact Left/Middle/Right down/up and Alt readouts, movement, double-click and both wheel axes. Mouse4/Mouse5 were not delivered with or without Alt; SGR mouse has no Super field. Standalone ALT-label access and unsupported terminal key-release behavior are not claimed. Native-renderer five-button/Super coverage is separate.
- Fresh TUI global Ctrl+Shift+Alt+Super+F8 and concurrent native FCT Ctrl+Shift+Alt+Super+Q produced their exact messages while another terminal held focus. The earlier overlapping TUI instances shared a desktop identity; repeating with one fresh TUI instance passed. Returning from document pages did not leave hidden shortcuts intercepting later dialogs. Packaged tutorials were not requested, so that row stays blank.

## Issues Found and Fix

- Accepted filenames disappeared after reopening/canceling a picker and repainting. Both TUI and FCT pages had bound their result list directly to the dialog output collection, which is cleared on the next ShowDialog. They now copy accepted paths. Fresh final-source TUI verification retained alpha.txt after two canceled pickers, 80×25/120×40 resize and Pink/SkyBlue template rebuilds; the fresh native FCT regression also passed.
- Earlier upstream message-label fixes were incorporated by rebase. Focused provider tests and fresh compact dialogs verified the final direct SolidLabel implementation. Test-harness corrections—waiting for repaint, targeting the innermost action, refocusing an editor and confirming popup dismissal—are not reported as product fixes.

## Terminal Comparisons

| Check | GNOME Terminal 3.58 / VTE 0.84 | Kitty 0.45.0 |
| --- | --- | --- |
| Native Ctrl+Q | Expected modal | Expected modal |
| Native Ctrl+Alt+Super+Q | Not delivered; ordinary input remained live | Exact You pressed Ctrl+Alt+Win+Q! modal, both Super sides |
| Input mechanism | Native Mutter events, no byte replay | Native Mutter events, no byte replay |
| Resize/palettes | 120×40, 100×30, 80×25 and restored | Same sizes; retained state and fresh compact startup |
| Normal exit | Four canceled Hide/Close requests, then accepted queued Hide; exit 0 | Separate fresh shutdown records with stty equality and native shell input |

The VTE Super failure remains a terminal limitation; the Kitty pass does not replace it. Evidence includes kitty-super.*, tui-native-mouse.json, tui-startup-*.png, tui-fresh-odd-100.png, tui-fresh-even-80.png, tui-file-copy-final-*.png, tui-direct-message-*.png and tui-*-shutdown.txt under <GacUI repo>/.github/TaskLogs/rp-20261103/.

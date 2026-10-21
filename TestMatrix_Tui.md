# Test Matrix Card 2026-10-21T11:19:05.560Z

## Environment

| Field | Value |
| --- | --- |
| Platform / application / commit / build configuration | Ubuntu 26.04, final wGac import/sync build, Clang C++23, no-reflection |
| Terminal name / version / profile / viewport sizes | Owned xterm-256color UTF-8 PTY: 100x30, 80x25 and 120x40; 240x40 for parallel filename comparison; GNOME Terminal 3.58.0/VTE 0.84.0 and Kitty 0.45 |
| Input methods / desktop availability | UTF-8/CSI/SGR replay and live cell capture; Mutter compositor-generated global/local keys; external GTK X11 clipboard |

## Test Matrix

| Test Item | 1st |
| --- | --- |
| Build / resource generation / import / sync | PASS |
| Unit tests | PASS: 28/28 provider/easy-layout cases; 13/13 native-service cases |
| Platform launch and terminal/input capabilities | PASS with terminal limitations below |
| Startup, platform services and teardown | PASS: fresh 100x30/80x25/120x40, modal interception, external clipboard and normal shell restoration |
| Follow-up regression checks | PASS: runtime cell checks plus provider regressions; physical-input limits below |
| List / TextList | PASS: retained datasets, styles and source rotation |
| List / ListView | PASS: six views, both complete scrolled datasets, column resizing exposes full filenames |
| List / TreeView | PASS: roots/children, source rotation and retained expansion |
| List / BindableDataGrid | PASS: five editors, sort/filter states, source rotation and seven view requests |
| Refresh List / TextList | PASS |
| Refresh List / BindableTextList | PASS |
| Refresh List / ListView | PASS |
| Refresh List / BindableListView | PASS |
| Refresh List / TreeView | PASS |
| Refresh List / BindableTreeView | PASS |
| Refresh List / BindableDataGrid | PASS: projection/header cycles, File edit and all seven views retain values |
| Layout / Repeat / RepeatStack | PASS |
| Layout / Repeat / RepeatFlow | PASS |
| Layout / Repeat / SharedSize (RepeatFlow) | PASS |
| Layout / Repeat / SharedSize (TextList) | PASS |
| Layout / Responsive | PASS |
| Control / Document Editor (Toolstrip) | PASS: editing/styles/alignment/modes, hyperlink/image, four explanatory file callbacks |
| Control / TextBox / TextBox | PASS: editing, tabs, Unicode and resize |
| Control / TextBox / TextBox (No Tab) | PASS: traversal, Unicode and retained text |
| Control / TextBox / Document | PASS: all three editors, Unicode, selection/undo/redo, tab and resize retention |
| Control / TextBox / Document (No Tab) | PASS: all three independent editors and Tab traversal |
| Control / Embedded Controls | PASS |
| Misc / Localization | PASS with POSIX formatting limitation below |
| Compact dialog and form layout checks | PASS for captured cells at 120x40/80x25; full Font moved into the smaller viewport |
| Misc / Dialogs / MessageDialog | PASS: seven sets, nine outputs, five icons and defaults |
| Misc / Dialogs / ColorDialog | PASS: RGB 12/34/56 acceptance/reopen/cancel |
| Misc / Dialogs / FontDialog | PASS: simple/full forms, effects/color retention and fixed cells; displayed appearance unverified |
| Misc / Dialogs / Open/Save FileDialog | PASS: fixture selection, filters, multiple paths, validation and nested confirmation |
| Misc / DatePicker | PASS: four independent dates, leap February, popup/resize retention |
| Window Manager | PASS for observable input/frame/child operations; limitations below |
| Live palette and clipping regression | PASS: live palette/state/child checks plus retained-popup/clipping provider regressions |
| Exit | PASS: six fresh normal exits, canceled-then-accepted Hide/Close, restored termios and usable shell input |
| Packaged tutorial checks (when requested) |  |
| Linux/macOS legacy-terminal and Kitty comparison (when applicable) | GNOME Super unavailable; same-build Kitty PASS |

## Observations and Limitations

- Application behavior was driven through an owned interactive PTY using the official `./test.sh --app:tui`. Evidence is live terminal-cell output and UTF-8/CSI/SGR input, supplemented by compositor-generated keys and an independent GTK X11 clipboard client. This does not establish physical keyboard/mouse/touch hardware, displayed font/color/profile appearance, or macOS support. Packaged tutorial verification was not requested and its result remains blank.
- Startup matched fresh 100x30, 80x25 and 120x40 viewports. A hosted shortcut modal intercepted owner interaction and dismissed normally. All four original Hide/Close buttons were canceled once (one invocation/query, zero ready notifications), then accepted on separate fresh runs. Both Stop buttons also returned normally. All six runs exited zero, restored the original terminal settings and accepted a shell input marker. The long showcase run also canceled all four buttons in sequence (4/4/0 readouts) before queued Hide exited normally. A real Kitty run closed normally too.
- Native portal activation while another GNOME Terminal had focus produced exactly `You pressed Ctrl+Shift+Alt+Win+F8!`. Portal rejection/cancellation and registration cleanup are covered by the four GlobalShortcuts service tests; a separate desktop denial interaction is not claimed. Local Ctrl+Q and three delivered SGR mouse buttons, Alt/Ctrl/Shift modifiers, movement and both wheel axes worked. SGR has no Super bit; standalone Alt overlays and extra native terminal mouse buttons remain unavailable/unverified.
- The provider/easy-layout selection passed 28/28 cases. It covers viewport fitting before application work, Hide/Close cancellation/notification/reentrancy, modal interception, retained live themes, tab intervals and scalar geometry, grid separators, frame/border composition, clipped wide glyph repair and timer/owner-thread behavior. These deterministic checks supplement runtime operations; they are not terminal-generated input or desktop appearance evidence. All five native service binaries passed again after the final official build (13/13 cases).
- TextList counters/removals/styles, source rotation, tree expansions and all Refresh List model/projection/header cycles retained state. Main ListView requested all six views and compared both datasets through scrolling; widening File columns exposed matching complete filenames. Grid default From-only filtering yielded three records, To-only four, both exactly the two specified records, and disabling filters restored five. All five cell types retained edits after source rotation; the edited birthday was 8/9/1988. Refresh DataGrid's File editor retained `RefreshFile` across text mappings and all seven view requests.
- All four Repeat pages completed the independent clear/reset/add/remove/counter sequences. Responsive View/Stack/Group reached both bounds with 4/7/4 levels and retained the editor through resize. Explicit level buttons were used; resizing was not described as automatic responsive-level switching.
- Toolstrip checks covered selection/replacement, clipboard/cut/undo/redo, styles, alignment, preview/selectable/editable behavior, hyperlink creation/activation/canceled edit/removal, image cancellation/insertion/deletion/undo/redo and all four file callbacks. The callbacks displayed the requested format/path and their not-implemented explanation; no document persistence is claimed. An independent GTK X11 reader received the exact two document markers, and its `External你好😀` text pasted into the TUI. Both three-pane document pages retained distinct third-editor markers through tab changes and resize.
- Message dialogs exercised seven button sets, nine outputs, five icons and default positions. RGB 12/34/56 acceptance/reopen/cancel agreed with the preview; tracker Home/End/arrow operations reached 0/255. Simple/full Font preserved effects and nested color while font metrics remained fixed. English/Chinese dialog controls, compact forms and nested validation were observed; the full Font window was moved into the smaller viewport after resizing. Custom-color persistence is unsupported. File fixtures covered asynchronous enumeration, filters, multiple exact paths, empty/missing selections, missing folders, create/overwrite cancellation and acceptance, default extensions and preserved results after cancellation. These pickers select paths, not file contents.
- DatePicker retained four independent values, including 2024-02-29, through popups, tab changes and resize. Localization changed translated strings and preserved escaped dollars; POSIX date/number formatting remains en-US as documented.
- All seven palette selections returned to SkyBlue, including the smaller viewport. Hidden editor content and undo/redo survived refresh. Child/grandchild movement, overlap, clipping, frame toggles and closing exposed the underlying cells correctly. Live-popup theme replacement and caption/clipping corner cases are covered by the provider tests; arbitrary caption editing and a precisely queued mid-modal refresh are not exposed by this application's terminal UI.

## Issues Found and Fix

- No new TUI implementation defect was reproduced. Test-driver corrections included incremental UTF-8 decoding, waiting for theme repaint, using display-cell widths for CJK, selecting exact popup items instead of same-named tabs, focusing the document after modal dismissal and clicking inside the shorter direct Hide/Close buttons. These were harness corrections, not application fixes.
- Preserve the GNOME local-Super limitation below. The Kitty pass does not turn it into a GNOME pass.

## Terminal Comparisons

### Local Ctrl+Alt+Super+Q

| Field | Required evidence |
| --- | --- |
| Host / build / configuration | Ubuntu 26.04.1, the same synchronized wGac TUI executable; UTF-8, Kitty disambiguated-key mode requested |
| Legacy terminal attempted first | GNOME Terminal 3.58.0, VTE 0.84.0; ordinary desktop profile |
| Operation and expected result | Mutter-generated Ctrl+Alt+Super+Q versus Ctrl+Alt+Q; distinct terminal bytes and exact local-Super dialog |
| Legacy result | Both chords delivered hex `1b11`; Super was lost |
| Kitty retry after failure | Kitty 0.45 delivered `1b5b3131333b313575` and `1b5b3131333b3775`; the app displayed `You pressed Ctrl+Alt+Win+Q!` |
| Conclusion | Only Kitty distinguished these local chords in the tested setup; portal global delivery is a separate pass |
| Evidence / cleanup | Raw hex and exact dialog above; the Kitty app returned normally with exit zero and restored modes. Physical-key and displayed-profile observations remain unverified |

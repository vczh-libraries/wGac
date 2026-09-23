# Test Matrix Card 2026-09-22T22:42:46-07:00

Scope: Linux FullControlTest with the native Wayland renderer over MiniHTTP only. Other verification targets were excluded by request; their result cells remain blank. Browser, RPT, RVM, TUI, standalone FCT, unit tests and other platforms were not verified.

## Test Matrix

| Test Item | 1st |
| --- | --- |
| [Linux][Test_CppTest_Rvm][`/MiniHttp`] |  |
| [Linux][`/RPT`][`/MiniHttp`] |  |
| [Linux][`/FCT`][`/MiniHttp`] | 2026-09-22T22:42:46-07:00 |
| [Linux][`/RVMT`][`/MiniHttp`] |  |
| [Linux][`/RVMT`][`/MiniHttp /Cli:<path>`] |  |

## Build and Synchronization

- CodePack, `import.sh`, `syncProj.sh`, `build.sh`, and the Core full build passed before verification.
- Synchronization refreshed upstream FullControlTest Easy Layout resources and their generated C++ snapshot. No framework or platform source fix was needed.
- Final `import.sh` and `syncProj.sh` passed. All 294 imported/generated files exactly match the snapshot used for runtime verification. Final `build.sh` passed (exit 0).

Completed: 2026-09-22T23:04:13-07:00; upstream GacUI `e8be96806`.

## FCT Verification

All UI input used the active native renderer automation endpoint. Each result was checked in Core Controls and the active renderer DOM, resolving its element references rather than relying on the historical Elements catalog.

| Check | Result |
| --- | --- |
| Initial UI | Passed: exact `Complete Control Showcase` title and List, Control, Misc, Window Manager tabs; no fatal state. |
| TextList add/clear | Passed: initially empty; one Add produced 0–9 in each list; Clear removed all twenty items while both lists and action buttons remained usable. |
| Editor typing | Passed: Search contained exactly `Wayland-MiniHTTP-22`; rich editor contained exactly `Rich22-Hello[Ab]{Cd}`. Renderer keyboard input preserved brackets and braces. |
| Tab retention | Passed: both exact markers survived List → Control without retyping. |
| Shortcut labels | Passed before and after replacement and takeover: `Ctrl+Q`, `Ctrl+Alt+Super+Q`, `{Ctrl+Shift+Alt+Super+Q}`; no `osSuper` placeholder. |
| Local shortcuts | Passed on all three renderers: Ctrl+Q opened exactly `You pressed Ctrl+Q!`; Ctrl+Alt+Super+Q opened exactly `You pressed Ctrl+Alt+Win+Q!`. Each distinct dialog was dismissed and disappeared. |
| Mouse payloads | Passed on renderer1 and replacement renderer2: Left/Middle/Right/Mouse4/Mouse5 exact down/up results and double clicks; movement; both directions of vertical/horizontal wheels. Checked no modifiers, Alt, Super, Alt+Super, and Ctrl+Shift+Alt+Super against the exact Alt/Super readout. |
| Mouse label styling | Passed before and after mouse checks: same font and text color as shortcut labels (`Noto Sans`, 12, `#F1F1F1FF`). |
| Renderer replacement | Passed: stopped only renderer1, kept Core alive, connected renderer2 on 8889, repeated all local shortcut and mouse checks, and confirmed both exact editor markers without retyping. |
| Concurrent takeover | Passed: renderer3 on 8890 took over from live renderer2, retained both exact editor markers, and passed local shortcut checks. Renderer2 exited 0 and closed its 8889 listener, leaving it unable to submit stale input. |
| Normal shutdown | Passed: renderer3 activated Exit → `self.Close() (InvokeInMainThread)`. Core and renderer3 exited 0 without fatal state or retry. No owned process or listener remained on 8888/8889/8890. |

## Platform Coverage Limits

- The global Ctrl+Shift+Alt+Super+Q activation is unavailable in the current wGac provider, as documented in README.md; its registration is a stub. Its displayed label was verified. This run does not claim global shortcut activation.
- Input evidence comes from the renderer automation service, including its keyboard and mouse command paths. Physical desktop input and the browser-only bracket KeyDown values were not verified.

## Issues Found and Fix

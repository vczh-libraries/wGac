# Test Matrix Card 2026-10-14T13:54:35.543Z

Scope: complete Linux verification from the current remote-protocol job and SOP. Times use UTC. Each row is updated when it starts or finishes.

## Test Matrix

| Test Item | 1st |
| --- | --- |
| [Linux][Test_CppTest_Rvm][`/MiniHttp`] | 2026-10-14T15:04:56.947Z |
| [Linux][`/RPT`][`/MiniHttp`] | 2026-10-14T14:45:33.109Z |
| [Linux][`/FCT`][`/MiniHttp`] | 2026-10-14T14:50:21.985Z |
| [Linux][`/RVMT`][`/MiniHttp`] | 2026-10-14T15:05:05.862Z |
| [Linux][`/RVMT`][`/MiniHttp /Cli:<path>`] | 2026-10-14T15:05:16.597Z |
| [Linux][CodePack / import / sync / build] | 2026-10-14T16:11:15.530Z (fixed) |
| [Linux][Native service unit tests] | 2026-10-14T16:11:15.530Z |
| [Linux][Local simple smoke] | 2026-10-14T14:29:34.495Z |
| [Linux][Local FullControlTest standard] | 2026-10-14T14:30:08.080Z |
| [Linux][Local FullControlTest hosted] | 2026-10-14T16:11:15.530Z (fixed) |

## Verification details

GacUI official Core full build and RVM host build passed. Focused GacUI unit tests passed 89/89 cases across five files covering TUI, EasyLayout, hosted windows and Alt navigation.

Final native regressions passed 9/9 cases: AsyncService 2, ImageService 2, GlobalShortcuts 4, RenderingLifecycle 1. The added cases cover drawing through repeated buffer resizing and suppressing portal diagnostics for TUI while retaining native diagnostics.

Local standard FCT passed lists, retained editors, local and real portal global shortcuts, five-button mouse/modifier checks, palette state, all four responsive layouts, all 24 films in each appearance, account scrolling and all modal close paths. Standard modal resizing used GNOME compositor input and settled at 240/350/440/680 by 320 plus 680 by 220/520.

Local hosted FCT passed the same controls, shortcuts (including the real portal chord), mouse matrix, palettes and complete responsive-modal sweep. Maximizing the native host before widening its virtual modal supplied enough client space.

Native RPT passed controls, local/global shortcuts, five-button modifier input, document modal, replacement/takeover with retained state, File-menu shutdown and the exact retained fatalError. Fatal IO rejected ordinary input and accepted exact !Exit. DOM checks resolve active Dom element references.

Remote FCT passed all controls, retained editors, palettes, responsive/modal sweep, renderer replacement, repeated local/global shortcuts and 25 mouse combinations, then normal queued Close. The global-dialog abort occurred once in five attempts; four consecutive debugger retries passed without an exception, so no speculative fix was made.

standalone RVM passed normal Translate, second-host rejection, local UI, normal shutdown and host-loss variants (1128/6118 ms). Requester terminated from RpcInjectedException.

network RVM passed normal Translate, second-host rejection, renderer replacement, normal shutdown and host-loss variants (284/5201 ms). Renderer retained exact RemotingTest_RvmHost disconnected. and exited through !Exit.

stdio RVM passed normal Translate, owned stdio child cleanup, renderer replacement, normal shutdown and host-loss variants (281/280 ms). Renderer retained exact RemotingTest_RvmHost disconnected. and exited through !Exit.

Final official import.sh, syncProj.sh and build.sh succeeded after both runtime fixes.

Post-fix hosted startup and vertical-tab Up/Down/Alt navigation passed five consecutive fresh runs, each exiting normally with code 0.

Post-fix hosted FCT repeated the full control/input/palette and responsive-modal sweep, including all 24 films at all four appearances and every close path, then exited 0.

Post-fix remote FCT passed vertical-tab Up/Down/Alt selection and the full responsive-modal sweep with clean Core/renderer exits.

## Platform coverage limits

The post-fix standard native run passed controls, palettes, opening the responsive modal and normal close. Its supplementary compositor resize retry did not establish focus or resizing on the intended modal, so that retry is unverified; the original standard sweep passed, and post-fix hosted and remote sweeps passed every size. Physical keyboard/mouse input and display fidelity were not inferred from automation.

## Issues Found and Fix

### Host development dependencies

The host `build.sh` failed because its cached Pango include directory no longer exists. The documented prerequisite script already includes `libpango1.0-dev`. A disposable Ubuntu 26.04.1 container was prepared with that unchanged script, and the official build passed there. No dependency or build-script source fix is indicated.

### FullControlTest missing generated DarkSkin include

The container build failed at seven `darkskin` references in generated FullControlTest C++. The new vertical tab rail uses DarkSkin templates and palette values. Upstream generated headers include their DarkSkin dependency, but standalone resource generation had only `GacUI.h`. Updated `<wGac repo>/syncProj.sh` to add `Skins/DarkSkin/DarkSkin.h` through `NormalInclude`, alongside the existing TuiSkin handling. Regeneration and the official build passed.

Post-fix import, synchronization and official build succeeded. The same `syncProj.sh` include adjustment and documentation were applied to iGac and pushed as `dc1cd4d`; its macOS scripts were syntax-checked but not run on Linux, as requested.

### Intermittent native FCT renderer abort

After a complete responsive-layout sweep, the first native FCT renderer aborted with an unhandled `vl::Error` while dismissing a real portal global-shortcut dialog. Core remained alive and a replacement renderer recovered its state. Four further attempts under a debugger all passed without an exception; the replacement also passed the mouse matrix and normal shutdown. This is one failure in five attempts, below the job's threshold for a reproducible issue. No speculative fix was made.

### Hosted layout resized an active Wayland draw buffer

Hosted startup crashed twice in three attempts. GDB captured cairo_surface_flush from WaylandBuffer::EndDraw through hosted GlobalTimer rendering. The pool had deleted the active draw buffer when layout resized its native window. WGacView now retains shared ownership through the draw; the native render target skips an obsolete-sized commit and returns ResizeWhileRendering. Windows Direct2D already defers canvas resizing and reports that result.

The new RenderingLifecycle regression paints through repeated pool resizing and checks the next buffer. It passed, as did five consecutive fresh hosted startups with normal exits and complete post-fix hosted/remote responsive sweeps.

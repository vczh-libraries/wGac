# Test Matrix Card 2026-11-25T10:34:24.109Z

Input revisions: GacUI `6f192cacc`, wGac `ea438a1`, GacJS `db7e1bdd`. Evidence: `<wGac repo>/build/rpxplat-20261125`.

## Test Matrix

| Test Item | 1st |
| --- | --- |
| [Linux][Test_CppTest_Rvm][`/MiniHttp`] | 2026-11-25T11:08:40.033Z |
| [Linux][`/RPT`][`/MiniHttp`] | 2026-11-25T11:12:47.573Z |
| [Linux][`/FCT`][`/MiniHttp`] | 2026-11-25T11:47:53.851Z |
| [Linux][`/RVMT`][`/MiniHttp`] | 2026-11-25T11:13:13.684Z |
| [Linux][`/RVMT`][`/MiniHttp /Cli:<command>`] | 2026-11-25T11:13:30.775Z |

## Local applications and build

| Test Item | 1st |
| --- | --- |
| CodePack / import / sync / build | 2026-11-25T11:54:55.354Z |
| Native service unit tests | 2026-11-25T10:52:34.523Z |
| Local Hello World | 2026-11-25T11:14:33.240Z |
| Local FullControlTest standard | 2026-11-25T11:37:09.509Z |
| Local FullControlTest hosted | 2026-11-25T11:44:51.573Z |
| Live Raw Clock pixels, palettes and renderer lifecycle | 2026-11-25T11:20:36.639Z |
| Native portal registration | 2026-11-25T12:00:27.237Z |
| Native desktop input and global shortcuts |  |
| Native touch acquisition |  |
| Native Touching |  |
| Native TouchingList |  |
| Native Toolstrip point-input scenario |  |

## Observations and Limitations

Linux only. Native touch acquisition, Touching and TouchingList are excluded because wGac has no native touch provider. The Toolstrip point-input scenario is also excluded by the SOP for wGac. Raw Clock and mouse/keyboard checks remain in scope. No earlier results are carried forward.

## Completed observations

Initial CodePack, import, dual-architecture resource generation and full `<wGac repo>/build.sh` succeeded. All seven native service invocations passed (29 cases): Async 2, Image 2, Clipboard 3, GlobalShortcuts 4, RenderingLifecycle 2, and RawElement 8 each in standard and hosted modes. These cover real Wayland buffers/cleanup and an isolated mock shortcut portal; desktop global activation remains separate. Evidence: `<wGac repo>/build/rpxplat-20261125/build.log`, `<wGac repo>/build/rpxplat-20261125/native-services-results.json` and `<wGac repo>/build/rpxplat-20261125/service-*.log`.

The full GacUI Linux unit suite passed 2,019/2,019 cases across 97/97 files (`<wGac repo>/build/rpxplat-20261125/unit-test.log`). The generated Linux unit inventory includes the upstream Windows-theme source, whose platform guards remain in effect.

Snapshot audit resolved cumulative element IDs in all 96 changed frames: 91 matched the committed rendered state exactly; five supplementary-character cases had only the expected Linux UTF-32 caret offsets (4 versus Windows UTF-16 5) and corresponding mock text widths. Image metadata and frame counts were unchanged. The complete local patch and semantic/metadata audits are retained in the evidence directory; verified platform/allocation churn was restored rather than replacing the shared baselines.

Standalone native RVM passed exact RPC and duplicate-host rejection, normal close, and idle-next-call/blocked-call host loss; both failure variants terminated through an unhandled `rpc_controller::RpcInjectedException`. Native RPT passed its normal workflow, five buttons/modifiers and both wheel axes, replacement/takeover, canceled/accepted close, and a fresh fatal case. A Core debugger observation recorded exactly one serialized `!Error` with `This is a fatel error!`; renderer `Dom.fatalError` matched, ordinary input did not resume it, and exact `!Exit` closed it. Linux io_uring sends were invisible to the initial syscall-payload probe; the package-serialization observation and renderer state provide the fatal evidence. No product fix was needed. Evidence: `rvm-local-network*`, `<wGac repo>/build/rpxplat-20261125/rpt-native-normal-and-strace-probe.log`, `<wGac repo>/build/rpxplat-20261125/native-rpt-fatal-core-fatal-packages.jsonl`, `<wGac repo>/build/rpxplat-20261125/native-rpt-fatal.json`.

Both native remote RVM host modes passed exact RPC, renderer replacement/takeover, normal shutdown and the owned stdio-child reap. Network mode rejected a second host. Fresh idle and blocked host-loss cases each produced one Core-authored serialized `!Error` carrying `RemotingTest_RvmHost disconnected.`, a matching retained renderer DOM, and nonzero Core termination; exact `!Exit` then closed each renderer. GDB observed Core package serialization without changing source, and reports SIGABRT through its child-result status. Evidence: `<wGac repo>/build/rpxplat-20261125/rvm-native-*-results.json`, `<wGac repo>/build/rpxplat-20261125/*-fatal-packages.jsonl`, fatal DOM captures and logs.

Live Raw Clock pixel checks passed in standard, hosted and remote modes, after palette/template changes, and after renderer replacement/takeover. Stable owned Wayland buffer captures show the complete white rim/hour marks, cyan/green/yellow hands and antialiased dark outlines; each later Core time produced changed clock pixels. Layout changes resized the standard/hosted backing surface from 642×517 to 642×885 and remote from 642×517 to 642×527, with all four rim extrema visible. The standard PNG was visually inspected. This validates application buffers, separately from desktop screen capture. Evidence: `<wGac repo>/build/rpxplat-20261125/clock-pixels-*.json`, `<wGac repo>/build/rpxplat-20261125/clock-pixels-*.png` and `<wGac repo>/build/rpxplat-20261125/native-clock-pixels.log`.

A live AT-SPI attempt found the owned native FCT process, but its application exposed zero accessible frames or controls, so document operations could not be repeated through UIA. GNOME ScreenSaver reported `false`; current input/capture limitations are not attributed to a confirmed lock. Portal applications also exposed no consent frames during this probe. Evidence: `<wGac repo>/build/rpxplat-20261125/native-fct-standard-atspi.json`.

Standard local FCT completed both document editors, context/toolbar edit operations, selection and clipboard, fonts/colors/effects/alignment, hyperlinks/read-only modes, images, private save/reload, RTF/HTML export, all eight styles, layout/palette retention and normal close. The final Toolstrip command group passed in an isolated fresh application after correcting the probe to move the pointer before focusing or opening its context menu. Gallery probes now explicitly open the dropdown before scrolling an offscreen style. These are probe corrections, not product fixes. Evidence: `<wGac repo>/build/rpxplat-20261125/fct-standard-*.json`, document XML/export files, retained split-run logs and `<wGac repo>/build/rpxplat-20261125/fct-one-fct-extra-Toolstrip-standard.log`.

Hosted FCT completed the same editor/clipboard/formatting/image/export suite, style/read-only checks, palette retention, responsive resize/scroll cycles and normal shutdown. Its four appearances were observed at actual client widths 240, 350, 440 and 613 (height 320); tall/short cases reached 428/220 pixels. The host constrained the Windows reference dimensions, while every appearance and the equal-width reachable footer passed. Native service IO did not execute compositor maximize actions; a Wayland trace contained no maximize request. Standard-mode OS window resize and wider native gallery geometry therefore remain unverified. Evidence: `<wGac repo>/build/rpxplat-20261125/fct-hosted-*.json`, `<wGac repo>/build/rpxplat-20261125/fct-hosted-initial-gallery-probe.log`, `<wGac repo>/build/rpxplat-20261125/fct-hosted-layout-and-maximize-probe.log`, `<wGac repo>/build/rpxplat-20261125/fct-local-matrix-hosted.log` and `host-maximize-probe.*`.

Captured standard/hosted/browser layout states retained exactly five pixels between the final vertical header and the responsive button under Default, Ember and Aurora. The selected header changed to the corresponding palette color before hover; contiguous header positions remained stable. Evidence: `<wGac repo>/build/rpxplat-20261125/layout-palette-geometry-audit.json`.

Remote FCT completed both full document suites, style/read-only checks, responsive layouts, exact retained document XML across replacement/takeover, fresh lists/shortcuts/buttons on the new renderer and normal queued close. Hosted and remote scrolling exposed all 24 films and all eight publisher names. Their four appearances followed 0→1→2→3 and back; remote widths were 240/350/440/613 and wide heights 220/438. Geometry audits confirmed equal-width footer buttons and consistent compact/detailed rows. Supplemental standard, hosted, remote and Firefox runs passed vertical-tab Up/Down and retained checkbox, choice and editor values across palettes. Evidence: `<wGac repo>/build/rpxplat-20261125/native-fct-matrix-result.json`, `<wGac repo>/build/rpxplat-20261125/fct-native-matrix.log`, `<wGac repo>/build/rpxplat-20261125/responsive-appearance-audit.json`, and `<wGac repo>/build/rpxplat-20261125/fct-*-layout-inputs.json`.

The live native GlobalShortcuts portal accepted CreateSession and BindShortcuts (both response 0), returned the requested Ctrl+Shift+Alt+Super+Q binding, and the session closed with the owned app. No consent window was exposed. Outside-focus activation remains unavailable: AT-SPI reported the owned GNOME terminal focused/active and accepted both string and keycode generation, but its raw capture received zero bytes; XTest likewise delivered no bytes to the verified Kitty window. Native FCT again exposed no accessible frames. These observations establish registration, not global activation or physical input. Evidence: `<wGac repo>/build/rpxplat-20261125/native-portal-result.json`, `<wGac repo>/build/rpxplat-20261125/native-desktop-atspi.json`, `<wGac repo>/build/rpxplat-20261125/native-atspi-input.json`, `<wGac repo>/build/rpxplat-20261125/native-key-capture.json` and `<wGac repo>/build/rpxplat-20261125/kitty-xtest-ctrl-q.json`.

Final `<wGac repo>/import.sh` → `<wGac repo>/syncProj.sh` → `<wGac repo>/build.sh` completed successfully in order. The upstream ownership audit matched all 23 changed imported/resource paths (allowing documented line endings and resource-generator settings) and all 124 changed GacJS snapshot JSON files. Evidence: `<wGac repo>/build/rpxplat-20261125/final-import.log`, `<wGac repo>/build/rpxplat-20261125/final-sync.log`, `<wGac repo>/build/rpxplat-20261125/final-build.log`, and `<wGac repo>/build/rpxplat-20261125/final-generated-ownership-audit.json`.

Additional selection checks retained Vertigo through every reachable responsive width and synchronized account choices between the compact menu and wide list. Native remote DOM showed exactly one account-selection image, beside Simon & Schuster; the matching browser screenshot showed one green arrow there. Standard mode passed compact movie/account selection. Evidence: `<wGac repo>/build/rpxplat-20261125/fct-*-responsive-selection.json`, `<wGac repo>/build/rpxplat-20261125/native-responsive-account-arrow-audit.json` and `<wGac repo>/build/rpxplat-20261125/browser-responsive-selected-account.png`.

## Issues Found and Fix

No reproducible product defect was found. Probe corrections and unavailable input paths are documented above.

## Raw Clock and Touching port verification, 2026-11-25

Compared GacUI `33d8ab2f4` with wGac `9d99584`. The rendering changes from GacUI `63585c923` were already ported in wGac `8cd2ab6`: the Clock draws black outlines beneath its white face and colored hands, while Touching draws a SkyBlue rectangle with a black border and orange contact rings over the skin background. No additional painter change was needed.

The shared Clock implementation matches upstream byte for byte; its header differs only by the expected framework include. Both raw-page XML resources match upstream. The local showcase, native renderer and raw-element test each directly compile the portable Clock base, Wayland plugin and Touching painter exactly once. Evidence: `<wGac repo>/build/raw-rendering-port-20261125/source-audit.json` and `<wGac repo>/build/raw-rendering-port-20261125/port-history-audit.json`.

Fresh owned-buffer captures verified the advancing Clock in standard, hosted and remote modes, palette changes, page switching, and renderer replacement/takeover. The complete rim, hour marks, colored hands and dark outlines remained visible. Standard and hosted Touching captures each contained 15,484 SkyBlue pixels and 1,040 black border pixels. A synthetic Core contact produced 76 exact orange pixels; releasing it removed all orange pixels. Touching descriptors remained present after renderer replacement and takeover. The Clock and Touching PNG captures were visually inspected. Evidence: `<wGac repo>/build/raw-rendering-port-20261125/clock-pixels-*`, `<wGac repo>/build/raw-rendering-port-20261125/touching-*-pixels.json`, `<wGac repo>/build/raw-rendering-port-20261125/touching-remote-lifecycle.json` and matching PNGs. These checks inspect application drawing buffers; native Wayland touch acquisition remains unsupported.

Native RPT and remote RVMT regressions passed replacement, takeover and shutdown checks. Standalone `<wGac repo>/test.sh --app:rvmt` with the MiniHTTP host returned the exact greeting `Hello, StandaloneRawPort!`; normal close ended the requester with status 0 and the host with status 1 during channel teardown. All owned processes stopped. Evidence: `<wGac repo>/build/raw-rendering-port-20261125/rpt-native-progress.json`, `<wGac repo>/build/raw-rendering-port-20261125/rvm-native-network-results.json` and `<wGac repo>/build/raw-rendering-port-20261125/standalone-rvmt-result.json`.

The required `<wGac repo>/import.sh` → `<wGac repo>/syncProj.sh` → `<wGac repo>/build.sh` sequence succeeded without generated-source changes. After the build, `Test_RawElement /C` and `Test_RawElement /C --hosted` each reported 8/8 passing cases, including fixed-time outlines/transparency, Touching border/contact pixels, clipping and attachment lifetime. The rebuilt showcase, native renderer and raw-element test binaries were byte-identical to the binaries used for the live checks. Evidence: `<wGac repo>/build/raw-rendering-port-20261125/{import,syncProj,build}.log`, `<wGac repo>/build/raw-rendering-port-20261125/raw-element-{standard,hosted}.log` and `<wGac repo>/build/raw-rendering-port-20261125/binaries-after-build-check.log`.

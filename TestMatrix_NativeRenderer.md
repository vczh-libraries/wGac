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

## Element shape rendering verification, 2026-11-26

WinXPSkin's radio background and selection bullet both request elliptical gradients. `GuiGradientBackgroundElementRenderer` ignored `GetShape()` and always filled a rectangle. The original renderer failed the new ellipse-corner pixel assertion, and xgSkins' hosted application buffers showed the same square fill and green square bullet. Windows and macOS renderer source both honor the requested shape; neither platform was run during this Linux verification.

The shared Cairo shape path now handles gradient backgrounds, solid backgrounds and solid borders. Rounded corners honor both radii and clamp them to the available size. Empty bounds cannot introduce a singular Cairo transform. Rectangle border alignment and the existing full-bounds ellipse border convention are preserved.

| Regression executable | Passing cases |
| --- | --- |
| `Test_ElementShapes /C` | 6/6 |
| `Test_ImageService /C` | 2/2 |
| `Test_RenderingLifecycle /C` | 3/3 |
| `Test_RawElement /C` | 8/8 |
| `Test_RawElement /C --hosted` | 8/8 |

The shape regressions cover all three shapes and four gradient directions, unequal corner radii, oversized and zero radii, identical solid/constant-gradient silhouettes, alpha, clipping, retained transforms and empty bounds followed by another element. Logs are `<wGac repo>/build/element-shapes-*.log`.

xgSkins imported the platform change without editing any skin source. Fresh hosted and multi-window runs passed exclusive radio selection, hover/press, radio-style lists, and preservation of selection and list data across Win7Skin, Win8Skin, DarkSkin and WinXPSkin. Application-buffer captures were inspected for radio circles, the Elements gallery, buttons, tabs, scrollbars, full progress, ribbon, toolstrip and calendar. Both final runs closed through the Exit page with status 0. Evidence is under `<xgSkins repo>/scripts/Linux/.build/shape-verification`, including `before-hosted-*`, `after-hosted-*`, `xg-results.json`, `xg-suite.log` and `xg-{hosted,standard}-*.png`.

wGac's native standard and hosted Elements pages also displayed the correct silhouettes. Remote RPT passed button activation, grid population/clear, retained state after replacement/takeover, and canceled/accepted normal close. Remote FCT retained the Elements page and changed palette after replacement/takeover; application-buffer captures showed correctly shaped gradients. The standalone RVM requester with the MiniHTTP host returned exactly `Hello, Shapes[Ab]{Cd}!` and closed normally. All owned test processes were stopped. Evidence: `integration-results.json`, `integration.log`, `remote-elements-*.png` and `rvm-local-*` in the same xgSkins evidence directory.

The additional element audit found that `GuiInnerShadowElementRenderer::Render` is still an unimplemented stub: its three gallery samples are blank. This existing limitation is documented in both READMEs and is not used by WinXPSkin. Verification used application automation and owned drawing buffers; it does not establish physical desktop input or compositor screenshot behavior.

The required wGac `import.sh` → `syncProj.sh` → `build.sh` sequence completed successfully with no imported or generated-source changes. The rebuilt native showcase, renderer, RVM requester and all four regression executables matched the tested binaries byte for byte (`build/element-shapes-binary-check.log`). xgSkins' Linux import, synchronization and build also succeeded, and its `Import/WGac` and `Import/WGacShared` snapshots match the final upstream directories.

## Remote clipboard verification, 2026-11-28

Scope: `<GacUI repo>/TODO_TASK_wGac.md`. Input revisions were GacUI `ec765a5d1` and wGac `0d2377d`. Evidence is under `<wGac repo>/build/clipboard-20261128`. This run uses Ubuntu 26.04.1, GNOME Wayland and XWayland. A separate Xvfb display provides an independent Core X11 selection without clipboard redirection; renderer-side clipboard clients use the desktop display. GNOME reported the desktop unlocked.

The required `<wGac repo>/import.sh` → `<wGac repo>/syncProj.sh` → `<wGac repo>/build.sh` sequence passed. Sixty source/import comparisons matched upstream after the documented line-ending normalization. The linked platform Core has exactly one `main`, `GuiMain`, public `SetupRemoteNativeController`, packed `SetupRemoteNativeControllerInternal`, and remote-view-model RPC initializer. Its object list includes the FCT/RPT/RVMT resources without duplicate objects. Evidence: `import.log`, `syncProj.log`, `build.log` and `source-audit.json` in the evidence directory.

Sixteen isolated launcher checks passed in foreground and `--unblock` modes. They verified Core `/MiniHttp /FCT /fakeClipboard /denyClipboard`, renderer `/MiniHttp /corehost:localhost /port:8889 /denyClipboard`, and rejection of renderer `--fakeClipboard` before executing an application. The `test_core.sh` checks used executable argument spies: FCT/RPT/RVMT select the platform Core, standalone `cpptest_rvm` selects its requester, manual RVMT starts the network host, and `--cli` builds the host and forwards one quoted command without launching a second host. These are launcher checks, not RVMT regression-suite results. The real renderer also rejected `/MiniHttp /fakeClipboard` with exit status 1; its syscall trace contains no network or io_uring initialization. Evidence: `launcher-audit.json`, `native-argument-rejection.json` and `renderer-fake-argument.strace`.

The rebuilt focused GacUI `TestRemote_Startup.cpp` suite passed 32/32 cases. Its clipboard cases exercise all eight routing combinations, acknowledged/rejected writes, delayed first snapshots, fallback notifications and writes during the gap, empty snapshots, present empty text, superseded writers, late updates after disconnect, and standalone-image input to both fake-clipboard editors. These are controlled protocol regressions, separate from native desktop evidence. The native clipboard service suite passed 3/3 cases for private readback, rich/image retention and selection-clear retirement. Evidence: `unit-remote-startup.log` and `native-clipboard-service.log`.

The native image comparison exposed a Wayland image-provider defect. Both alpha conversions truncated channels, so repeated PNG serialization darkened translucent pixels. Before the fix, each editor's restored fixture had 596 pixels in the first image and 71 in the second with a two-level premultiplied-channel error; alpha remained unchanged. Both conversions now round to the nearest channel value. The new native regression checks five PNG roundtrips across all 256 alpha values and every valid premultiplied channel value, preserving every Cairo pixel exactly. `Test_ImageService /C` passed 3/3 cases, including this regression. Evidence: `before-fix-pixel-audit.json` and `native-image-service.log`.

The final required import, synchronization and build sequence passed after the fix (`final-import.log`, `final-syncProj.log`, `final-build.log`). The source/import and linked-symbol audit passed again. Synchronization also copied the existing upstream TUI global-shortcut fallback into its resource and generated C++; no generated files were edited manually. `final-tested-binaries.sha256` identifies the Core, renderer and service-test binaries used for the final run.

Every matrix row starts a fresh `/MiniHttp /FCT` Core. Both Ribbon and Toolstrip were explicitly put into Editable mode through their visible menus; Toolstrip selections use visible Select All. The renderer connected without a GacUI sharing prompt. Evidence names use `final-<fake>-<core-deny>-<renderer-deny>` followed by `-verification.json`, `-trace.jsonl`, editor Controls/Dom dumps, and saved private documents.

| Core fake | Core deny | Renderer deny | Active clipboard | Ribbon | Toolstrip |
| --- | --- | --- | --- | --- | --- |
| Off | Off | Off | Renderer Wayland | Incoming formats and notifications pass; successful writes pending input serial | Same |
| Off | Off | On | Core X11 | Pass | Pass |
| Off | On | Off | Core X11 | Pass | Pass |
| Off | On | On | Core X11 | Pass | Pass |
| On | Off | Off | Renderer Wayland | Incoming formats and notifications pass; successful writes pending input serial | Same |
| On | Off | On | Core fake | Pass | Pass |
| On | On | Off | Core fake | Pass | Pass |
| On | On | On | Core fake | Pass | Pass |

In every OS-backed row, text was seeded before connection. An independent `xclip` or `wl-copy` owner then replaced text with an unsupported-only format and with `after-Ribbon 中文 Ω 🌍 é` or `after-Toolstrip 中文 Ω 🌍 é`. While the context menu remained open, Paste changed enabled → disabled → enabled before invocation. Its label changed between `#F1F1F1FF` and `#6D6D6DFF`; Controls/Dom dumps preserve those states. Paste inserted the replacement Unicode exactly, and a second Paste inserted the same value again. External clients read back the exact Unicode. Wayland changes reached Core during the notification callback without an offer-lock deadlock. X11 ownership changes and external reads worked with the Core on its invisible X11 window and no focused Wayland window.

In all three fake rows, both editors initially had Paste disabled. After a Cut, a separate desktop X11 client changed its own clipboard; the remote editor retained the private value and the external client retained its new value. In all six fallback rows, Ribbon Cut of `AB` from `ABCDE` produced `CDE`, then Paste at the end produced `CDEAB`. Toolstrip Select All/Cut produced an empty document, then typing `PREFIX` and pasting produced `PREFIXABCDE`. Rich Cut emptied each editor and Paste restored its styled paragraphs and two images. Content comparisons preserve left/center/right alignment, fonts, sizes, colors, bold/italic, image frame indices and display dimensions; redundant false-style wrappers and regenerated resource names are normalized.

Both editors imported the rich fixture and a standalone patterned 5×3 RGBA PNG from independent clipboard owners on both OS routes. Every X11 row exported rich-document bytes to an independent client and exported the single-image selection with `UTF8_STRING`, `application/x-gacui-document` and `image/png` together. Fake rows inserted the same image through the visible image picker, then Cut/Paste restored it. Private-document decoding and external PNG reads verify the original 1250×500 and 256×256 fixture images and the 5×3 pattern, including transparent and translucent pixels. Evidence: `document-binary-audit.json` and `pixel-audit.json`.

The two shared matrix rows each recorded one subscription, 14 renderer snapshots and eight Paste calls, with zero Core clipboard writes and zero Core fallback reads/writes after the first snapshot. The initial snapshot was empty before the compositor delivered the seat's selection; the preseeded text followed before external replacement checks. Subsequent text → unsupported-only → Unicode snapshots arrived in order. Paste checks and repeated Paste introduced no new subscription or read request. Each of the six denied rows recorded zero clipboard protocol messages and six local Cut calls. Evidence: `protocol-audit.json`; debugger field inspection reported no errors. Successful shared Cut acknowledgement and write-echo behavior remain pending native input; the focused protocol tests cover those paths independently.

The live fake-fallback handoff kept one Core alive across sharing renderer 8889 → denying takeover 8890 → sharing renderer 8891 → denying renderer 8892. The first renderer pasted `r1-seed`. The first denial disabled Paste, then a local Cut/Paste stored and restored `gap-value`. A new sharing renderer's unsupported-only snapshot disabled Paste instead of exposing that fallback; a later external offer pasted `r3-seed`. Denying again restored `gap-value`. The trace has four connection configurations and exactly two subscriptions. These native takeovers delivered disconnect notifications; takeover without a preceding disconnect, deliberately delayed snapshots, stale writers/updates, and present-empty-text/image-only cases are covered by the 32 focused tests rather than claimed as desktop timing tests. Evidence: `final-handoff-result.json` and its trace and phase dumps.

The isolation run used Core Xvfb `:2` and the renderer's GNOME Wayland clipboard as separate selection domains on one host, with no clipboard bridge between them. Core retained `core-local` while the editor pasted `renderer-before` and then `renderer-after`. Replacing Core's selection with `core-new` did not change the editor's shared value. A denying takeover then pasted `core-new`. The sharing interval had zero fallback reads or writes; two fallback reads after the disconnect correctly precede the denied connection. This verifies separate clipboard domains, not a connection between two physical computers. The outgoing `remote-copy` leg awaits a renderer input serial. Evidence: `final-isolation-result.json`, its trace and Controls/Dom dumps.

In the separate no-serial session, automation selected `AB` in `ABCDE` and invoked Cut. Exactly one remote write request (ID 92) received a matching `false` reply. Core reported `GuiRemoteClipboardInjection#The renderer could not write the clipboard.` No `SetSelectionText` call followed Cut, so the selected range was not deleted before Core failed. The debugger observed SIGABRT; repeating without the debugger exited with status 134. Evidence: `final-no-serial-cut-result.json`, `final-no-serial-cut-trace.jsonl`, the before-Cut Controls/Dom dumps, and `final-no-serial-direct-result.json`.

A separate X11 fallback probe copied `outgoing 中文 Ω 🌍 é` to an independent text client exactly, then exchanged the rich fixture Ribbon → Toolstrip → Ribbon through visible Copy/Paste commands. Paragraph formatting and both images survived. These are two editors in one FCT process; external rich-document serialization is separately verified by the independent X11 client in the matrix. Evidence: `final-peer-ribbon-toolstrip-ribbon.json` and its saved private documents.

The final artifact audit decoded the original fixture plus 46 captured clipboard/private documents. All rich-document paragraph/style comparisons passed. All 82 image comparisons had exact dimensions, zero alpha mismatches and zero premultiplied-channel error, including the additional cross-editor transfers. Eleven complete native traces passed protocol assertions with zero debugger inspection errors. The final binary checksums still match the built and tested executables.

Pending desktop coverage is limited to successful shared Wayland Copy/Cut publication, matching success replies and subsequent no-echo/cache behavior, outbound Unicode/rich/PNG exchange with renderer-side applications, and simultaneous rich/PNG MIME publication on that route. The automation IO commands do not supply a compositor input serial. The desktop was unlocked. AT-SPI accepted valid Shift keysym and hardware-keycode synthesis calls, but the subsequent Cut still entered the clipboard failure path; those calls did not supply a usable renderer serial. The session also lacks permission to write the kernel input device. Evidence: `input-facility-inspection.json` and `atspi2-input-evidence.json`. The tested rejection path is intentional and is not reported as a successful shared write. Separate physical-machine networking was not exercised. All other timing-only claims above are identified as focused unit coverage. All test applications, clipboard owners, debuggers and the isolated X server were stopped after verification.

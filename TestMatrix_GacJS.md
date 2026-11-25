# Test Matrix Card 2026-11-25T10:34:24.109Z

Input revisions: GacUI `6f192cacc`, wGac `ea438a1`, GacJS `db7e1bdd`. Evidence: `<wGac repo>/build/rpxplat-20261125`.

## Test Matrix

| Test Item | 1st |
| --- | --- |
| [Linux][`/RPT`][`/MiniHttp`] | 2026-11-25T10:40:42.307Z |
| [Linux][`/FCT`][`/MiniHttp`] | 2026-11-25T10:41:31.129Z |
| [Linux][`/RVMT`][`/MiniHttp`][Native `RemotingTest_RvmHost` over network] | 2026-11-25T10:50:44.466Z |
| [Linux][`/RVMT`][`/MiniHttp`][Native `RemotingTest_RvmHost` over stdio `/Cli:<command>`] | 2026-11-25T10:50:58.200Z |
| [Linux][`/RVMT`][`/MiniHttp`][GacJS browser host `?rvmhost`] | 2026-11-25T10:51:05.520Z |
| [Linux][`/RVMT`][`/MiniHttp`][GacJS Node `cli.js` over network] | 2026-11-25T10:51:20.776Z |
| [Linux][`/RVMT`][`/MiniHttp`][GacJS Node `cli.js` over stdio `/Cli:<command>`] | 2026-11-25T10:51:34.854Z |

## Build and generated output

| Test Item | 1st |
| --- | --- |
| Import / codegen / build / tests / repeat codegen | 2026-11-25T10:35:05.542Z |

## Additional browser checks

| Test Item | 1st |
| --- | --- |
| Live Chromium composition and Core selection ownership | 2026-11-25T10:53:16.127Z (passed; auto and contenteditable adapters) |
| Browser touch acquisition, scrolling, cancellation and replacement | 2026-11-25T10:55:48.742Z (passed; Chromium acquisition plus separately identified synthetic ID reuse) |
| Browser point editing and file-picker double taps | 2026-11-25T10:55:38.482Z (passed; Chromium CDP touch) |
| Full Toolstrip commands with automation point selection | 2026-11-25T11:01:10.822Z |
| Firefox synthetic point input | 2026-11-25T11:04:09.163Z |
| Raw Clock DPR 2 and resize | 2026-11-25T11:04:14.421Z |
| Read-only style and hyperlink commands | 2026-11-25T11:04:20.694Z |
| Native OS IME attempt |  |

## Observations and Limitations

Linux Playwright Firefox is the required renderer. No earlier results are carried forward.

## Completed observations

Import, codegen, build, all 279 tests across 11 packages, and repeat-codegen stability passed. Controlled browser tests executed Chromium, WebKit and Firefox. The private WebKit runtime was reused with its required library path; host library-cache preflight was skipped for that runtime, while every actual test ran. The Windows-only live E2E skip is separate from the completed live Linux Firefox matrix. Evidence: `<wGac repo>/build/rpxplat-20261125/gacjs-test-resumed.log` and `<wGac repo>/build/rpxplat-20261125/gacjs-codegen-stability.json`.

Firefox 146.0.1 completed RPT Home, exact local-shortcut dialogs and Super labels, grid Add/Clear, document modal, replacement/takeover, repeated input checks, canceled/accepted File close, and the fresh fatal regression. Fatal evidence contains exactly one Core-authored error, exact `This is a fatel error!` mask and nonzero Core exit. Mouse evidence separates Playwright Left/Middle/Right from DOM-synthetic five-button and modifier checks. Desktop global activation is not established by browser input. Evidence: `<wGac repo>/build/rpxplat-20261125/rpt-browser.log`, `<wGac repo>/build/rpxplat-20261125/rpt-browser-results.json`, `<wGac repo>/build/rpxplat-20261125/rpt-browser-normal-events.json` and `<wGac repo>/build/rpxplat-20261125/rpt-browser-fatal.json` in the evidence directory.

Firefox FCT passed both editors, exact clipboard/edit results, context and toolbar formatting, hyperlinks/read-only modes, image insertion and private/RTF/HTML file output, all eight styles, palette/state retention, responsive resize/scroll and mouse-capture release, replacement/takeover and normal Force Exit. The responsive clients settled at 240, 350, 440 and 680 by 320; compact scrolling exposed 24 film titles and wide navigation eight publishers. Raw Clock data changes produced changed canvas pixels with one painter. Outgoing bracket keys include 219/221 with shifted braces. Evidence: `<wGac repo>/build/rpxplat-20261125/fct-browser-matrix.log`, `<wGac repo>/build/rpxplat-20261125/fct-browser-responsive-sizes.json`, `<wGac repo>/build/rpxplat-20261125/browser-fct-matrix-result.json` and per-operation control/event captures. Native OS IME was attempted separately as recorded below; physical desktop input is not established.

All five Firefox RVM host modes passed initial/continued exact RPC, replacement/takeover and normal shutdown. Manual native, browser and Node network hosts rejected a second host without losing the accepted service. Both stdio modes used exact POST `!Exit`, returned `Queued`, exited Core normally and reaped the actual child; Node was resolved through PATH with a quoted script path containing spaces. Fresh idle-next-RPC and blocked-RPC host termination produced exactly one `RemotingTest_RvmHost disconnected.` fatal message and terminated Core in every mode. Evidence: `<wGac repo>/build/rpxplat-20261125/rvm-browser-*.log`, per-mode results, fatal JSON and host/Core logs. Externally started second-host admission does not apply to the owned stdio channels.

Chromium acquisition/gesture and click-pairing runs passed, including Touching/TouchingList, nested scrolling, selected-range point behavior, image-dialog folder navigation and single image insertion by double tap, cancellation/capture/replacement and no duplicate mouse fallback. The full Toolstrip command pass additionally used Core automation short point presses and 650 ms holds for caret/range setup and selected-range context menus. This is separate from Chromium-acquired touch and physical touch. Probe corrections used an interior editor coordinate and the documented seven-position `[Image]` representation; these were test-step corrections, with no product fix. Evidence: `<wGac repo>/build/rpxplat-20261125/touch-step3-browser*.json`, `browser-touch-extra` captures, `<wGac repo>/build/rpxplat-20261125/browser-toolstrip-point-results.json`, point command logs and retained probe logs.

Firefox DOM-synthetic point checks passed with explicitly stubbed capture; they do not establish Firefox hardware acquisition. DPR 2 and resized backing-canvas checks passed, as did Preview/Selectable rejection of style and hyperlink mutations with Editable restoration. The native IME attempt activated IBus CangJie3 and sent XTest input only to the verified owned headed Firefox window. Search and Document both remained `ABCD`, so native candidate selection/commit/cancellation could not be verified; the original US engine was restored. Evidence: `<wGac repo>/build/rpxplat-20261125/browser-native-ime-fct.json` and screenshot. Browser font dialogs exposed only Segoe UI, so a second installed-family choice was unavailable; font-size and color changes passed.

A supplemental Firefox run verified the collapsed Style popup at an actual 669×870 client, all eight styles/reapplication with range retention, and normal shutdown. This complements the original 1280×960 inline-gallery/dropdown run. Vertical-tab Up/Down and checkbox/choice/editor retention across palettes also passed. Native desktop key attempts through both AT-SPI and XTest produced no delivered bytes, so outside-focus global shortcut activation remains unverified. Evidence: `<wGac repo>/build/rpxplat-20261125/browser-fct-supplement-result.json`, `<wGac repo>/build/rpxplat-20261125/browser-narrow-gallery-size.json`, `<wGac repo>/build/rpxplat-20261125/browser-narrow-gallery.png` and the native card’s desktop-input evidence.

Additional selection checks retained Vertigo through every reachable responsive width and synchronized account choices between the compact menu and wide list. Native remote DOM showed exactly one account-selection image, beside Simon & Schuster; the matching browser screenshot showed one green arrow there. Standard mode passed compact movie/account selection. Evidence: `<wGac repo>/build/rpxplat-20261125/fct-*-responsive-selection.json`, `<wGac repo>/build/rpxplat-20261125/native-responsive-account-arrow-audit.json` and `<wGac repo>/build/rpxplat-20261125/browser-responsive-selected-account.png`.

## Issues Found and Fix

No reproducible product defect was found. Probe corrections and unavailable input paths are documented above.

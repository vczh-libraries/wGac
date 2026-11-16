# Test Matrix Card 2026-11-16T16:19:01.369Z

Input revisions: GacUI `78b86996`, wGac `485a57a4`, GacJS `3f89bfc4`. Evidence directory: <wGac repo>/build/rpxplat-20261116.

## Test Matrix

| Test Item | 1st |
| --- | --- |
| [Linux][`/RPT`][`/MiniHttp`] | 2026-11-16T16:31:14.062Z |
| [Linux][`/FCT`][`/MiniHttp`] | 2026-11-16T18:14:37.570Z |
| [Linux][`/RVMT`][`/MiniHttp`][Native `RemotingTest_RvmHost` over network] | 2026-11-16T16:44:21.167Z |
| [Linux][`/RVMT`][`/MiniHttp`][Native `RemotingTest_RvmHost` over stdio `/Cli:<command>`] | 2026-11-16T16:44:28.195Z |
| [Linux][`/RVMT`][`/MiniHttp`][GacJS browser host `?rvmhost`] | 2026-11-16T16:44:42.473Z |
| [Linux][`/RVMT`][`/MiniHttp`][GacJS Node `cli.js` over network] | 2026-11-16T16:44:56.178Z |
| [Linux][`/RVMT`][`/MiniHttp`][GacJS Node `cli.js` over stdio `/Cli:<command>`] | 2026-11-16T16:45:03.379Z |

## Build and generated output

| Test Item | 1st |
| --- | --- |
| Import / codegen / build / tests / repeat codegen | 2026-11-16 (passed; private WebKit runtime) |

## Observations and Limitations

Linux Playwright Firefox is the primary matrix renderer. Supplemental Chromium CDP and explicitly identified Firefox synthetic checks cover browser touch and composition. Windows, macOS, Wasm and physical touch hardware are outside this run. Native desktop input and IME limitations are recorded below.

## Issues Found and Fix

### Build/tests, attempt 1

The first root `yarn test` failed because Playwright WebKit was not installed. `BrowserEditing.test.ts` launches all three browser engines even though the live Linux matrix uses Firefox. Import, codegen and build succeeded. Provisioning the documented missing engine resolved the failure; the complete rerun passed without a product-code change. Evidence: `<wGac repo>/build/rpxplat-20261116/gacjs-test.log`.

## Completed observations

Firefox 146.0.1 completed the RPT walkthrough, canceled/accepted File close, renderer replacement and live takeover, repeated local shortcuts, and the fresh intentional fatal case. Every fatal check observed one Core-authored error package, exact `This is a fatel error!` mask, one expected page error, and nonzero Core exit. Both local shortcuts use the canonical Super labels. Mouse checks include Playwright Left/Middle/Right and separately identified DOM-synthetic five-button, movement, double-click, horizontal/vertical wheel and Alt/Super/Ctrl/Shift combinations before and after takeover. URL stayed unchanged. Evidence: `<wGac repo>/build/rpxplat-20261116/rpt-browser-complete.log`, `rpt-browser-normal-events.json`, `rpt-browser-normal-core.json`, `rpt-browser-normal.png`, and `rpt-browser-fatal.json`.

Browser input does not establish native desktop global-shortcut registration. The initial grid assertion included the unchanged Edit menu among new cells; the harness was corrected to compare the pre-add visible labels. This was a test-procedure error, not a product fix.

The complete root `yarn test --concurrency 1` passed after provisioning a private Ubuntu 24.04 WebKit runtime. The system Node 22.22.1 completed extraction; Node 26.5.0 had stalled while extracting the browser archive. The private WebKit launcher retains the supplied library path. Playwright's system-library-cache preflight was disabled for this private runtime after checking linked libraries and successfully launching WebKit; all actual browser tests still ran. No project test or assertion was disabled. Linux-only skips of the Windows live E2E harness remain separate from the live Firefox matrix. Evidence is under `<wGac repo>/build/rpxplat-20261116/`: `gacjs-test-final.log`, `webkit-runtime/`, and `browsers/`.

All five RVM host modes passed exact bracket/brace greeting edits, replacement/takeover, normal shutdown and idle/blocked host loss. Network/browser modes rejected a second host without disturbing the first. Both stdio modes reaped their normal child. Fatal cases each received one Core error with exact `RemotingTest_RvmHost disconnected.` and terminated Core nonzero. Evidence: `<wGac repo>/build/rpxplat-20261116/rvm-browser-*-results.json` and matching logs/fatal captures.

Firefox FCT uses actual Playwright mouse/keyboard input and browser text insertion; Core Controls are read for assertions. The default website advertises only its CSS default `Segoe UI` font. Font-size, effects and color checks use that available family; enumeration/selection of a second family is unavailable under this default configuration. Native runs separately verified DejaVu Sans Mono. Default face attributes may be omitted from serialized XML.

A Preview-refocus attempt clicked the newly created hyperlink, leaving its dialog open for subsequent mode commands. A focused P/S/E shortcut probe passed; the corrected procedure focuses unlinked text. This was a harness correction, not a product fix.

After a Toolstrip menu command, ordinary Playwright keystrokes correctly replace the selected range. `keyboard.insertText` alone does not type into the noneditable renderer container after that menu takes DOM focus. The harness now uses ordinary keys for printable ASCII and separately identifies direct Unicode insertion. Evidence: `browser-seed-probe.log`; no product change was needed.

## Browser-specific checks

| Test Item | Result |
| --- | --- |
| Firefox FCT rich editors, gallery, layout and lifecycle | 2026-11-16 (passed; detailed and fresh lifecycle runs) |
| Browser touch acquisition, scrolling, cancellation and replacement | 2026-11-16 (passed; Chromium acquisition and Firefox synthetic checks distinguished below) |
| Browser point editing and file-picker double taps | 2026-11-16T18:53:21.692Z (passed; Chromium CDP touch) |
| Live Chromium composition and Core selection ownership | 2026-11-16T18:48:33.410Z (passed; auto and contenteditable adapters) |
| Native operating-system IME attempt | 2026-11-16 (attempted; native delivery unavailable in the locked session) |
| Raw Clock canvas pixels, resize and DPR | 2026-11-16 (passed at DPR 1 and 2; resized DPR 2 viewport) |

Both Firefox editors completed context/toolbar commands, font/colors/effects, hyperlinks and read-only modes, images, private persistence and RTF/HTML exports. Gallery checks cover all eight styles, repeat application, sample menus, and a real 640-pixel collapsed Style group. Layout and selected document/range survive palette changes. Responsive cycling reached 240/350/440/680 widths and short/tall heights; each of the four appearances scrolled through all 24 film titles. Outside-page mouse release was an invalid first capture probe; releasing outside the renderer but inside the page passed resize/title capture cleanup, subsequent unpressed movement, ordinary resize and close. Evidence: `fct-browser-ribbon-passed.log`, `fct-browser-matrix.log`, `fct-browser-responsive-sizes.json`, `browser-capture-run.log` and `browser-mouse-capture.json`.

Firefox normal replacement/takeover retained the complete private document, repeated the local shortcuts and five-button modifier checks, and exited Core normally. Clock screenshots changed with Core time, exactly one canvas survived page changes, and backing dimensions matched DPR 1 and 2 before/after viewport resize. The headed Firefox IME attempt activated IBus CangJie3, verified the owned X11 window and focus, and sent candidate keys in both Search and Document; both stayed exactly `ABCD` at the interior caret. Native candidate selection/cancellation is unavailable in this locked session. The original `xkb:us::eng` engine was restored. Evidence: `fct-browser-lifecycle.log`, `browser-dpr.log`, `browser-clock-pixels-*.json`, and `browser-native-ime-summary.json`.

The main Chromium touch run exposed an outdated assertion in `<GacUI repo>/Test/Touching_Browser.mjs`: it expected a short press on a selected endpoint to preserve the range. The current SOP requires short-press collapse and long-press preservation. The helper now checks those separately and restores the exact range before scrolling/momentum assertions. The original failure evidence is retained as `touch-stale-assertion.log` and `touch-stale-assertion-evidence.json`; the corrected helper passed the complete rerun.

The corrected main touch run passed 167 recorded checks, including raw rectangle/contact pixels, rotation/scale, extra-contact pause, swipe clamping/reversal/deletion, list selection, document range retention, both momentum modes and palette replacement. The separate double-tap run passed 55 recorded checks plus detailed Ribbon/Toolstrip editor assertions, including exact one-image insertion and Undo/Redo. Supplemental Chromium 145.0.7632.6 checks passed contact membership/ID slots, consecutive transport indices, decimal-string sequence/timestamps, PlatformCanceled and CaptureLost cleanup without fabricated Up/click, accepted/rejected stream replacement, ordinary-button rejection, secondary-contact suppression, descendant touch-action and normal outside input. Capture-loss injection first establishes active capture with a move; releasing merely pending capture does not generate a lost-capture event. Firefox 146.0.1 also passed a separate DOM-synthetic point stream with explicitly stubbed capture APIs. Neither synthetic events nor CDP touch establishes physical hardware input. Evidence: `touch-step3-browser.json`, `touch-step3-browser-pairing.json`, `touch-browser*.log`, `browser-touch-extra-result.json`, and `browser-firefox-touch-result.json`.

Live Chromium tests passed both default EditContext and forced contenteditable adapters: exact Unicode insertion, forward/reversed selection replacement, canceled composition preserving Core text/ranges, subsequent commit replacing those ranges, multi-paragraph replacement, Undo/Redo and tab persistence. The harness waits for each committed paragraph before the next key and includes empty paragraphs when decoding Core XML. This remains separate from the unavailable native IME attempt. Evidence: `browser-editing-live.log`, `browser-editing-live-result.json`, and the composition-event captures.

The final read-only command pass succeeded in rebuilt standard native, hosted native, native remote and Firefox surfaces. Preview and Selectable reject hyperlink edit/removal and Ribbon style application without changing the document; returning to Editable restores both operations. Every run closed normally. Native popup Escape is addressed to the popup window; hosted/browser outside dismissal uses a visible main-window control. Evidence: `readonly-matrix.log` and `fct-*-readonly-style.json` in the evidence directory.

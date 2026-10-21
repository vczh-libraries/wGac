# Test Matrix Card 2026-10-21T11:19:05.560Z

## Test Matrix

| Test Item | 1st |
| --- | --- |
| [Linux][`/RPT`][`/MiniHttp`] | PASS |
| [Linux][`/FCT`][`/MiniHttp`] | PASS for listed evidence; OS/hardware limitations below |
| [Linux][`/RVMT`][`/MiniHttp`][Native `RemotingTest_RvmHost` over network] | PASS |
| [Linux][`/RVMT`][`/MiniHttp`][Native `RemotingTest_RvmHost` over stdio `/Cli:<path>`] | PASS |
| [Linux][`/RVMT`][`/MiniHttp`][GacJS browser host `?rvmhost`] | PASS |
| [Linux][`/RVMT`][`/MiniHttp`][GacJS Node `cli.js` over network] | PASS |
| [Linux][`/RVMT`][`/MiniHttp`][GacJS Node SEA over stdio `/Cli:<path>`] | PASS |

## Issues Found and Fix

- A rejected second browser RVM host exposed cleanup before renderer startup twice: `ResizeObserver` was undefined. Guarding renderer teardown with `rendererStarted` preserves the expected host-rejection error. The lifecycle unit regression and repeated browser-host row pass after the fix.

## Observations and limitations

- Linux GacJS runs in Firefox; Chromium supplies browser-level touch acquisition coverage in the Wasm matrix. Native wGac touch exclusions do not apply to either browser.

- Firefox FCT: both lists add/clear; distinct Search/rich-editor markers retained across tabs and renderer takeover; local Ctrl+Q and Ctrl+Alt+Super+Q dialogs; left/middle/right and Alt/Super/Ctrl/Shift readouts. Layout subpages, retained easy-layout editor/choice through Default/Ember/Aurora/Default; responsive modal 240/350/440/680 by 320 and 680 by 220/520, reverse cycles, equal footer widths, eight account menu entries, selection/scroll and OK/Cancel/title-close. Raw Clock pixels update, hide/show and resize; close/reconnect and live takeover preserve state. Full Firefox DOM-synthetic Touching/TouchingList/ordinary-touch operations pass through the HTTP renderer; no page errors. Force Exit settles normally. Physical touch and OS global-hotkey activation are not inferred.

- RPT: initial/clicked label, shortcuts, three grid rows/clear, document modal, reconnect/takeover state, normal confirmed close, and a fresh fatal run with exactly `This is a fatel error!`, one Core error packet and one browser rethrow.

- All five Firefox RVM host modes: initial/edited greetings, normal shutdown/host cleanup, idle host loss and host loss during editing. Network modes reject a second host without replacing the accepted one. Native/Node process tests suspend the accepted host before editing and kill that exact process; stdio tests track the executable below its shell. Browser-host pending-RPC test holds the actual reply, confirms an automation read remains queued, stops the host and observes the read release plus exactly one Core fatal packet/browser rethrow. Renderer takeover preserves the accepted browser host.

- Additional Chromium HTTP acquisition covered all five mouse buttons with no modifiers, Alt, Super, both, and Ctrl/Shift/Alt/Super. Exact down/up and modifier labels passed and the URL never changed. Firefox Playwright only accepts left/middle/right in `mouse.down`; its back/forward acquisition is not claimed. Browser global OS shortcuts and physical hardware remain unverified.

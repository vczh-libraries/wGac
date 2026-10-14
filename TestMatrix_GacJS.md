# Test Matrix Card 2026-10-14T13:54:35.543Z

Scope: complete Linux verification from the current remote-protocol job and SOP. Times use UTC. Each row is updated when it starts or finishes.

## Test Matrix

| Test Item | 1st |
| --- | --- |
| [Linux][`/RPT`][`/MiniHttp`] | 2026-10-14T14:04:06.370Z |
| [Linux][`/FCT`][`/MiniHttp`] | 2026-10-14T14:08:21.247Z |
| [Linux][`/RVMT`][`/MiniHttp`][Native `RemotingTest_RvmHost` over network] | 2026-10-14T14:25:02.162Z |
| [Linux][`/RVMT`][`/MiniHttp`][Native `RemotingTest_RvmHost` over stdio `/Cli:<path>`] | 2026-10-14T14:27:48.022Z |
| [Linux][`/RVMT`][`/MiniHttp`][GacJS browser host `?rvmhost`] | 2026-10-14T14:28:42.149Z |
| [Linux][`/RVMT`][`/MiniHttp`][GacJS Node `cli.js` over network] | 2026-10-14T14:27:52.051Z |
| [Linux][`/RVMT`][`/MiniHttp`][GacJS Node SEA over stdio `/Cli:<path>`] | 2026-10-14T14:28:01.220Z |
| [Linux][Import / codegen / build / test] | 2026-10-14T13:54:57.321Z |
| [Linux][Repeated codegen stability] | 2026-10-14T14:02:36.651Z |

## Verification details

Import, codegen, build and all 11 package test scripts passed. Firefox 146.0.1 is used for the live Linux matrix. The second codegen preserved hashes of all tracked files. Windows-only native E2E suites report their expected skip and are not counted as Linux runtime coverage.

RPT passed initial UI, Home mutation, all three populated grid rows and clear, document modal, replacement and takeover with retained state, repeated local shortcuts and five-button modifier/motion/double-click/wheel checks, normal File-menu close and a fresh exact Core-authored fatal error. Mouse4/Mouse5 used canceled browser DOM events; standard buttons used Playwright mouse input. Browser global desktop shortcuts are outside this renderer.

FCT passed dual-list add/clear, distinct search/rich-editor markers, bracket input, replacement with retained text and repeated shortcut/mouse checks. Vertical tabs, Up/Down and Alt access, the five-pixel gap, retained EasyLayout state and Default/Ember/Aurora/Default colors passed. Responsive modal sizes were 240/350/440/680 by 320, plus 680 by 220/520; all 24 films were traversed in each appearance, account menu selection and eight 32-by-32 images checked, grow/shrink cycles and all three close/reopen paths passed. Normal Force Exit ended Core with code 0.

native: initial greeting, Translate, renderer replacement, second-host rejection and normal UI close, idle-next-call loss (63 ms), and blocked-delivery loss (5336 ms) passed. Both fatal runs captured exactly one Core !Error and one matching page error.

stdio: initial greeting, Translate, renderer replacement, Core !Exit and child reaping, idle-next-call loss (56 ms), and blocked-delivery loss (9 ms) passed. Both fatal runs captured exactly one Core !Error and one matching page error.

node: initial greeting, Translate, renderer replacement, second-host rejection and normal UI close, idle-next-call loss (47 ms), and blocked-delivery loss (5373 ms) passed. Both fatal runs captured exactly one Core !Error and one matching page error.

sea: initial greeting, Translate, renderer replacement, Core !Exit and child reaping, idle-next-call loss (65 ms), and blocked-delivery loss (9 ms) passed. Both fatal runs captured exactly one Core !Error and one matching page error.

browser: initial greeting, Translate, renderer replacement, second-host rejection and normal UI close, idle-next-call loss (118 ms), and blocked-delivery loss (5328 ms) passed. Both fatal runs captured exactly one Core !Error and one matching page error.

A fresh RPT fatal run also captured exactly one incoming !Error with This is a fatel error!, one matching page error, the error mask and nonzero Core termination.

## Platform coverage limits

Physical input and displayed font/color fidelity were not inferred from browser automation. Mouse4/Mouse5 used browser DOM event dispatch; Windows-only native E2E suites were skipped as expected on Linux.

## Issues Found and Fix

No browser application defect was observed. Imported snapshot changes reflect current upstream platform paths, element identities and generated index order; repeated codegen was stable.

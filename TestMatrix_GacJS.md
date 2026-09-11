# Test Matrix Card 2026-09-10 15:49:34 PDT

## Test Matrix

| Test Item | 1st |
| --- | --- |
| [Linux][`/RPT`][`/MiniHttp`] | 2026-09-10T23:00:09.546Z |
| [Linux][`/FCT`][`/MiniHttp`] | 2026-09-10T23:03:01.073Z |
| [Linux][`/RVMT`][`/MiniHttp`][Native `RemotingTest_RvmHost` over network] | 2026-09-10T23:05:25.600Z |
| [Linux][`/RVMT`][`/MiniHttp`][Native `RemotingTest_RvmHost` over stdio `/Cli:<path>`] | 2026-09-10T23:06:42.746Z |
| [Linux][`/RVMT`][`/MiniHttp`][GacJS browser host `?rvmhost`] | 2026-09-10T23:09:12.540Z |
| [Linux][`/RVMT`][`/MiniHttp`][GacJS Node `cli.js` over network] | 2026-09-10T23:07:34.068Z |
| [Linux][`/RVMT`][`/MiniHttp`][GacJS Node SEA over stdio `/Cli:<path>`] | 2026-09-10T23:08:16.707Z |

## Issues Found and Fix

## Coverage and limitations

RPT: Home, two local chords, grid 3×3 add/clear, document modal, replacement/takeover and repeated input, detached input rejection, queued File Close with confirmation, and exact Core-authored fatal mask/SIGABRT passed in Firefox. Five mouse buttons/modifier/motion/double-click/wheel payloads were exercised at the browser event boundary; Left/Middle/Right additionally used Playwright mouse input. Physical desktop global shortcuts are not established by browser events. Logs/captures: `/tmp/rpxplat-*`.

RPT normal close returned Core exit 0 and the ordinary HTTP disconnected success mask. Firefox logged one final outstanding request failure after Core stopped; no Core fatal overlay or reconnect occurred.

FCT: both ten-item lists, two distinct retained editors, bracket/braces typing, local shortcuts, full input payload matrix, renderer replacement and normal Force Exit passed.

All five RVM host modes: initial/exact greeting, keyboard updates, renderer replacement and subsequent Translate, normal exit, idle-next-call loss and blocked-delivery loss passed. Manual modes also rejected a second host. Network blocked-call tests suspended the accepted process and observed unread response bytes before termination; browser-host testing intercepted the replacement Request before Core received it. Every fatal run retained the exact `RemotingTest_RvmHost disconnected.` mask and one matching page error. Both stdio modes exited normally through Core `!Exit` and reaped the owned child. Logs contain timings and exit statuses.

Final import, codegen, build and portable test suites passed after the upstream fix. A second codegen left every hashed tracked/source-snapshot file unchanged. Captured browser payloads contain left/right bracket KeyDown codes 219/221 (0xDB/0xDD) for the bracket/braces marker.

### Latest Core follow-up

After the detached-label and shared async fixes, Firefox repeated the RPT document-dialog/renderer-replacement path successfully. Explicit bracket key presses and Shift+BracketLeft/Right produced exact `Hello[Ab]{Cd}` and `IOKeyDown` codes 0xDB/0xDD with both shift=false and shift=true. Normal Force Exit returned Core exit 0.

### Snapshot line endings

The snapshot copier introduced CRLF-only diffs in roughly 5,000 JSON files from the upstream working tree. The generator now normalizes CRLF to LF while preserving content and BOM. `yarn run import`, `yarn codegen`, `yarn build` and `yarn test` all passed; a second codegen changed none of 5,493 tracked-file hashes.

Final Core follow-up after the narrowed hosted mouse fix also passed in Firefox: document modal, renderer replacement, retained Home state, Ctrl+Q and confirmed normal exit (Core exit 0).

Final pipeline after all fixes: import, codegen, build and tests passed (10 packages). A second codegen changed none of 5,558 tracked and new source-file hashes, including the new initial-name snapshots. The 37 existing file-dialog frame-tree differences are the initialized empty editor's nonwrapping maxWidth changing from 1 to -1; their regenerated protocol/snapshot files were retained together. Current-day highlight noise was removed before copying. All owned browser/Core/host processes and the static server were closed.

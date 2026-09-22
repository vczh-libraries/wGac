# Test Matrix Card 2026-09-21T19:07:36-07:00

## Test Matrix

| Test Item | 1st |
| --- | --- |
| [Linux][`/RPT`][`/MiniHttp`] | 2026-09-21T19:24:24-07:00 |
| [Linux][`/FCT`][`/MiniHttp`] | 2026-09-21T19:51:30-07:00 (fixed) |
| [Linux][`/RVMT`][`/MiniHttp`][Native `RemotingTest_RvmHost` over network] | 2026-09-21T19:54:50-07:00 |
| [Linux][`/RVMT`][`/MiniHttp`][Native `RemotingTest_RvmHost` over stdio `/Cli:<path>`] | 2026-09-21T19:56:24-07:00 |
| [Linux][`/RVMT`][`/MiniHttp`][GacJS browser host `?rvmhost`] | 2026-09-21T20:01:03-07:00 |
| [Linux][`/RVMT`][`/MiniHttp`][GacJS Node `cli.js` over network] | 2026-09-21T19:56:24-07:00 |
| [Linux][`/RVMT`][`/MiniHttp`][GacJS Node SEA over stdio `/Cli:<path>`] | 2026-09-21T19:56:24-07:00 |
| [Linux][Import / codegen / build / test] | 2026-09-21T21:26:13-07:00 |

## Issues Found and Fix

### Easy Layout selected choice is clipped

Firefox reproduces the native sample defect: the combo is 33 pixels wide and shows only `Fir…`. List 0–9 add/clear and independent Search/document markers passed. The sample now reserves 120 pixels for the combo. Regenerated resources pass both arrangement directions, deferred/idempotent rebuilds, all six palettes, 900/1100/1300-pixel viewport widths, replacement and live takeover with retained editor/choice, the complete baseline and repeated shortcut/mouse checks. Force Exit returned Core 0 and the ordinary success mask.

## Verification details

Firefox RPT: Home state, all three populated DataGrid rows and clear, document modal, replacement and live takeover, repeated local shortcuts and mouse/modifier matrix, confirmed File-menu close (Core exit 0), and a fresh exact Core-authored fatal error passed. Mouse4/Mouse5 used canceled DOM boundary events; no physical extended-button or Linux global-shortcut coverage is claimed. Normal close reached the success mask without a fatal overlay.

Native and Node hosts: all four network/stdio rows passed normal RPC and replacement, second-host rejection for network modes, and fresh idle-next-call / blocked-delivery host-loss runs. Each fatal run showed the exact Core-authored message once. Network blocked runs observed unread bytes on the suspended host socket; stdio runs killed the actual executable child and checked reaping. Manual network hosts exit 1 after ordinary requester shutdown; Core exits 0 and the renderer shows its ordinary success mask. Logs: `/tmp/rpxplat-rvm-browser-matrix.log`, `/tmp/rpxplat-rvm-browser-matrix-2.log`.

Browser-host mode also passed normal RPC/replacement/rejection and both fresh fatal variants. For blocked loss, the host page held its replacement `/Request` and RPC `/Response` before stopping only `session.host`; the separate active renderer received the exact Core error once. Log: `/tmp/rpxplat-browser-host.log`. A second codegen preserved all tracked file hashes.

After merging upstream `ae8b8504d`, import, codegen (including generated-code lint), build and all ten package test scripts passed again. The imported snapshot set includes all five new supplementary-character editor families. The complete browser matrix above ran before this final upstream merge; the protocol and browser implementation are unchanged.

The final repeated codegen preserved all 5,636 tracked and newly generated repository file hashes; all five imported supplementary-character frames match the canonical GacUI snapshots.

Final integration: GacUI `9c4491b35` includes the rebased fix and regenerated resources for both architectures. Resource compilation passed; a final CodePack run preserved every release-file hash used by the successful downstream build. All owned UI test processes and servers were closed.

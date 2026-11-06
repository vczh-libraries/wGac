# Test Matrix Card 2026-11-04T06:38:46.156Z

## Test Matrix

| Test Item | 1st |
| --- | --- |
| Import / codegen / build / unit tests | 2026-11-04T07:40:33.645Z (passed) |
| Codegen repeatability | 2026-11-04T07:40:33.645Z (passed) |
| [Linux][`/RPT`][`/MiniHttp`] | 2026-11-04T07:40:33.645Z (passed) |
| [Linux][`/FCT`][`/MiniHttp`] | 2026-11-04T07:40:33.645Z (passed) |
| [Linux][`/RVMT`][`/MiniHttp`][Native `RemotingTest_RvmHost` over network] | 2026-11-04T07:05:00.308Z (fixed; passed) |
| [Linux][`/RVMT`][`/MiniHttp`][Native `RemotingTest_RvmHost` over stdio `/Cli:<path>`] | 2026-11-04T07:05:00.309Z (fixed; passed) |
| [Linux][`/RVMT`][`/MiniHttp`][GacJS browser host `?rvmhost`] | 2026-11-04T07:05:00.309Z (fixed; passed) |
| [Linux][`/RVMT`][`/MiniHttp`][GacJS Node `cli.js` over network] | 2026-11-04T07:05:00.309Z (fixed; passed) |
| [Linux][`/RVMT`][`/MiniHttp`][GacJS Node `cli.js` over stdio `/Cli:<command>`] | Pending: replacement command not verified on this platform |

## Current Node stdio host

The replacement Node command has not been verified on Linux. Platform
verification was explicitly skipped during the Windows maintenance task. The
retired packaged-host row previously passed at 2026-11-04T07:05:00.309Z;
the observations below describe that historical run, not the replacement command.

## Observations and Limitations

- Firefox RPT: Home, both local shortcuts, all five buttons and modifier combinations, movement/double-click/wheels, three complete DataGrid rows and clear, Document modal, renderer replacement/live takeover and repeated shortcuts/extended buttons, confirmed File-menu close, and one exact Core-authored fatal package passed. Initial Core exited 0; fatal Core terminated with SIGABRT. Evidence: `<GacUI repo>/.github/TaskLogs/rp-20261103/firefox-rpt-*.json` and takeover screenshot. Browser events cover DOM-generated extended buttons/modifiers; native OS global shortcuts and physical hardware remain outside browser coverage.

- GacJS install, import, codegen, build and root test commands exited 0. The Windows-only HTTP E2E suites were explicitly skipped, live Firefox protocol checks are recorded separately. Logs: `<GacUI repo>/.github/TaskLogs/rp-20261103/yarn-*.log`.
- A second `yarn codegen` produced the same complete Git diff hash.

- All five RVMT host modes passed repeated `RpcA你好🙂` greetings, normal close, renderer replacement/live takeover, idle-next-call host loss and blocked-delivery host loss. Non-CLI modes rejected a second host while the original kept serving. Every fatal run produced exactly one Core-authored `RemotingTest_RvmHost disconnected.` error; stdio children were reaped. Browser loss used a separate live renderer and held host Request/Response traffic; native/Node network loss used a stopped host with queued socket bytes. Stdio blocked runs confirmed the Core automation request was pending before killing its actual child. Evidence: `<GacUI repo>/.github/TaskLogs/rp-20261103/firefox-rvm-*.json`.

- Firefox FCT passed two list populations/clear, independent search/rich markers, browser shortcut/mouse modifier combinations, palette retention, received clock time with changing hand pixels and correct resized canvas, DOM-synthetic Touching/TouchingList/ordinary/document selection and momentum, replacement/takeover with one new painter, and Force Exit with Core exit 0. The harness restored the Ribbon tab before checking its saved marker.
- Additional FCT responsive/rail checks passed: 240/350/440/680×320 grow/shrink cycles, compact row-height recovery, all 24 films, eight publishers, menu/list selection agreement, independently scrolled accounts at 680×220 then 680×520, natural 32×32 account and 96×96 film image bounds, equal footer widths, all three modal close actions and fresh 240×320 openings. Default/Ember/Aurora/Default retained editor/check/choice state and the five-pixel gap; keyboard Up/Down and displayed ALT labels passed. Evidence: `<GacUI repo>/.github/TaskLogs/rp-20261103/firefox-fct.json`, `firefox-responsive-*.json/png` and `firefox-rail-*.json/png`.

## Issues Found and Fix

- Firefox RVMT exposed dropped supplementary keyboard input (`🙂`): the browser accepted only one UTF-16 unit. The renderer now emits ordered units and UTF-32 Core combines surrogate pairs. The regression builds and all live Unicode host combinations passed.

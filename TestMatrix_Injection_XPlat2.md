# Injection integration verification 2026-10-29

Linux execution of `<GacUI repo>/TODO_Injection_XPlat2.md`. Browser tests used Playwright Firefox 146.0.1 on Ubuntu 26.04, with Core running under `LC_ALL=en_US.UTF-8 TZ=UTC` and Firefox using `America/New_York` and `tr-TR`.

| Check | Result |
| --- | --- |
| `./import.sh`, `./syncProj.sh`, `./build.sh` | Passed twice in that order; repeated import/sync produced no additional source differences |
| Imported libraries, test helpers and canonical entry points | Matched current GacUI inputs and the inventories consumed by the official build |
| GacJS dependency setup, import, codegen, build and tests | Passed; second codegen produced no additional diff |
| Core and native renderer launcher flags | Foreground and `--unblock` forwarded all three slash-prefixed injection switches; renderer also forwarded its selected host and automation port |
| Native Core with Firefox | All 12 individual opt-in combinations, all-enabled and mixed selections passed |
| OPFS documents, renderer loss/replacement and host routing | Passed |
| RPT and browser-host RVMT | Passed, including Unicode RPC and restart after observed host loss |
| Production Core after probe removal | Official full rebuild and real Firefox document/localization/input smoke passed |
| Wayland native applications | Standard/hosted FCT, RPT, standalone RVMT and Core/native-renderer RVMT smoke checks passed |

## Commands

From `<wGac repo>`, ran `./import.sh`, `./syncProj.sh`, `./build.sh`, then repeated the complete sequence. The library/test-helper source snapshots, shared arguments, renderer entry point, FCT handlers and RVMT initialization were compared with their canonical GacUI inputs. The imported Windows HTTP fallback changes came from the existing upstream release; no imported file was edited manually.

From `<GacJS repo>/Gaclib`, ran `yarn install`, `yarn run import`, `yarn codegen` twice, `yarn build` and `yarn test`. Root tests included the browser-backed portable OPFS suite and Workflow RPC driver integration. The normal Windows-only native E2E gate skipped its suite; the Firefox runs below supplied the Linux live evidence.

Core foreground launcher:

```bash
LC_ALL=en_US.UTF-8 TZ=UTC ./test_core.sh --app:fct --protocol:minihttp --remoteDT --remoteLocale --remoteFS
```

The final clean Core used the same command with `--unblock`. Its actual process arguments contained `/MiniHttp /FCT /remoteDT /remoteLocale /remoteFS`. Firefox opened the HTTP entry with `?remoteDT&remoteLocale&remoteFS&corehost=127.0.0.2`. Individual runs selected only the flags listed below; other features were disabled. Fresh processes used the rebuilt Core executable with `/MiniHttp /FCT` and the corresponding slash switches.

Native renderer forwarding was observed in both of these launcher paths:

```bash
LC_ALL=en_US.UTF-8 ./test.sh --app:renderer --corehost:127.0.0.2 --remoteDT --remoteLocale --remoteFS
LC_ALL=en_US.UTF-8 ./test.sh --app:renderer --corehost:127.0.0.2 --port:8890 --remoteDT --remoteLocale --remoteFS --unblock
```

## Provider matrix

Each cell below was a separate live Firefox/FCT run. Actual browser List and Localization interactions accompanied fresh Core API queries. Captured protocol requests identified provider selection independently of cached application labels.

| Feature | Core off / browser off | Core off / browser on | Core on / browser off | Core on / browser on |
| --- | --- | --- | --- | --- |
| DateTime | UTC predecessor; no injection | UTC predecessor; no injection | UTC predecessor; no injection | New York conversions; both injection request types observed |
| Locale | en-US predecessor; no injection | en-US predecessor; no injection | en-US predecessor; no injection | tr-TR system/user defaults, browser enumeration and locale requests |
| Filesystem | `NATIVE_ONLY` sentinel | `NATIVE_ONLY`; no injection | Empty read-only `/`; sentinel inaccessible | `BROWSER_ONLY` sentinel from OPFS and filesystem requests |

All three enabled selected the browser for all three features. A mixed run with Core DateTime/filesystem and browser Locale/filesystem selected only the browser filesystem; DateTime and Locale retained their predecessors. The request trace contained only filesystem injection in that mixed run.

## Observations

- DateTime converted `2024-01-01 02:03:04.567` UTC to `2023-12-31 21:03:04.567` and `2024-07-01 02:03:04.567` UTC to `2024-06-30 22:03:04.567`. Reverse conversions preserved the complete timestamps. Winter/summer offsets, year/day boundaries, current local time and millisecond precision were checked.
- Locale queries covered invariant/system/user defaults, enumeration, all date/time format lists, short/long day and month names, lower/upper and linguistic case mappings. The captured requests distinguished browser operations from identical predecessor results; the temporary probe also checked the existing local formatting/comparison/search behavior.
- OPFS checks covered Unicode paths, a 262147-byte binary pattern, empty files, read/peek/seek, ReadWrite preservation, WriteOnly replacement, close persistence, metadata, enumeration, rename/delete, missing paths, unsupported copy failure and absence of a partially created destination. Explicit close followed by destruction did not write back twice.
- Both FCT document editors loaded the same-origin document fixture, accepted typed Unicode markers, saved private files, discarded unsaved edits on reload, and exported HTML/RTF with two embedded images each. Saved bytes survived REFRESH. The Core-native fixture directory retained only its unchanged sentinel. HTML spans and RTF control words were decoded for text assertions.
- After Core actually observed renderer loss, fresh queries returned DateTime/Locale predecessors and an empty filesystem. Capability-off/on takeover and reconnection selected the expected providers without hanging. Closing a stream opened by the old renderer returned the expected connection-loss error.
- Mixed-case HTTP query names selected all three capabilities. Checkbox changes took effect only after REFRESH; off/on REFRESH cycles retained the selected host and OPFS fixture.
- Browser UI and request origins verified default localhost, explicit `127.0.0.1`, non-default `127.0.0.2`, hostname `vczh-Ubuntu-2404`, `::1` and `[::1]`.
- Firefox RPT exercised Click Me, grid add/clear, the document's embedded button and its modal, then confirmed File-menu close with Core exit 0.
- Firefox RVMT used Core `/MiniHttp /RVMT` and `?rvmhost&corehost=127.0.0.2`. A real `Translate` call returned `Hello, XPlat你好!`; both browser clients used the selected host. REFRESH removed the accepted host. A fresh Core input operation made Core observe the loss and terminate with SIGABRT. Restarting Core restored Unicode RPC; normal shutdown exited 0. No heartbeat or recovery was added.
- The temporary API probe was removed. The official `test_core.sh` full rebuild then passed fresh Firefox localization, actual input, both document editors and native-storage isolation again; its Controls response contained no probe field.
- Native Wayland renderer input crossed the protocol for FCT and RPT. Renderer loss/replacement and a concurrent takeover retained application state; the detached renderer exited 0. RPT cancel/confirm close and normal Core/renderer shutdown passed. `./test.sh --app:fct` and `--app:fct --hosted` both passed actual list add/clear and shutdown. `./test.sh --app:rvmt` and Core/native-renderer RVMT both completed Unicode RPC with the rebuilt native host using `/MiniHttp /corehost:127.0.0.2`.

## Harness corrections and coverage limits

The temporary filesystem probe initially expected unsupported copy to succeed; its expectation was corrected to the documented failure contract. Temporary native checks were also corrected to follow the list's continuing item counter, include hosted popup windows in the automation tree and allow the normal bounded automation response timeout. These did not require production changes.

This is Linux execution. macOS/WebKit/Safari, Windows HTTP.sys, another physical computer, physical touch and compositor-global-shortcut consent were not tested. Firefox Unicode input used keyboard events on the actual renderer input surface. Browser and native automation results do not establish physical-device coverage. WASM results are recorded separately in `<GacUI repo>/.github/TaskLogs/TestMatrix_Wasm.md`.

# Native injection verification

Execution of `<GacUI repo>/TODO_Injection3.md` on Linux x86_64,
Ubuntu 26.04.1 LTS, 2026-10-29. macOS, GacJS and Windows-only transports/UIA
are outside this run.

## Starting state

GacUI, wGac, VlppOS, Workflow and Tools were clean. The root instructions
matched `<Tools repo>/MonoRepo.md`. The real Wayland session supplied
`WAYLAND_DISPLAY=wayland-0` and `XDG_RUNTIME_DIR`; tests used `C.UTF-8`.

| Repository | Initial revision |
| --- | --- |
| GacUI | `4712bba1c1ea48c8d9cdd194142eab6ef2aca1bb` |
| wGac | `06d2ed07c395ade240a781d4a7922ae4ac5086e2` |
| VlppOS | `8bfae82b3eee44a3ed480f57246d18b235c658c2` |
| Workflow | `4eec736eb4d13964682f4aba8f8b9ba89c224afb` |
| Tools | `b322d42dd20001d17e36fd7bc6ecc8a1a9d4a271` |

During verification, GacUI origin advanced to
`5b5482da44cd3c9ab488af1b6d43e8e2504bbc96` with a Project.md argument-documentation
reorganization. Read and fast-forwarded that change; it changes no tested source.

Commands below run from `<wGac repo>` unless another directory is stated.
Temporary drivers, logs, JSON observations and isolated fixtures are retained
locally under `<wGac repo>/.codex-injection3`, excluded from Git. They are test
evidence, not production dependencies. All UI input used the application's
HTTP automation endpoints. No keys were sent to the Codex terminal.

## Synchronization and builds

Executed `./import.sh`, `./syncProj.sh`, then `./build.sh`. Import and resource
synchronization succeeded. The first native build exposed missing Cairo/Pango
development packages. The documented `sudo ./build-prerequisites-ubuntu.sh`
installation was requested; this session could not use sudo. Following the
repository's disposable-container fallback, Ubuntu package-manager downloads
of the documented prerequisites were extracted into a temporary Bubblewrap
filesystem. The official build command was then:

```bash
.codex-injection3/build-container.sh ./build.sh
```

This provided ordinary system include/library paths inside the disposable
filesystem. No host packages, CMake files or committed build scripts were
changed to depend on the temporary environment. Native applications ran in the
real host Wayland session. The official build succeeded. The user subsequently installed the documented
prerequisites successfully; the final build runs directly on the host with
`./build.sh` (Cairo 1.18.4, Pango 1.57.0, libdecor 0.2.5).

Static comparison confirmed all 48 imported framework, skin and test-helper
files matched `<GacUI repo>/Import` and `<GacUI repo>/Release`, allowing their
required line-ending differences. SharedArguments, renderer GuiMain and both
FullControlTest event-handler files matched their canonical sources. Active
inventories contain no obsolete `FullControlTestPalette` reference. Repeated
resource generation removed obsolete commented document-handler stubs and
regenerated the FullControlTest resource payload; these are generated changes.

The transport fix below was published with:

```bash
# From <VlppParser2 repo>/Tools/CodePack:
./Bin/CodePack ../../../VlppOS/Release/CodegenConfig.xml
# From <wGac repo>:
cp ../VlppOS/Release/*.h ../VlppOS/Release/*.cpp ../GacUI/Import/
./import.sh
.codex-injection3/build-container.sh ./build.sh
```

Only root release C++ files were copied; IncludeOnly was not imported.

## Automated native tests

From `<VlppOS repo>/Test/Linux/UnitTest`:

```bash
MAKEFLAGS=-j4 ../../../.github/Ubuntu/build.sh
LC_ALL=C.UTF-8 ./Bin/UnitTest /C /F:TestInterProcess.cpp /F:TestInterProcess_AsyncSocket.cpp /F:TestInterProcess_AsyncSocket_MiniHttpApi.cpp /F:TestInterProcess_HttpRequest.cpp
```

Passed **4/4 files, 110/110 cases**, exit 0, including a repeat after the final
registration-race refinement. New coverage checks pending-poll loss with and
without error promotion, intentional replacement, subsequent delivery, explicit
context cancellation, peer loss, shutdown, late notification registration and
preservation of accepted-response completion. The build also exposed an
existing dependent-template call missing `template` in a generic test lambda;
that Clang compile error was corrected.

From `<GacUI repo>/Test/Linux/UnitTest`:

```bash
MAKEFLAGS=-j2 ../../../.github/Ubuntu/build.sh
LC_ALL=C.UTF-8 ./Bin/UnitTest /C /F:TestRemote_Startup.cpp /F:TestDocumentConfig.cpp /F:TestControls_Ribbon_Gallery.cpp
```

The initial run passed **3/3 files, 167/167 cases**, exit 0. These native unit
tests use mock protocol implementations; they supplement the real process
checks below. The second run with regenerated VlppOS imports also passed **167/167 cases**.
Four generated compiler snapshots differ only in declaration/property ordering;
their changes are retained in a separate snapshot commit. Rendering-frame
content did not change.

## Instrumented real Core and Wayland renderer

Temporary public injection implementations were installed in the compiled
renderer entry point. Core exposed fresh-query commands through a temporary
automation-service subclass. Tests used separate logical/Core storage and two
renderer backing directories with `CORE-SENTINEL`, `RENDERER-A` and
`RENDERER-B`. Request logging distinguished remote methods from Core-only
methods. Cached labels were not used to infer routing or fallback.

Launch form:

```bash
MAKEFLAGS=-j4 INJECTION3_LOGICAL="$PWD/.codex-injection3/logical" ./test_core.sh --app:fct --protocol:minihttp -remoteDT -remoteLocale -remoteFS
INJECTION3_TAG=renderer-a INJECTION3_LOGICAL="$PWD/.codex-injection3/logical" INJECTION3_BACKING="$PWD/.codex-injection3/renderer-a" ./test.sh --app:renderer --corehost:localhost --port:8889 -remoteDT -remoteLocale -remoteFS
node .codex-injection3/matrix.mjs remoteDT,remoteLocale,remoteFS
node .codex-injection3/lifecycle.mjs
```

All **25 Core/renderer configurations** passed.
The matrix driver launches fresh renderers with no flags, each flag alone,
and all three flags. Core configurations are no flags, each flag alone, and
all three flags. It checks before connection, connected results, and fresh
post-disconnect results for every row. The remaining-row driver invokes the
same Core wrapper with `--unblock`, retaining the printed PID and actual
executable arguments. Renderer background forwarding uses reordered host,
port and injection options.

| Feature | Distinguishable expectation and observed checks |
| --- | --- |
| DateTime | Renderer conversion offsets of +14 hours in winter and +13 in summer; both directions, year/day rollover and milliseconds preserved. Core UTC and arithmetic retained; LocalTime derived from Core UTC, never the fake renderer's 2099 clock. |
| Locale | Renderer-tagged defaults, enumeration, five format-list APIs, day/month names and four case conversions. Date/time/number/currency formatting, comparison, ordinal comparison and searching retained Core behavior. |
| Filesystem | Renderer sentinel only when both flags enabled. Core storage when its flag is off. With Core enabled and renderer unavailable/disabled, root enumeration succeeds empty and opens/mutations fail. |
| Lifecycle | Fresh predecessor DateTime/Locale and empty injected filesystem after disconnect. Live takeover by renderer B with Locale only changes locale while filesystem stays empty. Reconnecting B with all flags selects its own backing storage. |

With fake injection disabled but fresh Core probes still available, actual
renderer public APIs and Core routed results agreed: winter
`2024/12/31 15:30:42.123`, summer `2024/6/1 15:30:42.456`, `en-US`,
`ProbeAbC` lowercased to `probeabc`, native format lists and filesystem access.
This is the Unix EnUsLocaleImpl baseline, not evidence inferred from changing
the host locale.

The argument driver (`node .codex-injection3/arguments.mjs`) passed all
**25 invalid-option cases**. Native duplicates, empty/duplicate/malformed
hosts, URL/path/port forms and invalid bracketed IPv6 exited nonzero. `strace`
confirmed no `bind`/`listen` before rejection. Wrappers rejected Core host
options and host options on non-renderer targets.

## Filesystem and documents

The isolated filesystem matrix passed nested Unicode/space names, path joining
and relative paths, enumeration, create/copy/rename/delete, metadata, missing
opens, empty files and a **262,147-byte** arbitrary-binary round trip. Verified
actual renderer bytes and unchanged Core sentinel/storage. Access-mode checks
covered read-only, write-only, read/write truncation, seek/position/size,
zero-byte writes, buffered snapshots, successful close writeback and no second
writeback after explicit close plus destruction. The corrected temporary
mapping implementation produced the expected relative path `binary.bin`.

Separate intentional-failure sessions verified retained buffered bytes after
renderer loss and rejection of stale stream closes after disconnect/takeover,
including a read-only stream. Normal UI sessions closed their streams first.

Executed temporary drivers:

```bash
node .codex-injection3/doc-basic.mjs
node .codex-injection3/dialogs.mjs
node .codex-injection3/inspect-exports.mjs .codex-injection3/renderer-a/RIBBON.rtf .codex-injection3/renderer-a/RIBBON.html .codex-injection3/renderer-a/TOOLSTRIP.rtf .codex-injection3/renderer-a/TOOLSTRIP.html
```

Both editors loaded the isolated copy of `<GacUI repo>/Test/TestDocument.bin`,
edited recognizable Unicode text, saved separately, changed again and reloaded.
Full document XML, text, six paragraphs/alignment, both embedded images and all
13 styles (including unused styles) survived. Cross-loading each editor's output
verified handler ownership. Selection copy/paste, undo and redo passed.

Actual private-save dialogs cleared Modified only after successful persistence.
RTF/HTML dialogs preserved Modified; canceled load/save dialogs preserved the
document and created no canceled output. The direct-handler driver separately
verified that persistence itself does not clear Modified outside the command.

Added a 291x212 JPEG and inspected the RTF/HTML image contents: each export
contained three valid embedded PNGs (291x212, 1250x500 and 256x256), with valid
PNG framing and decompressible pixel data. Original image paths were unavailable
in the isolated fixture; repository assets were not removed. The no-injection
Core row repeats this export to Core-local storage.

Actual ribbon popup items were exercised at wide and collapsed widths, including
scrolling the Content group into view. Header1 and Strong changed the document
style, and selecting an already-selected item preserved the range
`(0,0)-(0,7)`. Reopening, resizing and repeating after switching to Aurora and
refreshing the theme passed. These observations used popup items, not inline
gallery highlights.

## Hosts and disconnects

`node .codex-injection3/hosts.mjs` combined all injection flags with automation
port 8891. Each successful destination exposed Core Controls and renderer Dom,
accepted IO through both endpoints, and completed real document load/edit/save/
reload with renderer-owned output:

| Destination | Result |
| --- | --- |
| omitted | passed |
| `localhost` | passed |
| `127.0.0.1` | passed |
| `vczh-Ubuntu-2404` | passed |
| `172.19.215.186` | passed |
| `::1` | passed |
| `[::1]` | passed |
| scoped link-local `fe80::215:5dff:fe01:250e%eth0` | rejected before launch by the existing host parser; not passed |

Protocol traffic stayed on 8888. These are same-machine destinations; no second
computer was available. IPv6 loopback succeeded, but scoped link-local and
cross-machine IPv6 communication are not established by this run.

After the transport fix, killing a renderer while its folder enumeration was
delayed returned `enumerated=false, count=0` in **466 ms**. Core remained usable,
its unavailable filesystem stayed empty, and a new renderer connected normally.
Core-initiated exit ended the renderer with **exit 0**, without a fatal overlay.

## Defects fixed

1. `<wGac repo>/WGac/Renderers/WGacRendererImpl.cpp`: native startup never
   initialized `LC_CTYPE`. A native syscall trace showed `mkdir` receiving an
   empty path for a Unicode name. Shared standard/hosted/raw startup now selects
   the environment character locale before controller creation. The Unicode
   filesystem and real dialog checks passed after this fix. README translations
   describe the UTF-8 requirement; date/number formatting is unchanged.
   The final disabled-feature run exposed the same missing initialization in
   `<GacUI repo>/Test/GacUISrc/RemotingTest_Core/Main.cpp`: a Unicode private-save
   destination produced `demo::WriteFctDocument()#Cannot open output.` while
   renderer-owned output succeeded. Core now also initializes `LC_CTYPE`; both
   editors passed Unicode Core-local save/reload/export after rebuilding.
2. `<VlppOS repo>/Source/InterProcess/AsyncSocket`: losing the waiting MiniHTTP
   poll did not notify the logical connection until another server write. LLDB
   showed Core stuck in `GuiRemoteProtocolAsyncJsonChannel::BatchWrite` with the
   dead renderer still marked connected. Request contexts now support pending
   cancellation notification, and the current poll reports loss through the
   existing nonfatal-local-error/promotion policy. Intentional replacement and
   shutdown remain nonfatal. Notification is installed after the registration
   queue drains to cover overlapping registrations without reentrant Stop
   deadlock. No heartbeat, disconnect acknowledgement or recovery protocol was
   added. Owning knowledge-base pages and generated consumers are updated.

3. `<GacUI repo>/Test/GacUISrc/CppTest_Rvm/GuiMain.cpp` and
   `<GacUI repo>/Test/GacUISrc/RemotingTest_Core/GuiMain.cpp`: normal RVM exit
   retained service proxies until after `GuiApplicationInitialize` finalized
   global storage and closed the shared HTTP listener. GDB breakpoints traced
   host-loss injection to `SocketHttpRegistry::FinalizeResource` and the later
   `RpcInjectedException` to the generated proxy destructor's `ObjectHold`.
   Both apps now destroy their window, release owned proxies and stop RPC from
   `GuiMain`, before global cleanup. The outer teardown remains safe after this
   earlier stop. The canonical standalone entry point is synchronized into
   wGac. Active host-loss policy is unchanged.

## Final native verification

Restored the original renderer Main.cpp and Core GuiMain.cpp, removing both
temporary includes/substitutions. Then ran, in order:

```bash
./import.sh
MAKEFLAGS=-j4 ./syncProj.sh
CMAKE_BUILD_PARALLEL_LEVEL=2 ./build.sh
```

All three passed; the final build used the installed host dependencies directly.
Repeated the 48-file comparison and canonical entry-point/handler checks: passed.
No temporary hooks remain in authored, generated or imported sources.

The native UI driver handles MultiWindow popups through their window IDs and
opens cascading submenus by hovering. It compares document text independently
of per-character XML run boundaries. Early driver assertions were corrected
and the corresponding checks repeated.

Passed final all-enabled and all-disabled Core/renderer sessions with real
public implementations: both editors loaded, typed Unicode text, privately
saved to Unicode filenames, changed, reloaded identical XML, and exported
RTF/HTML with both embedded PNGs validated. Renderer loss left Core available;
replacement and concurrent takeover preserved the complete document. Old and
active renderers exited normally, and Core/renderer shutdown returned 0.

Both standard and hosted standalone FullControlTest repeated the same two-editor
file-dialog/persistence/export workflow and exited 0.

The native RemoteProtocolTest workflow passed its clicked-button state, keyboard
shortcuts and dialogs, all five mouse buttons with modifier combinations, the
three-column/three-row grid and Clear command, and the document's embedded
button/modal dialog. Renderer disconnect, replacement and live takeover retained
Home state. File-menu Close with confirmation ended Core and renderer with
exit 0. The driver waits for final document layout before targeting its embedded
button and counts grid cells by column, accounting for wrapped text.

Final native driver commands (each starts and cleans up its owned processes):

```bash
node .codex-injection3/native-workflows.mjs standard
node .codex-injection3/native-workflows.mjs hosted
REUSE_CORE=1 node .codex-injection3/native-workflows.mjs enabled
REUSE_CORE=1 node .codex-injection3/native-workflows.mjs disabled
REUSE_CORE=1 node .codex-injection3/native-rpt.mjs
node .codex-injection3/native-workflows.mjs core-rvm
node .codex-injection3/native-workflows.mjs rvm
node .codex-injection3/native-rvm-cli.mjs
node .codex-injection3/native-rvm-loss.mjs core
node .codex-injection3/native-rvm-loss.mjs standalone
node .codex-injection3/verify-final.mjs
```

`REUSE_CORE=1` uses the existing Core binary after a successful official wrapper
build, avoiding an unnecessary rebuild between sessions. Core and host were
built from their respective `<GacUI repo>/Test/Linux/RemotingTest_Core` and
`<GacUI repo>/Test/Linux/RemotingTest_RvmHost` directories with
`MAKEFLAGS=-j4 ../../../.github/Ubuntu/build.sh`. Network RVM launches Core with
`/MiniHttp /RVMT`, then the host with `/MiniHttp`, then the native renderer.
The stdio case adds `/Cli:<built-host-path>` to Core and lets Core own that child.

After the shutdown fix, Core RVM passed real `Translate` calls, rejection of a
second network host with the original host still serving subsequent calls,
and normal exit 0 for Core and renderer. A separate stdio-host session passed
RPC and normal exit 0 as well. Killing the accepted network host and typing
again still terminated Core nonzero within 323 ms, with renderer
`Dom.fatalError` exactly `RemotingTest_RvmHost disconnected.`; exact `!Exit`
then closed the retained renderer with exit 0. This expected fatal test used
fresh processes, separately from normal-shutdown verification.

Repeated the ordered import/sync/build after the RVM source update; all three
commands succeeded. The rebuilt standalone RVM passed `Translate`, rejection
of a second host, subsequent calls through the original host and normal exit 0.
In a separate intentional host-loss session it still terminated with SIGABRT
from an unhandled RPC exception, within 1,124 ms of the next input.

The final audit passed all **48 imported files** and **8 canonical source
copies**, including the corrected standalone RVM entry point. Diff whitespace
checks respected generated CRLF files. Authored/imported/generated sources and
the actual Core, native renderer, RVM and FullControlTest binaries contain no
temporary injection/query hooks. The relevant upstream delivery revisions are
GacUI `429db1ba5` and VlppOS `769f64e`; wGac keeps its authored verification/fix
commit separate from the synchronized generated/imported outputs.
Decoded generated FullControlTest resource bytes contain no temporary probe
markers or machine-specific home/temporary paths. All owned test applications
and debuggers were stopped; ports 8888, 8889, 8890 and 8891 had no listeners.

## Coverage limits

Cross-machine and scoped link-local IPv6 communication were unavailable as
reported above. No macOS, GacJS, Windows UIA/transport, physical touch or desktop
global-hot-key consent result is claimed. All reported native UI interactions
use the application's automation service in a real Wayland session.

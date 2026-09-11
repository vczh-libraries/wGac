# Test Matrix Card 2026-09-10 15:49:34 PDT

## Test Matrix

| Test Item | 1st |
| --- | --- |
| [Linux][Test_CppTest_Rvm][`/MiniHttp`] | 2026-09-11T00:04:49.504Z |
| [Linux][`/RPT`][`/MiniHttp`] | 2026-09-10T23:58:05.681Z (fixed) |
| [Linux][`/FCT`][`/MiniHttp`] | 2026-09-11T00:02:18.064Z (fixed) |
| [Linux][`/RVMT`][`/MiniHttp`] | 2026-09-11T00:05:56.347Z |
| [Linux][`/RVMT`][`/MiniHttp /Cli:<path>`] | 2026-09-11T00:06:37.724Z |
| [Linux][Hello World][local] | 2026-09-10T23:18:40.432Z |
| [Linux][FullControlTest][local standard] | 2026-09-10T23:36:12.941Z (fixed) |
| [Linux][FullControlTest][local hosted] | 2026-09-10T23:36:46.902Z (fixed) |
| [Linux][WGacAsyncService][unit] | 2026-09-10T23:39:26.780430+00:00 |

## Issues Found and Fix

### Core launcher contract

`test_core.sh` invoked incremental builds despite the job requiring full builds. Added the required `-f`; a full Core build and launch completed successfully.

### Standalone FullControlTest palette synchronization

The upstream showcase raises `PaletteSelected`, but wGac had neither mirrored nor attached its shared handler. The sync script now copies the handler and the local entry point attaches it; upstream includes support both development and imported builds. All six palette choices produced their expected accent colors in standard mode; hosted mode also passed every palette, editor retention, both shortcuts, full mouse payload matrix, and queued normal Close. Hosted active-frame blending differs by one RGB channel from the primary accent.

### Standard-mode second shortcut dialog

After closing Ctrl+Q, Ctrl+Alt+Super+Q opens the shared Win-spelled confirmation, but OK and Enter do not dismiss it. Reproduced in fresh processes and traced to the shared automation mouse-position cache. Same coordinates skipped movement delivery to a new window. GacUI now refreshes hit testing before each mouse-button event; a new regression failed before the fix and passed afterward. Both shortcut dialogs and mouse payload checks passed after rebuilding. Then 25 consecutive pairs of both shortcut dialogs and normal queued Close passed with the task-pump fix. Manual duplicate key-release probes subsequently triggered the expected invalid-input assertion; those probes were a harness error.

### Wayland nested task pumping

A modal opened by one queued callback stranded remaining callbacks in the same local batch. This could time out Controls reads. Pending tasks now remain available to nested pumping and Stop until each task starts. A deterministic native-service regression reproduced the failure before the fix; both service regressions passed after the fix, as did 25 consecutive standard-mode dialog pairs.

### Native RPT renderer replacement

Core terminated with SIGSEGV on renderer replacement after the document dialog. LLDB identified a retained label whose layout render target was detached while its remote cache owner remained valid. Font-height lookups now use that owner. A focused replay regression crashed before the fix and passed afterward (6/6 cases); rebuilt native runtime verification passed, including 25 consecutive document-dialog/replacement/retained-state/shortcut cycles. Live takeover, normal File-menu confirmation and Core-authored fatal retention also passed.

### Shared Core modal task pumping

The same batch-draining problem also exists in GacUI SharedAsyncService. A new regression failed when a nested pump could not consume the next queued callback. Pending work now remains visible, with a sequence counter preventing duplicate execution and preserving ordinary batch boundaries. The focused 15-case automation suite passes after the fix; the full suite passes 89/89 files and 1767/1767 cases.

### Remote FullControlTest coverage

Both list panes showed exact 0–9 records and cleared, then 10–19 and cleared. Independent Search and document markers, including `Hello[Ab]{Cd}`, survived tabs and renderer replacement. Both shortcut dialogs and the five-button/modifier/movement/double-click/two-axis wheel automation matrix passed before and after replacement. Queued Close ended Core and renderer with exit 0. Linux global OS shortcuts remain unsupported.

### Native view-model combinations

Standalone CppTest_Rvm, remote Core with a manual native host, and remote Core with an auto-launched stdio host all passed exact greeting updates and normal shutdown. Network cases rejected a second host while retaining the original service. Remote cases retained state after renderer replacement. Each combination passed fresh idle-next-call and delivery-acknowledgement-loss injections; stopped network hosts had unread response bytes before termination, and the stdio requester was blocked in its RPC wait. Standalone failures were unhandled `RpcInjectedException` aborts. Both Core modes retained exact `RemotingTest_RvmHost disconnected.` and accepted only `!Exit`; the normally closed stdio child was reaped.

### Final generated-resource follow-up

After the file-dialog resource regeneration and hosted click fix, native hosted FullControlTest repeated both shortcuts, the complete five-button/modifier/wheel input matrix, and queued normal Close. Native RPT repeated its document modal, renderer replacement, retained Home state, both shortcuts and confirmed normal exit; Core and renderer exited 0. The final event-count refinement passes 54 TUI/list cases, 25 additional native terminal pairs, and the rebuilt hosted FullControlTest shortcut/full mouse matrix/normal-close follow-up.

# Test Matrix Card 2026-09-21T19:07:36-07:00

## Test Matrix

| Test Item | 1st |
| --- | --- |
| [Linux][Test_CppTest_Rvm][`/MiniHttp`] | 2026-09-21T20:03:30-07:00 |
| [Linux][`/RPT`][`/MiniHttp`] | 2026-09-21T20:15:26-07:00 |
| [Linux][`/FCT`][`/MiniHttp`] | 2026-09-21T20:12:02-07:00 (fixed) |
| [Linux][`/RVMT`][`/MiniHttp`] | 2026-09-21T20:04:38-07:00 |
| [Linux][`/RVMT`][`/MiniHttp /Cli:<path>`] | 2026-09-21T20:05:54-07:00 |
| [Linux][Import / sync / build] | 2026-09-21T19:55:54-07:00 |
| [Linux][Hello World][local] | 2026-09-21T19:27:45-07:00 |
| [Linux][FullControlTest][local standard] | 2026-09-21T19:58:44-07:00 (fixed) |
| [Linux][FullControlTest][local hosted] | 2026-09-21T20:00:31-07:00 (fixed) |
| [Linux][WGacAsyncService][unit] | 2026-09-21T19:21:13-07:00 |

## Issues Found and Fix

### Easy Layout selected choice is clipped

Local standard FullControlTest exposes a 16-pixel-wide combo and a zero-width selected-text label. The dropdown remains operable, but its selected value is invisible. Initial evidence: `/tmp/rpxplat-easy-initial.json`. A 120-pixel minimum now preserves the selected text. Standard mode passed both arrangement directions, deferred/repeated rebuilds, six palettes, baseline editors/lists, local shortcuts and all five generated mouse buttons/modifiers.

### Hidden native popup remains registered

Repeated Easy Layout combo selection hides its Wayland surface, but the popup remains in the automation control tree. LLDB shows `GuiComboBoxListControl::OnListControlItemMouseDown` reaching `WGacNativeWindow::Hide(false)`; that path lacks the `Closed()` notification that removes it from GacUI’s opening-popup list. The visibility transition now delivers the notification once, including the accepted-close path. Ten consecutive open/select/dismiss cycles now remove the popup immediately. The full local baseline and queued normal close passed with exit 0.

## Verification details

Local standard and hosted FullControlTest passed both list panes, independent Search/document markers, Easy Layout mixed-row geometry, stored arrangement and repeated rebuilds, all palettes, retained choices/editor bindings, shortcuts, generated five-button/modifier input and queued close (exit 0). Standard mode additionally passed ten popup open/select/dismiss cycles after the visibility-notification fix. Remote FCT passed the same baseline, abrupt replacement/live takeover, retained editor/choice state, repeated shortcuts/mouse input and normal Core/renderer exit.

Standalone RVM passed normal RPC, rejected a second host, and terminated nonzero with an explicit unhandled `RpcInjectedException` in both fresh host-loss variants. Native remote network/stdio RVM passed replacement/takeover and normal closure, then both exact Core-authored fatal variants with ordinary IO rejected and `!Exit` accepted. Blocked network runs recorded unread host socket bytes; blocked stdio recorded a pending requester Controls read before killing the actual child. Logs: `/tmp/rpxplat-native-rvm.log`, `/tmp/rpxplat-native-rvm-2.log`, `/tmp/rpxplat-native-rvm-3.log`, `/tmp/rpxplat-native-ui.log`. Linux global shortcuts remain unavailable; automation inputs do not establish physical hardware delivery.

Native RPT additionally passed all three populated grid rows and clear, the scrolled inline document modal, replacement/takeover, repeated input checks, confirmed File-menu closure, and a fresh exact `This is a fatel error!` retained fatal state. Log: `/tmp/rpxplat-native-rpt.log`.

After upstream `ae8b8504d`, CodePack and the official import/sync/full-build sequence passed. Fresh standard and hosted FCT runs passed the 120-pixel choice geometry, both arrangements, all palettes, selected-text retention, the corrected `eighteen` label and normal close (exit 0). Standard mode repeated ten popup open/select/dismiss cycles with no retained popup. The complete remote matrix above preceded this final merge; remote protocol code is unchanged and the new editor implementation is guarded for UTF-16.

Final integration: GacUI `9c4491b35` includes the rebased fix and regenerated resources for both architectures. Resource compilation passed; a final CodePack run preserved every release-file hash used by the successful downstream build. All owned UI test processes and servers were closed.

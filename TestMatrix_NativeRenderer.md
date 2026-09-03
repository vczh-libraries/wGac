# Test Matrix Card 2026-09-02 23:38:27 PDT

## Test Matrix

| Test Item | 1st |
| --- | --- |
| [Linux][Test_CppTest_Rvm][`/MiniHttp`] | 2026-09-03 00:01:33 PDT |
| [Linux][`/RPT`][`/MiniHttp`] | 2026-09-03 00:08:44 PDT (fixed) |
| [Linux][`/FCT`][`/MiniHttp`] | 2026-09-03 00:21:12 PDT |
| [Linux][`/RVMT`][`/MiniHttp`] | 2026-09-03 00:28:04 PDT |
| [Linux][`/RVMT`][`/MiniHttp /Cli:<path>`] | 2026-09-03 00:37:20 PDT |

## Additional Verification

| Test Item | 1st |
| --- | --- |
| [Linux][Test_FullControlTest][Native, non-remoting] | 2026-09-02 23:49:50 PDT |
| [Linux][Test_FullControlTest][Native hosted, non-remoting] | 2026-09-03 00:06:00 PDT |

## Issues Found and Fix

### [Linux][`/RPT`][`/MiniHttp`] 1st

- The first automation attempt sent popup/menu mouse down and up without allowing a remote round trip between them, then retried against the still-active modal and tripped the IO-state invariant. Re-running popup activation as explicit `!LeftDown` and `!LeftUp` commands with state verification between them completed the full workflow, renderer replacement/takeover, Mouse4/Mouse5 checks, clean close, and retained fatal-state check. No source change was required for this test-driver issue.

# Test Matrix Card 2026-09-05 Linux TUI port

## Test Matrix

| Test Item | 1st |
| --- | --- |
| [Linux][`/RPT`][`/MiniHttp`] | 2026-09-05 00:53 PDT |
| [Linux][`/FCT`][`/MiniHttp`] | 2026-09-05 00:57 PDT |
| [Linux][`/RVMT`][`/MiniHttp`][Native `RemotingTest_RvmHost` over network] | 2026-09-05 01:01 PDT |
| [Linux][`/RVMT`][`/MiniHttp`][Native `RemotingTest_RvmHost` over stdio `/Cli:<path>`] | 2026-09-05 01:05 PDT |
| [Linux][`/RVMT`][`/MiniHttp`][GacJS browser host `?rvmhost`] | 2026-09-05 01:07:31 PDT |
| [Linux][`/RVMT`][`/MiniHttp`][GacJS Node `cli.js` over network] | 2026-09-05 01:08:54 PDT |
| [Linux][`/RVMT`][`/MiniHttp`][GacJS Node SEA over stdio `/Cli:<path>`] | 2026-09-05 01:10:57 PDT |

## Issues Found and Fix

No product failures were found in the Linux GacJS matrix. A polling request may
fail after an intentional, successful application shutdown; no unexpected
JavaScript exception or error dialog occurred during normal workflows.

## Coverage

All seven rows passed in Firefox. RPT/FCT covered their feature workflows,
shortcuts, bracket/brace typing, independent Alt/Super mouse modifiers, renderer
replacement/takeover, retained state, and normal shutdown. The three primary
buttons used browser mouse input; all five buttons/double clicks and all four
wheel directions were additionally exercised with DOM events and checked in
outgoing protocol messages and application readouts.

Each of the five RVM host modes passed normal ownership/shutdown and both
idle-host-loss and delivery-acknowledgement-loss checks, including exact
Core-authored fatal messages, retained rendering, and stdio-child cleanup.
The SOP's Windows global hotkey check is not applicable on Linux.

GacJS dependency import, code generation, build, package tests, and a second
code-generation pass succeeded; the second pass produced no semantic changes.

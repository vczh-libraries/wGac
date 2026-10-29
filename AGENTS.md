# General Instructions

At the end of every request, run the following scripts in order:
- `./import.sh`
- `./syncProj.sh`
- `./build.sh`
and they must all succeed.

When missing any dependencies to libraries, cmake and build scripts must be updated properly, and treat `./build.sh` as the only official verification.

All build scripts and makefiles, including CMake files, must remain usable on a fresh computer with a fresh clone after installing the documented prerequisites. They must not depend on temporary folders, machine-specific paths, or artifacts left over from previous builds. Any required generated inputs must be reproducible through the documented build process.

## Before You Start

Read these files before changing this repository:

1. [README.md](README.md) for the repository structure, scripts, build, and test commands.
2. [../GacUI/.github/KnowledgeBase/Index.md](../GacUI/.github/KnowledgeBase/Index.md) for the GacUI framework knowledge base. Consult the relevant articles before changing a GacUI platform interface. In particular:
   - `KB_GacUI_Design_PlatformInitialization.md` for entry points, renderer setup, and service registration.
   - `KB_GacUI_Design_MainWindowModalWindow.md` for modal windows and hosted mode.
   - `manual/gacui/coding-agent/automation-service.md` for automation services and IO commands.
3. [../GacUI/.github/Jobs/DebugRemoteProtocolWithNativeRenderer.md](../GacUI/.github/Jobs/DebugRemoteProtocolWithNativeRenderer.md) before testing the native remote renderer.

`Import/GacUI.h` defines the platform abstraction interfaces. When Wayland behavior is unclear, compare the current Windows implementation in `Import/GacUI.Windows.cpp` and the Cocoa implementation in sibling `../iGac`.

## Key Facts

- This is the Wayland implementation of GacUI.
- `Import/` is assembled from sibling `../GacUI/Import/` and `../GacUI/Release/` by `./import.sh`.
- `Import-Test/` contains the test-only `Test.RemotingHelpers` amalgamation assembled by `./import.sh`.
- `<wGac repo>/syncProj.sh` incrementally builds Workflow's `CppMerge` and GacUI's `GacGen` and `GacBuild`, copies the four upstream resource trees, and calls `<wGac repo>/GacBuild.sh -mode:GacGen` to merge x32/x64 C++ into `<wGac repo>/Apps/*/Source/`.
- Platform code is under `WGac/`; shared test support is under `WGacShared/`.
- On Ubuntu, run `sudo ./build-prerequisites-ubuntu.sh` to install the system build dependencies. The prerequisite script requires root but never invokes `sudo` itself, and `./build.sh` never downloads or installs dependencies.
- If `./build.sh` fails because Ubuntu packages are missing, do not add dependency-download logic to `build.sh`. Ask the user to run `sudo ./build-prerequisites-ubuntu.sh`; if system installation is unavailable, use a disposable container with the documented dependencies installed.
- Build with `./build.sh` or clean-build with `./build.sh --rebuild`.
- Test native apps with `./test.sh --app:simple`, `./test.sh --app:fct`, `./test.sh --app:fct --hosted`, `./test.sh --app:rvmt`, or `./test.sh --app:renderer`; use `./test_core.sh --app:fct|rpt|rvmt --protocol:minihttp` for the sibling GacUI Core-side launcher.
- The simple, Full Control Test, and Remote View Model Test apps expose GacUI automation through MiniHTTP on port 8888.
- The native remote renderer connects to `GacUI/Test/Linux/RemotingTest_Core` through `/MiniHttp`.
- Renderer automation uses port 8889 by default; use `--port:<port>` for a concurrent takeover renderer.

## Generated and Imported Files

- Never edit `Import/` directly. Fix framework code upstream in GacUI and rerun `./import.sh`, or fix Wayland compatibility in `WGac/` and CMake.
- Never edit `Import-Test/` directly. Fix remoting helpers upstream in GacUI and rerun `./import.sh`.
- Never edit `Apps/*/Resources/` or `Apps/*/Source/` directly. Change the owning GacUI resource and rerun `./syncProj.sh`.
- Generated reflection sources are retained but excluded from test targets, which compile with `VCZH_DEBUG_NO_REFLECTION`.
- Do not edit build output under `build/`.

`<wGac repo>/GacBuild.sh` owns upstream executable discovery, explicit absolute GacGen and CppMerge paths, and the temporary full-metadata entry point. It defaults to GacBuild driver-XML mode; put `-mode:GacGen` first for one resource. Preserve the caller's working directory, argument boundaries, native exit status and cleanup on failure. Keep the temporary GacGen symlink unresolved: its adjacent `Metadata.txt` selects full `Reflection32.bin` and `Reflection64.bin` from `<GacUI repo>/Test/Resources/Metadata/`, including generated dialog types absent from the normal core-only metadata.

GacBuild runs `/P32` and `/P64`, validates both staging trees, and merges matching files with CppMerge while preserving upstream seed `USER_CONTENT`, embedded resources and `RemoteViewModelTestRpc.h/.cpp`. Keep generated reflection files and the existing target exclusions. Retain `<wGac repo>/Apps/*/Resources/GacBuild.log` and `<wGac repo>/Apps/*/Resources/Resource.xml.log/{x32,x64}/` for diagnostics, including compiler logs, staged C++/RPC files and `Deploy.xml`. These artifacts are ignored by Git. A compiler, artifact, merge or deployment failure must stop synchronization with nonzero status.

The files under `WGac/Protocol/` are committed `wayland-scanner` outputs. Normal builds consume them without regenerating them. With `wayland-scanner`, `pkg-config`, and `wayland-protocols` installed, run these commands from the repository root to refresh all three client protocols:

```bash
WGAC_PROTOCOLS_DIR="$(pkg-config --variable=pkgdatadir wayland-protocols)"

wayland-scanner client-header \
    "$WGAC_PROTOCOLS_DIR/stable/xdg-shell/xdg-shell.xml" \
    WGac/Protocol/xdg-shell-client-protocol.h
wayland-scanner private-code \
    "$WGAC_PROTOCOLS_DIR/stable/xdg-shell/xdg-shell.xml" \
    WGac/Protocol/xdg-shell-protocol.c

wayland-scanner client-header \
    "$WGAC_PROTOCOLS_DIR/unstable/xdg-decoration/xdg-decoration-unstable-v1.xml" \
    WGac/Protocol/xdg-decoration-unstable-v1-client-protocol.h
wayland-scanner private-code \
    "$WGAC_PROTOCOLS_DIR/unstable/xdg-decoration/xdg-decoration-unstable-v1.xml" \
    WGac/Protocol/xdg-decoration-protocol.c

wayland-scanner client-header \
    "$WGAC_PROTOCOLS_DIR/unstable/text-input/text-input-unstable-v3.xml" \
    WGac/Protocol/text-input-unstable-v3-client-protocol.h
wayland-scanner private-code \
    "$WGAC_PROTOCOLS_DIR/unstable/text-input/text-input-unstable-v3.xml" \
    WGac/Protocol/text-input-unstable-v3-protocol.c
```

Review the complete generated diff before committing because both the scanner and the installed protocol XML versions affect the output.

## Wayland Platform Rules

- Do not use `GetCurrentController()` in `WGac/` OS-provider code. Hosted mode changes the current application-level controller. Use `GetWGacController()` for native Wayland objects and services.
- Preserve the standard, hosted, and raw/native-renderer setup paths when changing renderer or service initialization.
- Native windows used by automation must be validated against the controller's currently created-window list before an integer ID is dereferenced.
- Keep platform changes limited to the files required by the task. The root `wgac.c` demo and editor configuration are legacy inputs and are not normal update targets.

## Testing Rules

- Exercise GUI behavior through the MiniHTTP automation service, not by assuming that a successful launch is sufficient.
- Verify both standard and hosted Full Control Test modes.
- Verify `--app:rvmt` with `GacUI/Test/Linux/RemotingTest_RvmHost /MiniHttp`.
- When testing the native remote renderer, run both `/RPT` and `/FCT` workflows and verify renderer replacement, takeover, and clean shutdown.
- For native Wayland dialogs, use the Linux guidance in `../Tools/Copilot/Guidelines/Running-ComputerUse.md`.
- Always stop test apps, remote cores, renderers, and debugging helpers when testing is complete.
- Use `--unblock` to launch a test executable in the background and print its PID.

## Documentation

Update `README.md` and its Chinese translation `README_CN.md` when files, targets, scripts, prerequisites, or commands change. Keep the two documents mutually linked and semantically equivalent.

Linux-specific verification process belongs in:

- `../GacUI/.github/Jobs/job.rpXPlat.prompt.md`
- `../GacUI/.github/Jobs/job.rpWasm.prompt.md`

Linux computer-use guidance belongs in:

- `../Tools/Copilot/Guidelines/Running-ComputerUse.md`

## Remote feature injection

Native renderer executables accept `/remoteDT`, `/remoteLocale` and `/remoteFS`.
All three demo options default to false. The corresponding `test.sh --app:renderer`
and `test_core.sh --app:fct --protocol:minihttp` shell options are `-remoteDT`,
`-remoteLocale` and `-remoteFS`; scripts translate them to slash switches in both
foreground and `--unblock` modes. Enable each desired feature independently on
both core and renderer. Keep the canonical renderer GuiMain and SharedArguments
synchronized from GacUI. Reconnection reevaluates capabilities; DateTime/Locale
fall back to the predecessor, while an unavailable injected filesystem is empty.

FullControlTest's authored shared helper is `FullControlTestEventHandlers.h/.cpp`.
Synchronize it with the XML/generated event definitions. Native entry points call
`demo::AttachFctEvents<demo::DocumentEditorBase>` once per main window, attaching
four document events per editor plus the palette event to their correct owners.

## Choosing the remote Core host

`./test.sh --app:renderer --corehost:core.example.com` selects the destination; omitting `--corehost` defaults to localhost. The option is renderer-only, case-sensitive, nonempty and allowed once. Both foreground and `--unblock` branches forward one quoted `/corehost:<host>` argument. The native entry point uses the canonical GacUI parser and accepts DNS hostnames, IPv4 and IPv6 addresses (optionally bracketed), without a URL, path or separate port. Invalid or duplicate native arguments are rejected before transports start.

```bash
./test.sh --app:renderer --corehost:core.example.com --port:8889
./test.sh --app:renderer --corehost:::1 --port:8890 --unblock
```

Native renderers here use `/MiniHttp`; `/Pipe` and `/Http` are Windows transports. Protocol TCP port is always 8888. `--port` changes only local renderer automation, default 8889, and can be combined with destination and injection options. The local Core/requester launchers need no destination option. A manually launched `RemotingTest_RvmHost /MiniHttp /corehost:core.example.com` connects to Core `/RVMT` or a requester server; its local `/Cli` mode rejects an explicit host.

Keep SharedArguments and renderer GuiMain synchronized from GacUI, and maintain the platform Main.cpp separately. Socket destinations are unbracketed IPv6; HTTP authorities add brackets. Native listeners use wildcard interfaces; network reachability still depends on the host environment. Same-machine hostname checks do not verify another computer.

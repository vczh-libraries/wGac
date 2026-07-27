# General Instructions

## Before You Start

Read these files before changing this repository:

1. [README.md](README.md) for the repository structure, scripts, build, and test commands.
2. [../GacUI/.github/KnowledgeBase/Index.md](../GacUI/.github/KnowledgeBase/Index.md) for the GacUI framework knowledge base. Consult the relevant articles before changing a GacUI platform interface. In particular:
   - `KB_GacUI_Design_PlatformInitialization.md` for entry points, renderer setup, and service registration.
   - `KB_GacUI_Design_MainWindowModalWindow.md` for modal windows and hosted mode.
   - `KB_GacUI_Manual_AutomationService.md` for automation services and IO commands.
3. [../GacUI/DebugRemoteProtocolWithNativeRenderer.md](../GacUI/DebugRemoteProtocolWithNativeRenderer.md) before testing the native remote renderer.

`Import/GacUI.h` defines the platform abstraction interfaces. When Wayland behavior is unclear, compare the current Windows implementation in `Import/GacUI.Windows.cpp` and the Cocoa implementation in sibling `../iGac`.

## Key Facts

- This is the Wayland implementation of GacUI.
- `Import/` is assembled from sibling `../GacUI/Import/` and `../GacUI/Release/` by `./import.sh`.
- `Apps/` is synchronized and generated from sibling GacUI resources by `./syncProj.sh`.
- Platform code is under `WGac/`; shared test support is under `WGacShared/`.
- Build with `./build.sh` or clean-build with `./build.sh --rebuild`.
- Test with `./test.sh --app:simple`, `./test.sh --app:fct`, `./test.sh --app:fct --hosted`, or `./test.sh --app:renderer`.
- The simple and Full Control Test apps expose GacUI automation through MiniHTTP on port 8888.
- The native remote renderer connects to `GacUI/Test/Linux/RemotingTest_Core` through `/MiniHttp`.

## Generated and Imported Files

- Never edit `Import/` directly. Fix framework code upstream in GacUI and rerun `./import.sh`, or fix Wayland compatibility in `WGac/` and CMake.
- Never edit `Apps/*/Resources/` or `Apps/*/Source/` directly. Change the owning GacUI resource and rerun `./syncProj.sh`.
- Generated reflection sources are retained but excluded from test targets, which compile with `VCZH_DEBUG_NO_REFLECTION`.
- Do not edit build output under `build/`.

## Wayland Platform Rules

- Do not use `GetCurrentController()` in `WGac/` OS-provider code. Hosted mode changes the current application-level controller. Use `GetWGacController()` for native Wayland objects and services.
- Preserve the standard, hosted, and raw/native-renderer setup paths when changing renderer or service initialization.
- Native windows used by automation must be validated against the controller's currently created-window list before an integer ID is dereferenced.
- Keep platform changes limited to the files required by the task. The root protocol fixtures and editor configuration are legacy inputs and are not normal update targets.

## Testing Rules

- Exercise GUI behavior through the MiniHTTP automation service, not by assuming that a successful launch is sufficient.
- Verify both standard and hosted Full Control Test modes.
- When testing the native remote renderer, run both `/RPT` and `/FCT` workflows and verify renderer replacement, takeover, and clean shutdown.
- For native Wayland dialogs, use the Linux guidance in `../Tools/Copilot/Guidelines/Running-ComputerUse.md`.
- Always stop test apps, remote cores, renderers, and debugging helpers when testing is complete.
- Use `--unblock` to launch a test executable in the background and print its PID.

## Documentation

Update `README.md` and its Chinese translation `README_CN.md` when files, targets, scripts, prerequisites, or commands change. Keep the two documents mutually linked and semantically equivalent.

Linux-specific remote-renderer workflow changes belong in:

- `../GacUI/DebugRemoteProtocolWithNativeRenderer.md`
- `../Tools/Jobs/job.verifyRemoteProtocol.prompt.md`

Linux computer-use guidance belongs in:

- `../Tools/Copilot/Guidelines/Running-ComputerUse.md`

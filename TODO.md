You are going to mirror the structure of `iGac` repo to this `wGac` repo.
`iGac` is in the same github organization, you can clone it in the parent folder of `wGac` for reference.
`iGac` implements GacUI on top of Cocoa, `wGac` implements GacUI on top of wayland, they are isomorphic, it is reasonable to have the same structure.
In order to mirror the structure, you have to perform the following list of changes:
- Rename `Source` to `WGac`.
- Delete `Release` submodule in `wGac`, and `Tests`.
- You are going to prepare the following scripts doing similar things in `iGac`. They are written for zsh and you are running on bash, please be careful:
  - `syncOrg.sh`, almost copy, but no need to test this script.
  - `import.sh`, copy code from `../GacUI/(Import|Release)` to `Import`.
  - `syncProj.sh`, copy `FullControlTest` and `RemoteProtocolTest` to `Apps` folder, and run `GacGen` to generate source code.
  - `build.sh`, build every test app, arguments should keep identical.
  - `test.sh`, launch a test app, arguments should keep identical.
- You are going to prepare test app folders:
  - `WGacShared`, shared code, just like `iGac/MacShared`.
  - `WGacTest`, hello world app, just like `iGac/MacTest`.
  - `WGacFullControlTest`, full control test app, supports with or without hosted mode, just like `iGac/MacFullControlTest`.
  - `RemotingTest_Renderer_Wayland`, remote protocol native renderer working with `../GacUI/Test/Linux/RemotingTest_Core`, just like `iGac/RemotingTest_Renderer_macOS`.
- Extra `WGac/Services/WGacAutomationService.(h|cpp)` must be created just like what `iGac/Mac/NativeWindow/CocoaAutomationService.(h|cpp)` do.
- `WGacShared/MiniHttpAutomationService.cpp` will be copied from `GacUI`, just like how `iGac.syncProj.sh` does.
- `WGacTest`, `WGacFullControlTest`, `RemotingTest_Renderer_Wayland` must use `vl::presentation::wayland::WGacAutomationService*` with `MiniHttpAutomationService`, just like how test apps in `iGac` does.
- You must rewrite `README.md` and `AGENTS.md` just like how `iGac` does.
  - `README_CN.md` will be a Chinese translation of `README.md`, and they must refer each other.
- You are going to run all test apps and see if you can operate with them using the automation service exposed from the mini HTTP server.
  - When running `WGacFullControlTest` without hosted mode, you will encounter wayland's native dialog, follow Windows and macOS examples in `../Tools/Copilot/Guidelines/Running-ComputerUse.md` to complete the Linux one, make sure it is useful.
- You are going to follow `../Tools/Jobs/job.verifyRemoteProtocol.prompt/md`'s native renderer part to see if `RemotingTest_Renderer_Wayland` is working properly.
  - If anything in this document and referenced documents in `GacUI` need to update, feel free to update, but only limit the scope to Linux specific part. If you find any instructions can be shared with different platforms, you can re-organize them.
- Commit and push all local changes in all affected repos.

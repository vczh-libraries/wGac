# wGac — GacUI 的 Wayland 移植

[English](README.md)

wGac 使用 Wayland、Cairo、Pango 和 XKBCommon，为 Linux Wayland 实现 GacUI 的原生平台层。

[![Ask DeepWiki](https://deepwiki.com/badge.svg)](https://deepwiki.com/vczh-libraries/wGac)

## 环境依赖

仓库中已提交 `Import/` 和 `Apps/` 快照，因此常规编译不需要其他源码仓库。在 Debian 或 Ubuntu 上安装：

```bash
sudo apt update
sudo apt install build-essential clang cmake pkg-config \
    libwayland-dev libxkbcommon-dev \
    libcairo2-dev libpango1.0-dev libfontconfig1-dev \
    libgdk-pixbuf-2.0-dev libglib2.0-dev liburing-dev \
    xdg-desktop-portal
```

还需要安装桌面环境对应的 Portal 后端，例如 GNOME 上的 `xdg-desktop-portal-gnome`。wGac 通过 FileChooser Portal 显示原生的打开和保存文件对话框。

请在 Wayland 桌面会话中运行应用，并确保 `WAYLAND_DISPLAY` 和 `XDG_RUNTIME_DIR` 可用。

维护仓库时，请将 GacUI、Workflow 和 Tools 仓库放在 wGac 的同级目录，或者运行 `./syncOrg.sh`。`import.sh` 读取 GacUI 框架快照；`syncProj.sh` 读取 GacUI 测试资源，并编译 Workflow 和 GacUI 中的代码生成器。同级的 Release 仓库不是编译或导入依赖。

## 项目结构

```text
wGac/
├── WGac/                              Wayland 平台实现
│   ├── Protocol/                      已提交的 Wayland 协议源码
│   ├── Renderers/                     基于 Cairo/Pango 的 GacUI 渲染器
│   ├── Services/                      原生平台服务和自动化服务
│   └── Wayland/                       显示、输入设备和缓冲区集成
├── WGacShared/                        GacUI、wGac 和共享测试库
├── WGacTest/                          Hello World 测试应用
├── WGacFullControlTest/               标准或 Hosted 模式的 Full Control Test
├── RemotingTest_Renderer_Wayland/     RemotingTest_Core 的原生渲染器
├── Apps/                              同步的资源和生成的 C++ 源码
├── Import/                            导入的 GacUI 合并源码
├── import.sh                          从同级 GacUI 刷新 Import
├── syncProj.sh                        刷新并生成 Apps 和共享源码
├── syncOrg.sh                         同步同组织的同级仓库
├── build.sh                           编译所有测试目标
└── test.sh                            启动一个测试目标
```

运行 `./import.sh` 后，`Import/` 是只读快照；框架修复应提交到 GacUI，Wayland 兼容修复应提交到 wGac。`Apps/*/Resources/` 和 `Apps/*/Source/` 中的文件由 `./syncProj.sh` 同步或生成，不能直接修改。

## 同步依赖

同步同组织的同级仓库：

```bash
./syncOrg.sh
```

刷新导入的框架快照：

```bash
./import.sh
```

该脚本使用 `../GacUI/Import/` 和 `../GacUI/Release/` 替换 `Import/`，加入 DarkSkin 的 Release 源码，并将快照设为只读。

刷新 Full Control Test 和 Remote Protocol Test：

```bash
./syncProj.sh
```

该脚本增量编译 Workflow 的 `CppMerge` 和 GacUI 的 `GacGen`，复制上游资源目录，在 `Apps/` 中重新生成 x64 C++ 源码，将可移植的 MiniHTTP 自动化服务复制到 `WGacShared/`，并将可移植的原生渲染器入口复制到 `RemotingTest_Renderer_Wayland/`。

## 编译

```bash
./build.sh
./build.sh --rebuild
```

第一个命令执行增量编译。`--rebuild` 会用 `git clean -xdf` 删除被忽略的编译输出并执行全量编译，因此使用前请先提交或暂存所有新增源码。

根 CMake 项目使用 C++23，并编译：

- `GacUI`：导入的 GacUI 框架。
- `WGac`：Wayland 平台层。
- `WGacShared`：共享的 MiniHTTP 自动化支持。
- `Test_HellWorld_Cpp`。
- `Test_FullControlTest`。
- `RemotingTest_Renderer_Wayland`。

## 运行和自动化

```bash
./test.sh --app:simple
./test.sh --app:simple --unblock
./test.sh --app:fct
./test.sh --app:fct --hosted
./test.sh --app:fct --hosted --unblock
./test.sh --app:renderer
./test.sh --app:renderer --unblock
```

`--hosted` 只能与 `--app:fct` 一起使用。`--unblock` 会在后台启动所选程序并输出 PID。

普通应用在 8888 端口提供 MiniHTTP 自动化服务：

- Hello World：`/Automation/Test_HellWorld_Cpp`
- Full Control Test：`/Automation/Test_FullControlTest`

例如：

```bash
curl http://localhost:8888/Automation/Test_HellWorld_Cpp/Controls
curl -H 'Content-Type: application/json; charset=utf8' \
    --data '!Exit' \
    http://localhost:8888/Automation/Test_HellWorld_Cpp/IO
```

使用 `GET .../Controls` 查看控件树，使用 `POST .../IO` 或 `POST .../IO/<windowId>` 发送 IO 命令。命令成功入队时会返回 `Queued`。

验证结束后，必须停止所有后台测试进程。

## 原生远程渲染器

以 `/MiniHttp` 模式编译并启动 `GacUI/Test/Linux/RemotingTest_Core`，应用参数选择 `/RPT` 或 `/FCT`，然后运行：

```bash
./test.sh --app:renderer
```

Core 监听 8888 端口。Wayland 渲染器通过 `/MiniHttp` 连接，并在 8889 端口通过以下路径提供 DOM 和渲染端 IO：

```text
/Automation/RemotingTest_Renderer_Wayland
```

完整的 RPT/FCT、替换、接管和清理流程请参阅 [GacUI 原生渲染器验证指南](../GacUI/DebugRemoteProtocolWithNativeRenderer.md)。

## 已知限制

- 打开和保存文件已实现原生 FileChooser Portal；原生颜色和字体对话框尚未实现。
- 消息框目前仍使用现有的后备行为。
- Wayland 不允许客户端全局定位普通顶层窗口；位置请求由合成器决定。

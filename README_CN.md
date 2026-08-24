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
    libdecor-0-dev libdecor-0-plugin-1-gtk \
    libcairo2-dev libpango1.0-dev libfontconfig1-dev \
    libgdk-pixbuf-2.0-dev libglib2.0-dev liburing-dev
```

保留的 `WGacDialogService` 实现依赖 GIO，但当前 Wayland 应用会选择 `FakeDialogService`，无需安装桌面 Portal 后端。

请在 Wayland 桌面会话中运行应用，并确保 `WAYLAND_DISPLAY` 和 `XDG_RUNTIME_DIR` 可用。

wGac 启动时需要 libdecor 和实际可用的运行时装饰插件；如果只能得到 libdecor 不绘制装饰的后备插件，wGac 会输出诊断并停止，而不会静默地继续运行。为了在所有合成器上安全地反复切换 GacUI 自定义边框和平台边框，即使服务端装饰可用，wGac 也会强制 libdecor 提供客户端绘制的平台边框。从 GacUI 的角度看，该 libdecor 边框属于原生平台边框，与 GacUI 的自定义窗口模板不同。

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
├── WGacCppTestRvm/                    Remote View Model Test 客户端
├── RemotingTest_Rendering_Wayland/    RemotingTest_Core 的原生渲染器
├── Apps/                              同步的资源和生成的 C++ 源码
├── Import/                            导入的 GacUI 合并源码
├── Import-Test/                       仅供测试使用的 GacUI 远程辅助合并源码
├── import.sh                          从同级 GacUI 刷新 Import
├── syncProj.sh                        刷新并生成 Apps 和共享源码
├── syncOrg.sh                         同步组织仓库（包括 wGac）
├── build.sh                           编译所有测试目标
├── test.sh                            启动一个原生测试目标
└── test_core.sh                       全量编译并启动同级 GacUI 的 Core 侧目标
```

运行 `./import.sh` 后，`Import/` 和 `Import-Test/` 都是只读快照；框架修复应提交到 GacUI，Wayland 兼容修复应提交到 wGac。`Import-Test/` 保存专用的中立 `Test.RemotingHelpers` 文件对；stdio 传输实现由 `Import/` 中对应的 `VlppOS.Linux.cpp` 提供。这些文件只供平台测试目标使用，不属于常规 GacUI 框架快照。`Apps/*/Resources/` 和 `Apps/*/Source/` 中的文件由 `./syncProj.sh` 同步或生成，不能直接修改。

## 同步依赖

同步组织仓库，包括当前 wGac 检出：

```bash
./syncOrg.sh
```

刷新导入的框架快照：

```bash
./import.sh
```

该脚本会重新创建 `Import/` 和 `Import-Test/`，从 `../GacUI/Import/` 和 `../GacUI/Release/` 复制常规框架文件，加入 DarkSkin 的 Release 源码，把中立 `Test.RemotingHelpers` 文件对移动到 `Import-Test/`，并将两个快照都设为只读。导入的 `VlppOS.Linux.cpp` 提供 stdio 传输实现。

刷新 Full Control Test、Remote Protocol Test 和 Remote View Model Test：

```bash
./syncProj.sh
```

该脚本增量编译 Workflow 的 `CppMerge` 和 GacUI 的 `GacGen`，复制三个上游资源目录，保留资源自带的种子 C++ 文件，并在 `Apps/` 中重新生成 x64 C++ 源码。它还会刷新共享的原生渲染器入口、RVM 入口和 RVM 初始化文件。MiniHTTP 自动化已经包含在导入的 GacUI 快照中，可复用的远程测试辅助代码来自 `Import-Test/`；二者都不再以本地 `WGacShared/Mini*.cpp` 副本维护。

## 编译

```bash
./build.sh
./build.sh --rebuild
```

第一个命令执行增量编译。`--rebuild` 会用 `git clean -xdf` 删除被忽略的编译输出并执行全量编译，因此使用前请先提交或暂存所有新增源码。

根 CMake 项目使用 C++23，并编译：

- `GacUI`：导入的 GacUI 框架。
- `WGac`：Wayland 平台层。
- `WGacShared`：供测试目标共享的已导入远程测试辅助代码。
- `Test_HellWorld_Cpp`。
- `Test_FullControlTest`。
- `Test_CppTest_Rvm`。
- `RemotingTest_Rendering_Wayland`。

## 运行和自动化

```bash
./test.sh --app:simple
./test.sh --app:simple --unblock
./test.sh --app:fct
./test.sh --app:fct --hosted
./test.sh --app:fct --hosted --unblock
./test.sh --app:rvmt
./test.sh --app:rvmt --unblock
./test.sh --app:renderer
./test.sh --app:renderer --port:8890
./test.sh --app:renderer --unblock
./test_core.sh --app:cpptest_rvm --protocol:minihttp --unblock
./test_core.sh --app:fct --protocol:minihttp
./test_core.sh --app:rpt --protocol:minihttp
./test_core.sh --app:rvmt --protocol:minihttp [--cli]
```

`--hosted` 只能与 `--app:fct` 一起使用。`--port:<1-65535>` 只能与 `--app:renderer` 一起使用，用于选择该渲染器的自动化监听端口；它不会改变连接 Core 8888 端口的 `/MiniHttp` 通道。渲染器自动化端口默认为 8889。`--unblock` 会在后台启动所选程序并输出 PID。

`test_core.sh` 沿用相同的 `--app:` 和 `--unblock` 参数命名，并要求指定
`--protocol:minihttp`（可移植平台唯一支持的传输）。它会在启动每个用到的
GacUI 项目前，使用 `-f` 调用 GacUI 的 `Test/Linux` 编译脚本。手动模式的
`cpptest_rvm` 和 `rvmt` 会先启动 requester/Core，等待一秒，再全量编译并
启动 `RemotingTest_RvmHost`；后台手动模式会输出两个 PID。`--cli` 仅支持
`--app:rvmt`，它会预先编译 host，并由 Core 通过 stdio 自动启动。
可移植的 `Test_CppTest_Rvm` 仍只支持手动 `/MiniHttp`。

普通应用在 8888 端口提供 MiniHTTP 自动化服务：

- Hello World：`/Automation/Test_HellWorld_Cpp`
- Full Control Test：`/Automation/Test_FullControlTest`
- Remote View Model Test：`/Automation/CppTest_Rvm`

例如：

```bash
curl http://localhost:8888/Automation/Test_HellWorld_Cpp/Controls
curl -H 'Content-Type: application/json; charset=utf8' \
    --data '!Exit' \
    http://localhost:8888/Automation/Test_HellWorld_Cpp/IO
```

使用 `GET .../Controls` 查看控件树，使用 `POST .../IO` 或 `POST .../IO/<windowId>` 发送 IO 命令。命令成功入队时会返回 `Queued`。

验证结束后，必须停止所有后台测试进程。

`--app:rvmt` 会等待对应的 Workflow RPC Host。先启动客户端，再运行：

```bash
../GacUI/Test/Linux/RemotingTest_RvmHost/Bin/RemotingTest_RvmHost /MiniHttp
```

## 原生远程渲染器

以 `/MiniHttp` 模式编译并启动 `GacUI/Test/Linux/RemotingTest_Core`，应用参数选择 `/RPT` 或 `/FCT`，然后运行：

`./test_core.sh --app:rpt --protocol:minihttp` 会全量编译并启动 `/RPT`
形式（`/FCT` 请改用 `fct`）。现有 `test.sh` 继续用于启动原生渲染器：

```bash
./test.sh --app:renderer
```

Core 监听 8888 端口。Wayland 渲染器通过 `/MiniHttp` 连接，默认在 8889 端口通过以下路径提供 DOM 和渲染端 IO：

```text
/Automation/RemotingTest_Rendering_Native
```

停止旧渲染器后，替换渲染器可以继续使用 8889 端口。测试实时接管时，保留 8889 上的现有渲染器，并用 `--port:8890` 启动新渲染器；随后在 8890 端口使用相同的自动化路径。

完整的 RPT/FCT、替换、接管和清理流程请参阅 [GacUI 原生渲染器验证指南](../GacUI/.github/Jobs/DebugRemoteProtocolWithNativeRenderer.md)。

## 已知限制

- 原生对话框：
  - 打开和保存文件已实现原生 FileChooser Portal。
  - 消息框尚未实现。
  - 颜色选择器尚未实现。
  - 字体选择器尚未实现。
  - 这些限制属于 `WGacDialogService`；Wayland 实现目前始终使用 GacUI 的 `FakeDialogService`，因此应用不会调用任何原生对话框。
- Wayland 不允许客户端全局定位普通顶层窗口；位置请求由合成器决定。
- libdecor 没有设置平台边框窗口图标的 API，因此不支持 `IconVisible`，其 getter 始终返回 `false`。
- libdecor 无法单独隐藏最大化控件。最大化操作入口由 `SizeBox`（边框的缩放能力）决定；`MaximizedBox` 会保留并返回请求值，但无法突破这一平台限制。

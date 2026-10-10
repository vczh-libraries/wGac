# wGac — GacUI 的 Wayland 移植

[English](README.md)

wGac 使用 Wayland、Cairo、Pango 和 XKBCommon，为 Linux Wayland 实现 GacUI 的原生平台层。

## 环境依赖

仓库中已提交 `Import/` 和 `Apps/` 快照，因此常规编译不依赖同级源码仓库。在 Ubuntu 上，使用以下命令一次性安装系统编译依赖：

```bash
sudo ./build-prerequisites-ubuntu.sh
```

该脚本要求 root 权限，但绝不会自行调用 `sudo`。它使用 `apt-get` 更新 apt 元数据，并安装编译器、CMake、`pkg-config`、所需的开发包以及 libdecor GTK 运行时插件。如果需要审查对系统的修改，请在运行前阅读脚本。在 Debian 或其他 Linux 发行版上，请使用该发行版的软件包管理器安装对应软件包。

保留的 `WGacDialogService` 实现依赖 GIO，但当前 Wayland 应用会选择 `FakeDialogService`。全局快捷键通过 GIO 使用桌面的 GlobalShortcuts Portal 后端；普通应用对话框不依赖 Portal。

`./build.sh` 只配置并编译项目，绝不会下载或安装依赖。如果 CMake 在 Ubuntu 上报告缺少编译器、`pkg-config` 或开发模块，请运行 `sudo ./build-prerequisites-ubuntu.sh`，然后重试 `./build.sh`。

请在 Wayland 桌面会话中运行应用，并确保 `WAYLAND_DISPLAY` 和 `XDG_RUNTIME_DIR` 可用。

wGac 启动时需要 libdecor 和实际可用的运行时装饰插件；如果只能得到 libdecor 不绘制装饰的后备插件，wGac 会输出诊断并停止，而不会静默地继续运行。为了在所有合成器上安全地反复切换 GacUI 自定义边框和平台边框，即使服务端装饰可用，wGac 也会强制 libdecor 提供客户端绘制的平台边框。从 GacUI 的角度看，该 libdecor 边框属于原生平台边框，与 GacUI 的自定义窗口模板不同。

维护仓库时，请将 GacUI、Workflow 和 Tools 仓库放在 wGac 的同级目录，或者运行 `./syncOrg.sh`。`import.sh` 读取 GacUI 框架快照；`syncProj.sh` 读取 GacUI 测试资源，并编译 Workflow 和 GacUI 中的代码生成器。同级的 Release 仓库不是编译或导入依赖。

## 支持的平台

- Windows 的实现可以在 [Release 仓库](https://github.com/vczh-libraries/Release) 找到
- Linux 的实现可以在 [wGac 仓库](https://github.com/vczh-libraries/wGac) 找到
- macOS 的实现可以在 [iGac 仓库](https://github.com/vczh-libraries/iGac) 找到
- HTML5 的实现可以在 [GacJS 仓库](https://github.com/vczh-libraries/GacJS) 找到

![](./Screenshots/FCT_Default.png)

![](./Screenshots/TUI_SkyBlue%20(default).png)

## 项目结构

```text
wGac/
├── WGac/                              Wayland 平台实现
│   ├── Protocol/                      已提交的 Wayland 协议源码
│   ├── Renderers/                     基于 Cairo/Pango 的 GacUI 渲染器
│   ├── Services/                      原生平台服务和自动化服务
│   └── Wayland/                       显示、输入设备和缓冲区集成
├── WGacShared/                        GacUI、wGac 和共享测试库
├── WGacTest/                          Hello World 应用和原生服务测试
├── WGacFullControlTest/               标准或 Hosted 模式的 Full Control Test
├── WGacTuiControlTest/                使用 TuiSkin 的终端控件展示
├── WGacCppTestRvm/                    Remote View Model Test 客户端
├── RemotingTest_Rendering_Wayland/    RemotingTest_Core 的原生渲染器
├── Apps/                              同步的资源和生成的 C++ 源码
├── Import/                            导入的 GacUI 合并源码
├── Import-Test/                       仅供测试使用的 GacUI 远程辅助合并源码
├── import.sh                          从同级 GacUI 刷新 Import
├── GacBuild.sh                        使用完整元数据运行上游生成器
├── syncProj.sh                        刷新并生成 Apps 和共享源码
├── syncOrg.sh                         同步组织仓库（包括 wGac）
├── build-prerequisites-ubuntu.sh      安装 Ubuntu 系统编译依赖
├── build.sh                           编译所有测试目标
├── test.sh                            启动一个原生测试目标
└── test_core.sh                       全量编译并启动同级 GacUI 的 Core 侧目标
```

不能直接修改 `Import/` 和 `Import-Test/`；框架修复应提交到 GacUI，Wayland 兼容修复应提交到 wGac。这是开发行为规范；生成的文件和目录仍保留所有者写入权限。`Import-Test/` 保存专用的中立 `Test.RemotingHelpers` 文件对；stdio 传输实现由 `Import/` 中对应的 `VlppOS.Linux.cpp` 提供。这些文件只供平台测试目标使用，不属于常规 GacUI 框架快照。`Apps/*/Resources/` 和 `Apps/*/Source/` 中的文件由 `./syncProj.sh` 同步或生成，不能直接修改。

## 同步依赖

同步组织仓库，包括当前 wGac 检出：

```bash
./syncOrg.sh
```

刷新导入的框架快照：

```bash
./import.sh
```

该脚本会重新创建 `Import/` 和 `Import-Test/`，从 `../GacUI/Import/` 和 `../GacUI/Release/` 复制常规框架文件，加入 DarkSkin 和 TuiSkin 的 Release 源码，把中立 `Test.RemotingHelpers` 文件对移动到 `Import-Test/`，并保持两个快照的所有者写入权限。导入的 `VlppOS.Linux.cpp` 提供 stdio 传输实现。

刷新终端控件展示、Full Control Test、Remote Protocol Test 和 Remote View Model Test：

```bash
./syncProj.sh
```

该脚本通过现有的原生编译辅助脚本，增量编译 `<Workflow repo>/Tools/CppMerge`、`<GacUI repo>/Tools/GacGen` 和 `<GacUI repo>/Tools/GacBuild`。它复制四个上游资源目录及其种子 C++ 文件，然后针对每个资源调用 `<wGac repo>/GacBuild.sh -mode:GacGen`。GacGen 通过 `/P32` 和 `/P64` 暂存两种架构的输出；GacBuild 验证两份输出，并调用 CppMerge 将对应文件合并到 `<wGac repo>/Apps/*/Source/`。合并保留上游种子的 `USER_CONTENT`、嵌入式资源和 `RemoteViewModelTestRpc.h/.cpp`。生成的反射文件继续保留，并继续从使用 `VCZH_DEBUG_NO_REFLECTION` 的编译目标中排除。

`GacBuild.sh` 在同级 GacUI 和 Workflow 仓库中定位三个可执行文件，并向 GacBuild 显式传入 GacGen 和 CppMerge 的绝对路径。GacGen 使用临时符号链接路径，不能将其解析为真实可执行文件路径，否则会改变元数据查找位置。链接旁的 `Metadata.txt` 选择 `<GacUI repo>/Test/Resources/Metadata/` 中的完整 `Reflection32.bin` 和 `Reflection64.bin`，其中包含开发工具通常使用的核心元数据所缺少的生成对话框类型。包装脚本在成功或失败后都会删除临时入口，并返回原生工具的退出状态。维护依赖同级 Workflow 和 GacUI 仓库，不需要 Release 仓库。

`syncProj.sh` 编译工具后，也可以直接使用包装脚本：

```bash
./GacBuild.sh -FileName path/to/GacUI.xml -Dump
./GacBuild.sh -mode:GacGen -FileName Apps/TuiControlTest/Resources/Resource.xml
```

默认的 GacBuild 模式接收用于增量资源发现的驱动 XML；`-Dump` 只报告编译计划。将 `-mode:GacGen` 放在第一个参数位置可重新编译单个资源，也可以通过 `-MappingFileName <mapping>` 指定依赖映射。即使从其他目录调用包装脚本，XML 和映射文件路径仍相对于调用者的工作目录解析。

可检查 `<wGac repo>/Apps/*/Resources/GacBuild.log` 和 `<wGac repo>/Apps/*/Resources/Resource.xml.log/{x32,x64}/`，查看编排及编译诊断、暂存的 C++/RPC 文件、二进制缓存和 `Deploy.xml` 部署清单。成功或失败后都保留已产生的暂存文件，并由 Git 忽略；下次同步会重新复制资源目录。生成失败、必要文件缺失、合并失败或部署失败都会以非零状态终止同步。

脚本还会从 `<GacUI repo>/Test/GacUISrc/CppTest_Tui/Main.cpp` 复制共享的 TUI GuiMain，并刷新共享的原生渲染器入口、RVM 入口、RVM 初始化文件和共享的 FullControlTest 配色处理函数。RVM 入口使用的共享自动化参数头文件会复制到根目录并提交为 `<wGac repo>/SharedArguments.h`，确保全新克隆无需同级源码或临时文件即可编译。独立展示程序连接该处理函数，使标准模式和托管模式中的配色切换都能刷新现有控件。MiniHTTP 自动化已经包含在导入的 GacUI 快照中，可复用的远程测试辅助代码来自 `<wGac repo>/Import-Test/`；二者都不再以本地 `<wGac repo>/WGacShared/Mini*.cpp` 副本维护。

## 编译

```bash
./build.sh
./build.sh --rebuild
```

第一个命令执行增量编译。`--rebuild` 只删除 `build/` 目录并执行全量编译。依赖检测仍由 CMake 负责；如果 Ubuntu 缺少所需的系统软件包，请运行一次 `sudo ./build-prerequisites-ubuntu.sh`，然后重试编译。

CMake 会丢弃缓存中已不存在的 X11 头文件目录和库文件路径（例如指向已删除的临时 sysroot 的路径），并重新搜索。TUI 剪贴板需要 `libx11-dev` 和 `libxfixes-dev`；如果依赖检测失败，请通过依赖安装脚本安装这些软件包。

根 CMake 项目使用 C++23，并编译：

- `GacUI`：导入的 GacUI 框架。
- `WGac`：Wayland 平台层。
- `WGacShared`：供测试目标共享的已导入远程测试辅助代码。
- `Test_HellWorld_Cpp`。
- `Test_FullControlTest`。
- `Test_CppTest_Rvm`。
- `WGacTui` 和 `Test_TuiControlTest`。
- `RemotingTest_Rendering_Wayland`。

编译后运行原生服务回归测试：

```bash
./build/WGacTest/bin/Test_AsyncService /C
./build/WGacTest/bin/Test_ImageService /C
./build/WGacTest/bin/Test_GlobalShortcuts /C
```

这些测试覆盖嵌套模态消息循环中的待执行任务，以及回调停止服务时对剩余任务的取消。
图像测试覆盖禁用状态的灰度渲染、全部 256 个 Alpha 值、透明像素合成、缓存复用，以及原始图像保持不变。禁用图像使用与 Windows 和 macOS 相同的提亮灰度转换，并保持 Alpha 不变。转换后的表面按需创建，保存在图像帧持有的 `INativeImageFrameCache` 中，由使用该帧的渲染器共享。
全局快捷键测试使用隔离的 D-Bus 会话（依赖安装脚本包含 `dbus-daemon`）和模拟 Portal，覆盖应用注册、提前到达的响应、修饰键编码、多快捷键、UI 线程触发、重复或无效注册、拒绝、取消和注销。测试不会授予桌面权限，也不能代替真实桌面验证。

## 运行和自动化

```bash
./test.sh --app:tui
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

## 输入映射

原生 Wayland 输入使用 VlppOS 的共享声明。鼠标移动、首次悬停、按钮、双击和两个滚轮轴均独立保留 Alt 与 Super 状态。
左右方括号及按住 Shift 输入的大括号分别映射为 `KEY_LEFT_BRACKET`（`0xDB`）和
`KEY_RIGHT_BRACKET`（`0xDD`）；按键名称查询使用 `[` 和 `]`。

## 已知限制

- 终端输入有独立的修饰键和快捷键限制；请参阅[终端输入限制](#终端输入限制)，了解 Kitty 的使用方式及仍然存在的限制。
- 原生对话框：
  - 打开和保存文件已实现原生 FileChooser Portal。
  - 消息框尚未实现。
  - 颜色选择器尚未实现。
  - 字体选择器尚未实现。
  - 这些限制属于 `WGacDialogService`；Wayland 实现目前始终使用 GacUI 的 `FakeDialogService`，因此应用不会调用任何原生对话框。
- Wayland 不允许客户端全局定位普通顶层窗口；位置请求由合成器决定。
- libdecor 没有设置平台边框窗口图标的 API，因此不支持 `IconVisible`，其 getter 始终返回 `false`。
- libdecor 无法单独隐藏最大化控件。最大化操作入口由 `SizeBox`（边框的缩放能力）决定；`MaximizedBox` 会保留并返回请求值，但无法突破这一平台限制。

## 全局快捷键

wGac 通过 [GlobalShortcuts Portal](https://flatpak.github.io/xdg-desktop-portal/docs/doc-org.freedesktop.portal.GlobalShortcuts.html) 注册全局快捷键。原生及 Hosted Full Control Test 使用 **Ctrl+Shift+Alt+Super+Q**；TUI 展示程序使用 **Ctrl+Alt+Super+Shift+F8**。原生远程渲染器会将 Portal 触发事件转发给 Core。其他应用获得焦点时，这些快捷键仍然可以生效。

GNOME 从 [GNOME 48](https://release.gnome.org/48/developers/#global-shortcuts) 开始支持该 Portal，包括 Ubuntu 26.04 LTS 的 GNOME 50。wGac 在运行时检测 Portal，不根据 Ubuntu 版本判断支持情况。Portal 缺失或按键不受支持时返回 `NotSupported`。用户授权及最终桌面绑定是异步的：返回的注册 ID 只表示已保留请求，授权后才能触发；桌面也可以更改请求的组合键。取消或拒绝授权会使该注册保持未激活状态。

`<wGac repo>/test.sh` 会在当前用户的 XDG 应用目录中创建隐藏的 desktop 文件，并将 `WGAC_APPLICATION_ID` 设置为 `org.gaclib.wGac.<app>`。通过 [Registry Portal](https://flatpak.github.io/xdg-desktop-portal/docs/doc-org.freedesktop.host.portal.Registry.html)，从终端启动的测试程序也能提供所需的应用身份。各测试应用分别授权。其他从终端启动的应用应安装自己的对应 desktop 文件并设置该变量；已有桌面或沙盒身份的应用可以不设置，由 Portal 自动识别。

首次注册时，GNOME 可能在 GacUI 外显示 **Add Keyboard Shortcuts**。请批准所需快捷键，然后在其他窗口获得焦点时测试。这个桌面授权对话框不属于 `FakeDialogService`，也不会出现在 MiniHTTP 控件树中。注销快捷键会取消待完成的 Portal 请求并关闭会话；进程退出会释放全部会话。GNOME 50 可能在请求取消后仍保留授权窗口，请使用 **Cancel** 关闭这个过期窗口。Portal 重启后，请重启应用以重新注册。

### Ubuntu 26.04 复查

在 Ubuntu 26.04.1 / GNOME Shell 50.1 上复查后，**新增可用功能为一项：全局快捷键**，由原生、Hosted、TUI 和远程渲染器共用。其他已记录限制仍然存在：

| 功能 | Ubuntu 26.04 结果 |
| --- | --- |
| GNOME Terminal 的本地 Super 快捷键 | VTE 0.84.0 仍丢弃 Super；请使用 Kitty 等兼容终端。 |
| SGR 鼠标 Super / 单按 Alt 显示提示 | 终端协议及所请求键盘模式的限制不变。 |
| 原生消息、颜色及字体对话框 | `WGacDialogService` 仍未实现；应用对话框继续使用 `FakeDialogService`。已有的 FileChooser Portal 不计为新功能。 |
| 普通 Wayland 窗口的全局定位 | 仍由合成器控制；较新的拖动协议不提供任意定位。 |
| 平台边框图标 / 独立最大化按钮 | libdecor 0.2.5 仍没有对应 API。 |
| TUI locale 及外部富文本剪贴板 | 属于现有 provider 限制，桌面升级不会实现这些路径。 |

此前列出的标题栏拖动缺陷已无法复现：通过合成器发送输入拖动原生远程渲染器时，鼠标相对窗口左上角的偏移保持不变。wGac 的 `96ec017` 提交已经实现合成器移动路径，因此这里只修正历史记录，不将其计为 Ubuntu 26.04 新增功能。构建及真实桌面测试结果请参阅[验证记录](TestMatrix_Tui.md#ubuntu-2604-review-2026-10-09)。

## 终端控件展示

`./test.sh --app:tui` 在当前终端中以前台方式运行 Hosted GacUI 控件展示，要求 stdin/stdout 连接交互式终端；拒绝 `--unblock`、`--hosted` 和 `--port`，不提供 HTTP 自动化接口。请从 120x40 单元开始，再测试 80x25，并遵循 [TUI SOP](../GacUI/.github/Jobs/DebugTuiControlTestSop.md)。当前结果记录在 [TestMatrix_Tui.md](TestMatrix_Tui.md)。

`WGac/TUI` 将 `TuiControllerBase` 与现有 wGac 字体/光标、按键名称和图像服务组合。计时器使用 VlppOS 所在线程的事件循环，所有窗口和对话框都绘制在终端单元中，不创建 Wayland surface 或 libdecor 窗口。字体度量固定为 `TuiFont`、字号 1，实际字体和颜色由终端决定。

TUI 剪贴板使用 X11 selection（编译依赖包括 `libx11-dev`、`libxfixes-dev`）。在 Wayland 桌面中，XWayland 负责与其他桌面应用互通，请保留 `DISPLAY`。没有 X display 时，剪贴板对象仅在进程内共享。外部传输为 UTF-8 文本，富文档和图像对象只在应用内保留。XFixes 所有权通知会更新依赖剪贴板状态的命令。其他客户端读取 selection 时，本应用必须仍在运行。支持接收增量 selection；发送文本的大小不能超过 X 服务器的最大请求长度。

正常 Hide、Close 和 Stop 会恢复终端输入输出模式；关闭终端标签页不能替代正常退出测试。

TUI provider 在启动工作线程前根据环境初始化 `LC_CTYPE`，以支持原生文件/图像服务中的 Unicode 文件名；请使用 UTF-8 locale。现有 POSIX locale 服务的日期和数字格式仍采用 en-US，展示程序的翻译标签和对话框则按应用选定语言切换。

### 终端输入限制

本地快捷键 **Ctrl+Alt+Super+Q** 要求终端将 Super 与 Alt 分别上报。GNOME Terminal 3.52.0 / VTE 0.76.0 会丢弃 Super，发送与 Ctrl+Alt+Q 相同的字节。Ubuntu 26.04.1 的 GNOME Terminal 3.58.0 / VTE 0.84.0 仍然如此：通过合成器生成输入时，两种组合键均产生 `1b 11`。限制发生在终端编码输入时：GNOME/Wayland 会将 Super 传递给获得焦点的终端，但 TUI 进程只能收到终端输出的字节。原生 Wayland 渲染器则直接接收属于其自身焦点窗口的修饰键状态。

请使用 [Kitty](https://sw.kovidgoyal.net/kitty/) 运行本地 Super 快捷键。可以将 Kitty 与 GNOME Terminal 同时安装，打开 Kitty 窗口后，在 `wGac` 目录运行现有应用：

```bash
./test.sh --app:tui
```

VlppOS 会自动启用 Kitty 键盘协议并解析 Super；无需修改应用代码、升级桌面或特别配置键盘协议。[Linux 验证记录](TestMatrix_Tui.md)已确认，在 GNOME Wayland/XWayland 下使用 Kitty 0.32.2 时，左右两个 Super 键均可触发 Ctrl+Alt+Super+Q。

桌面快捷键与剩余终端限制独立于 Kitty 的本地 Super 支持：

- **Ctrl+Alt+Super+Shift+F8** 是全局快捷键，没有本地按键回退路径。它独立于终端键盘编码使用桌面 Portal，因此授权后在 GNOME Terminal 中也能生效。桌面环境要求请参阅[全局快捷键](#全局快捷键)。
- 标准 SGR 鼠标报告没有 Super 位，因此鼠标 `osSuper` 保持为 false。旧终端的 Meta 仍映射为 Alt。
- 当前请求的键盘模式不单独上报修饰键，因此不能通过单按 Alt 显示访问键提示；仍可使用鼠标和方向键操作菜单。

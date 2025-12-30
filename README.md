# wGac Tests 编译指南

本目录包含 GacUI 在 Wayland 环境下的测试和示例项目。

## 系统要求

- Linux 系统（支持 Wayland）
- CMake >= 3.24
- GCC/Clang（支持 C++20）
- pkg-config

## 依赖库

在编译之前，请确保安装以下依赖：

**Debian/Ubuntu:**
```bash
sudo apt install cmake build-essential pkg-config \
    libwayland-dev libxkbcommon-dev \
    libcairo2-dev libpango1.0-dev libfontconfig1-dev \
    libgdk-pixbuf-2.0-dev libgio-2.0-dev
```

**Fedora:**
```bash
sudo dnf install cmake gcc-c++ pkg-config \
    wayland-devel libxkbcommon-devel \
    cairo-devel pango-devel fontconfig-devel \
    gdk-pixbuf2-devel glib2-devel
```

**Arch Linux:**
```bash
sudo pacman -S cmake base-devel pkg-config \
    wayland libxkbcommon \
    cairo pango fontconfig \
    gdk-pixbuf2 glib2
```

## 编译步骤

### 1. 初始化子模块

首先需要初始化 Release 子模块（包含 GacUI 源码和资源）：

```bash
cd /path/to/wGac
git submodule update --init --recursive
```

### 2. 创建构建目录

```bash
cd Tests
mkdir -p build
cd build
```

### 3. 运行 CMake 配置

```bash
cmake ..
```

如需调试版本：
```bash
cmake -DCMAKE_BUILD_TYPE=Debug ..
```

### 4. 编译

编译所有测试项目：
```bash
make -j$(nproc)
```

编译单个项目（例如 Cpp）：
```bash
make Cpp
```

## 项目列表

### GacUI_HelloWorlds（入门示例）
| 项目 | 说明 |
|------|------|
| Cpp | 纯 C++ 入门示例 |
| CppXml | C++ + XML 资源示例 |
| MVVM | MVVM 架构示例 |

### GacUI_Layout（布局示例）
| 项目 | 说明 |
|------|------|
| Alignment | 对齐布局 |
| Flow | 流式布局 |
| Responsive1/2 | 响应式布局 |
| RichTextEmbedding | 富文本嵌入 |
| SharedSize | 共享尺寸 |
| Stack | 堆栈布局 |
| Table | 表格布局 |
| TableSplitter | 表格分割器 |

### GacUI_Xml（XML 绑定示例）
| 项目 | 说明 |
|------|------|
| Binding_Bind | 数据绑定 |
| Binding_Eval | 表达式求值 |
| Binding_Format | 格式化绑定 |
| Binding_Uri | URI 绑定 |
| Binding_ViewModel | ViewModel 绑定 |
| Event_Cpp/Script/ViewModel | 事件处理示例 |
| Instance_Control/Window/MultipleWindows | 实例化示例 |
| Member_Field/Parameter/Property | 成员示例 |
| Misc_ImportFolder | 导入文件夹 |

### GacUI_Controls（控件示例）
| 项目 | 说明 |
|------|------|
| AddressBook | 地址簿 |
| Animation | 动画效果 |
| CalculatorAndStateMachine | 计算器与状态机 |
| ColorPicker | 颜色选择器 |
| ContainersAndButtons | 容器与按钮 |
| DataGrid | 数据表格 |
| DocumentEditorRibbon | 文档编辑器（Ribbon） |
| DocumentEditorToolstrip | 文档编辑器（工具栏） |
| GlobalHotKey | 全局热键 |
| ListControls | 列表控件 |
| Localization | 本地化 |
| MenuVisibility | 菜单可见性 |
| ProgressAndAsync | 进度条与异步 |
| QueryService | 查询服务 |
| TriplePhaseImageButton | 三态图片按钮 |

### GacUI_ControlTemplate（控件模板）
| 项目 | 说明 |
|------|------|
| WindowSkin | 窗口皮肤 |

## 运行测试

编译完成后，可执行文件位于 `build/<Category>/<ProjectName>/` 目录下：

```bash
# 运行 HelloWorld 示例
./GacUI_HelloWorlds/Cpp/Cpp

# 运行布局示例
./GacUI_Layout/Alignment/Alignment
```

**注意**: 运行需要 Wayland 桌面环境。如果使用 X11，可以通过 XWayland 运行，或设置：
```bash
export XDG_SESSION_TYPE=wayland
```

## 常见问题

### Q: 编译报错找不到头文件
确保已正确初始化 git 子模块：
```bash
git submodule update --init --recursive
```

### Q: 运行时报错 "cannot connect to display"
确保在 Wayland 会话中运行，或设置正确的 `WAYLAND_DISPLAY` 环境变量。

### Q: pkg-config 找不到某个库
检查是否安装了对应的开发包（-dev 或 -devel 后缀）。

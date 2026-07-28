# Minixer (zh-CN)

轻量级单通道独立音频混音台（Standalone Mixer），基于 JUCE 8 / C++17 / CMake，支持 Windows 与 Linux。

Minixer 常驻于系统后台，可将硬件麦克风输入经 VST3 / VST2 插件链（EQ、Compressor、Gate 等）实时处理后，输出至任意音频设备（Windows 上如 VB-Audio Virtual Cable），供 Discord、Zoom、OBS、游戏等第三方软件作为虚拟麦克风输入。

## 核心功能

- **独立输入 / 输出设备选择**：Windows 支持 WASAPI、DirectSound，Linux 支持 ALSA；可独立指定输入/输出设备、采样率与缓冲区大小。
- **VST3 / VST2 插件机架**：12 个插件插槽，支持 64-bit / 32-bit VST3 与 VST2 插件动态加载、旁通、删除。
- **插件崩溃隔离（进程级沙盒）**：每个插件实例运行在独立的 `PluginHost64` / `PluginHost32` 子进程中，单个插件崩溃自动旁通，不影响主程序。
- **实时电平监测**：输入 / 输出 Peak / RMS 电平表，dBFS 刻度，过载预警。
- **通道条控制**：Input Trim、Pan、Stereo、Output Fader，支持 dB 刻度微调。
- **预设系统**：保存 / 加载整套插件链状态与通道条参数。
- **插件扫描与黑名单**：后台扫描 VST3 / VST2 插件目录，崩溃/超时插件自动加入黑名单并跳过。
- **系统常驻**：最小化至系统托盘，支持开机自启（Windows 注册表 / Linux XDG Autostart）、全局快捷键、配置持久化。

## 技术栈

- JUCE 8（git submodule，pin 8.0.15）
- C++17
- CMake 3.22+
- Windows 10 / 11、Linux（X11；Wayland 经 XWayland）

## 构建步骤

### 1. 克隆（含 JUCE 子模块）

```bash
git clone --recursive https://github.com/ARPO35/Minixer.git
# 已克隆但未拉子模块时：
git submodule update --init --recursive
```

也可使用本机已有的 JUCE 8 而不拉取子模块，配置时加
`-DMINIXER_JUCE_PATH=/path/to/JUCE`。

### 2. 安装依赖（仅 Linux）

Debian / Ubuntu：

```bash
sudo apt-get install build-essential cmake ninja-build pkg-config \
  libasound2-dev libfreetype-dev libfontconfig1-dev \
  libx11-dev libxcomposite-dev libxcursor-dev libxext-dev libxinerama-dev \
  libxrandr-dev libxrender-dev libxi-dev
```

Fedora：

```bash
sudo dnf install cmake gcc-c++ ninja-build pkgconf-pkg-config \
  alsa-lib-devel freetype-devel fontconfig-devel libcurl-devel \
  libX11-devel libXcomposite-devel libXcursor-devel libXext-devel \
  libXinerama-devel libXrandr-devel libXrender-devel libXi-devel
```

Windows 只需 Visual Studio 2022（含 C++ 桌面开发负载）与 CMake。

### 3. 配置与编译

```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release .
cmake --build build
```

Windows（Visual Studio 生成器）：

```powershell
cmake -B build -G "Visual Studio 17 2022" .
cmake --build build --config Release
```

产物位于 `build/Minixer_artefacts/Release/`（主程序与 `PluginHost64`）。
32 位沙盒子进程（`PluginHost32`）见下文「32 位插件桥接」。

> `PluginHost64` 必须与主程序位于同一目录，否则插件沙盒无法正常工作；
> CMake 的 install 规则与 CI 产物已保证该布局。

### 4. 打包（可选）

```bash
cd build && cpack -G "TGZ;RPM"
```

产出 `minixer-<版本>-Linux.tar.gz` 与 `.rpm`（含 `Minixer`、`PluginHost64`、`PluginHost32`、`assets/`、桌面入口与图标）。

## 32 位插件桥接

- **Windows**：用 32 位配置整体构建一次即可获得 `PluginHost32.exe`：
  `cmake -B build32 -G "Visual Studio 17 2022" -A Win32 . && cmake --build build32 --config Release`
- **Linux**：经 i686 工具链单独构建 `PluginHost32`（需 gcc-multilib 与各依赖库的 32 位开发包）：
  `cmake -B build32 -DCMAKE_TOOLCHAIN_FILE=cmake/linux-i686-toolchain.cmake -DMINIXER_BUILD_32BIT=ON . && cmake --build build32`

## 持续集成

`.github/workflows/linux-build.yml`：push / PR 时在 ubuntu-22.04 上完成
x86_64 全量构建 + i686 `PluginHost32` 交叉构建，产出 tar.gz 与 rpm，
并做 `ldd` 与 xvfb 启动冒烟检查。

## 目录结构

```
Minixer/
├── Source/                 # 项目源码
│   ├── Components/         # UI 组件
│   ├── IPC/                # 进程间通信抽象与 Windows/POSIX 实现
│   ├── LookAndFeel/        # 自定义外观
│   ├── Plugin/             # 插件宿主、机架、桥接节点
│   ├── PluginHost/         # PluginHost 子进程源码
│   └── Settings/           # 应用设置与快捷键
├── JuceLibraryCode/        # JuceHeader.h（伞头，CMake 下手工维护）
├── deps/
│   ├── JUCE/               # JUCE 8.0.15（git submodule）
│   └── vst2-headers/       # VST2 宿主 ABI 净室头文件（BSD-3-Clause）
├── assets/                 # 图标资源
├── cmake/                  # 工具链文件
├── packaging/              # 桌面入口等打包资源
├── docs/                   # 设计文档
├── CMakeLists.txt          # 构建脚本
└── README.md
```

## 注意事项

- 本项目**不包含 ASIO SDK**，Windows 默认使用 WASAPI / DirectSound；Linux 使用 ALSA（PipeWire/PulseAudio 用户可经 pipewire-alsa 桥接）。如需 ASIO 支持请自行下载 Steinberg ASIO SDK 并配置。
- VST2 宿主通过 `deps/vst2-headers/` 的净室头文件（Xaymar/vst2sdk 的 BSD-3-Clause 衍生）启用；若你持有 Steinberg VST2 SDK 授权，直接替换该目录内容即可，构建配置不变。
- Linux 版 HID 外设快捷键暂不支持（键盘与 MIDI 快捷键正常）。
- Windows 版 VST 插件不能直接在 Linux 加载；可经 yabridge 桥接，桥接后的插件会出现在标准扫描路径（`~/.vst3/yabridge` 等）中。
- 运行时需要 `PluginHost64`（及可选的 `PluginHost32`）与主程序位于同一目录。

## 开源协议

GNU Affero General Public License v3.0 (AGPLv3)

详见项目根目录 [LICENSE](LICENSE) 文件。

---

# Minixer (English)

A lightweight single-channel standalone audio mixer, built with JUCE 8 / C++17 / CMake, for Windows and Linux.

Minixer runs in the background, taking hardware microphone input through a VST3 / VST2 plugin chain (EQ, Compressor, Gate, etc.) and routing the processed signal to any audio device (e.g. VB-Audio Virtual Cable on Windows). Third-party applications such as Discord, Zoom, OBS, and games can then use it as a virtual microphone input.

## Features

- **Independent input / output device selection**: WASAPI and DirectSound on Windows, ALSA on Linux; input and output devices, sample rate, and buffer size can be configured independently.
- **VST3 / VST2 plugin rack**: 12 plugin slots with dynamic load, bypass, and removal for both 64-bit and 32-bit VST3 and VST2 plugins.
- **Plugin crash isolation (process-level sandbox)**: each plugin instance runs in a separate `PluginHost64` or `PluginHost32` child process. Crashes are isolated and the slot is automatically bypassed without affecting the main mixer.
- **Real-time level metering**: input / output Peak / RMS meters with dBFS scale and clip warning.
- **Channel strip controls**: Input Trim, Pan, Stereo, and Output Fader with fine dB-scale adjustment.
- **Preset system**: save and load the entire plugin chain state together with channel strip parameters.
- **Plugin scanning and blacklist**: scan VST3 / VST2 plugin directories in the background; plugins that crash or time out are automatically added to a blacklist and skipped.
- **System resident**: minimize to the system tray, with startup launch (Windows registry / Linux XDG Autostart), global shortcuts, and persistent configuration.

## Tech Stack

- JUCE 8 (git submodule, pinned to 8.0.15)
- C++17
- CMake 3.22+
- Windows 10 / 11, Linux (X11; Wayland via XWayland)

## Build Instructions

### 1. Clone (with the JUCE submodule)

```bash
git clone --recursive https://github.com/ARPO35/Minixer.git
# If already cloned without submodules:
git submodule update --init --recursive
```

To use an existing local JUCE 8 checkout instead of the submodule, configure with
`-DMINIXER_JUCE_PATH=/path/to/JUCE`.

### 2. Install dependencies (Linux only)

Debian / Ubuntu:

```bash
sudo apt-get install build-essential cmake ninja-build pkg-config \
  libasound2-dev libfreetype-dev libfontconfig1-dev \
  libx11-dev libxcomposite-dev libxcursor-dev libxext-dev libxinerama-dev \
  libxrandr-dev libxrender-dev libxi-dev
```

Fedora:

```bash
sudo dnf install cmake gcc-c++ ninja-build pkgconf-pkg-config \
  alsa-lib-devel freetype-devel fontconfig-devel libcurl-devel \
  libX11-devel libXcomposite-devel libXcursor-devel libXext-devel \
  libXinerama-devel libXrandr-devel libXrender-devel libXi-devel
```

On Windows you only need Visual Studio 2022 (C++ desktop workload) and CMake.

### 3. Configure and build

```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release .
cmake --build build
```

Windows (Visual Studio generator):

```powershell
cmake -B build -G "Visual Studio 17 2022" .
cmake --build build --config Release
```

Binaries are placed under `build/Minixer_artefacts/Release/` (the app and `PluginHost64`).
For the 32-bit sandbox helper (`PluginHost32`), see "32-bit plugin bridging" below.

> `PluginHost64` must reside in the same directory as the main executable,
> otherwise the plugin sandbox will not work; the CMake install rules and CI
> artifacts already guarantee this layout.

### 4. Packaging (optional)

```bash
cd build && cpack -G "TGZ;RPM"
```

Produces `minixer-<version>-Linux.tar.gz` and `.rpm` (containing `Minixer`,
`PluginHost64`, `PluginHost32`, `assets/`, a desktop entry and the icon).

## 32-bit plugin bridging

- **Windows**: build once with a 32-bit configuration to get `PluginHost32.exe`:
  `cmake -B build32 -G "Visual Studio 17 2022" -A Win32 . && cmake --build build32 --config Release`
- **Linux**: build `PluginHost32` separately with the i686 toolchain (requires gcc-multilib and 32-bit development packages):
  `cmake -B build32 -DCMAKE_TOOLCHAIN_FILE=cmake/linux-i686-toolchain.cmake -DMINIXER_BUILD_32BIT=ON . && cmake --build build32`

## Continuous Integration

`.github/workflows/linux-build.yml`: on every push / PR, builds the full
x86_64 product plus the i686 `PluginHost32` on ubuntu-22.04, packages
tar.gz and rpm, and runs `ldd` and xvfb launch smoke checks.

## Directory Structure

```
Minixer/
├── Source/                 # Project source code
│   ├── Components/         # UI components
│   ├── IPC/                # IPC abstractions and Windows/POSIX implementations
│   ├── LookAndFeel/        # Custom look-and-feel
│   ├── Plugin/             # Plugin hosting, rack, and bridge nodes
│   ├── PluginHost/         # PluginHost child-process source
│   └── Settings/           # Application settings and shortcuts
├── JuceLibraryCode/        # JuceHeader.h (umbrella header, hand-maintained)
├── deps/
│   ├── JUCE/               # JUCE 8.0.15 (git submodule)
│   └── vst2-headers/       # Clean-room VST2 hosting ABI headers (BSD-3-Clause)
├── assets/                 # Icon resources
├── cmake/                  # Toolchain files
├── packaging/              # Desktop entry and packaging resources
├── docs/                   # Design documents
├── CMakeLists.txt          # Build script
└── README.md
```

## Notes

- This project **does not include the ASIO SDK**; WASAPI / DirectSound is used on Windows and ALSA on Linux (PipeWire/PulseAudio users can bridge via pipewire-alsa). Add the Steinberg ASIO SDK manually if ASIO support is required.
- VST2 hosting is enabled via the clean-room headers in `deps/vst2-headers/` (BSD-3-Clause, derived from Xaymar/vst2sdk). If you hold a Steinberg VST2 SDK license, simply replace that directory's contents; no build changes are needed.
- HID-device shortcuts are not supported on Linux yet (keyboard and MIDI shortcuts work).
- Windows VST plugins cannot be loaded directly on Linux; they can be bridged via yabridge, after which they appear in the standard scan paths (`~/.vst3/yabridge`, etc.).
- At runtime, `PluginHost64` (and optionally `PluginHost32`) must be in the same directory as the main executable.

## License

GNU Affero General Public License v3.0 (AGPLv3)

See the [LICENSE](LICENSE) file in the project root for the full text.

# Minixer — 系统常驻虚拟混音台开发计划

> **版本**：v0.1-draft  
> **日期**：2026-07-09  
> **技术栈**：JUCE 8 / C++17 / Windows (WASAPI)  
> **目标**：构建一个开源、系统常驻、支持完整 VST3 插件链的实时音频混音台，通过 VB-Audio Virtual Cable 实现系统级音频路由。

---

## 1. 项目概述

### 1.1 核心定位
Minixer 是一款**轻量级单通道独立音频混音台（Standalone Mixer）**，非 DAW，非宿主，只拥有一般DAW的混音台功能。它常驻于系统后台，将硬件麦克风输入经 VST3 插件链（EQ、Compressor、Gate 等）实时处理后，输出至任意音频驱动（如 VB-Audio Virtual Cable、DirectSound 等，输入输出设备皆可指定），供 Discord、Zoom、OBS、游戏等第三方软件作为虚拟麦克风输入。

### 1.2 目标用户
- 直播主播（需要实时人声处理）
- 播客/配音工作者
- 远程会议重度用户（需要降噪、EQ、压缩）
- 游戏玩家（实时变声、语音增强）

### 1.3 核心价值
- **系统常驻**：开机自启，最小化至系统托盘，零打扰
- **完整插件支持**：VST3 插件机架，与专业 DAW 一致的插件挂载能力（参考Fl Studio等专业DAW的混音台机架）
- **实时低延迟**：目标端到端延迟 < 20ms（@48kHz/512 samples）
- **插件崩溃隔离**：单插件崩溃不拖垮混音台（进程级沙盒，类似Live）
- **开源免费**：AGPLv3 协议，社区可扩展

---

## 2. 可行性分析

### 2.1 技术可行性

| 技术点 | 可行性 | 说明 |
|--------|--------|------|
| JUCE 异构 IO | ✅ | `AudioDeviceManager` 支持 WASAPI/DirectSound 下输入输出设备独立指定 |
| VST3 插件宿主 | ✅ | JUCE `PluginFormatManager` + `AudioProcessorGraph` 原生支持 |
| 虚拟音频路由 | ✅ | 任意音频驱动 提供标准 Windows 音频设备对 |
| 进程级插件隔离 | ✅ | Windows 共享内存 + 独立子进程方案成熟 |
| 系统常驻 | ✅ | JUCE 支持系统托盘 + 注册表开机自启 |

### 2.2 信号流（以 VB-Audio Virtual Cable 为例）

```
硬件麦克风 ──► [JUCE Minixer] ──► VB-CABLE Input
                                    │
                                    ▼ (系统内核)
                              VB-CABLE Output
                                    │
                                    ▼
                         Discord / Zoom / OBS / 游戏
```

### 2.3 关键限制与对策

| 限制 | 对策 |
|------|------|
| 输入单声道 / 输出立体声不匹配 | 在 Graph 输入节点后添加 Mono→Stereo up-mixing 处理器 |
| 采样率不一致（设备/插件） | 强制统一 48kHz，初始化时拒绝不支持的配置 |
| 插件 UI 跨进程嵌入 | 第一阶段使用独立浮动窗口；第二阶段通过 HWND 跨进程 SetParent 或屏幕捕获实现内嵌 |
| WASAPI 独占模式抢占设备 | 使用共享模式（Shared Mode），确保系统其他应用正常工作 |

---

## 3. 功能设计

### 3.1 音频引擎（Audio Engine）—— 基础路由已完成（插件机架、设备管理、信号路由、电平监测已完成）

| 模块 | 功能描述 |
|------|----------|
| 设备管理 | ✅ 独立选择输入/输出设备、采样率、缓冲区大小、ASIO/WASAPI/DirectSound 切换 |
| 信号路由 | ✅ 基于 `AudioProcessorGraph` 的音频节点 |
| 插件机架 | ✅ 12 个插槽，支持 VST3 插件动态加载、旁通、删除（预设保存/加载） |
| 电平监测 | ✅ 输入/输出 Peak/RMS 电平表，dBFS 刻度，过载预警（Clip Indicator） |
| 输入/输出增益 | ✅ Input Trim + Output Fader，支持 dB 刻度微调，已纳入预设序列化 |

### 3.2 混音台 UI（Mixer Interface）—— 核心功能已完成（路由可视化待实现）

| 模块 | 功能描述 |
|------|----------|
| 通道条（Channel Strip） | ✅ 输入增益、插件机架插槽、输出推子、声像（Pan） |
| 插件编辑器 | ✅ 点击插件槽位弹出 `AudioProcessorEditor` 独立浮动窗口 |
| 电平表 | ✅ 实时输入/输出电平显示 |
| 预设管理 | ✅ 保存/加载整个插件链状态（含各插件参数），支持快速切换预设 |

### 3.3 系统常驻（System Resident）—— 已完成

| 模块 | 功能描述 |
|------|----------|
| 系统托盘 | ✅ 最小化至 Windows System Tray，支持左键单击/双击唤出、右键菜单快速操作 |
| 开机自启 | ✅ 注册表启动项，支持"静默启动"（直接最小化到托盘） |
| 全局快捷键 | ✅ 如 `Ctrl+Shift+M` 呼出主窗口、`Ctrl+Shift+B` 旁通所有效果 |
| 配置持久化 | ✅ 设备选择/采样率/缓冲区大小、插件链、窗口位置自动保存至 `%AppData%/Minixer/` |

### 3.4 插件管理（Plugin Management）—— 插件扫描已完成（扫描入口在 Settings 页）

| 模块 | 功能描述 |
|------|----------|
| 插件扫描 | ✅ 复用 JUCE PluginListComponent 的 Options > “scan for new or updated VST3 plug-ins” 选择目录并扫描；后台线程扫描使原生进度对话框正常刷新，入口位于 Settings 页 |
| 黑名单 | 崩溃插件自动加入黑名单，下次启动跳过 |
| 格式支持 | 优先 VST3；可选 CLAP、LV2、LADSPA/DSSI |

### 3.5 插件崩溃隔离（Plugin Crash Protection）

| 模块 | 功能描述 |
|------|----------|
| 沙盒子进程 | 每个第三方插件运行在独立 `PluginHost.exe` 子进程 |
| IPC 音频传输 | 共享内存（Memory Mapped File）+ 无锁环形缓冲区，目标延迟 < 1ms |
| 崩溃检测 | 主进程通过 `WaitForSingleObject` 监控子进程状态 |
| 自动旁通 | 插件崩溃后，音频流自动直通（Bypass），不影响整体混音台 |
| 重启恢复 | 用户可一键重启崩溃插件，自动恢复先前参数状态 |

### 3.6 预设管理（Preset Management）—— 已完成

| 模块 | 功能描述 |
|------|----------|
| 预设管理 | ✅ 使用 XML 保存预设，提供预设列表，支持删除、重命名、复制预设 |
| 预设保存 | ✅ 保存当前插件链状态（含各插件参数）与通道条参数（Input Trim / Pan / Stereo / Output Fader） |
| 预设加载 | ✅ 加载已保存的预设，快速切换插件链与通道条参数 |

### 3.7 设备管理（Device Management）—— 设备选择已完成

| 模块 | 功能描述 |
|------|----------|
| 设备选择 | ✅ 用户可独立选择输入/输出设备（混合选择，即输入设备也可以是输出设备，反之亦然）、采样率、缓冲区大小、ASIO/WASAPI/DirectSound 切换 |
| 设备状态 | 显示当前选中设备的实时状态（如是否连接、是否支持实时处理） |

---

## 4. 架构设计

### 4.1 整体架构（以 VB-Audio Virtual Cable 为例）

```
┌─────────────────────────────────────────────────────────────┐
│                        UI Layer (GUI)                        │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────────────┐  │
│  │  Mixer GUI  │  │ Plugin Rack │  │  System Tray / Menu │  │
│  └──────┬──────┘  └──────┬──────┘  └──────────┬──────────┘  │
└─────────┼────────────────┼──────────────────┼───────────────┘
          │                │                  │
          ▼                ▼                  ▼
┌─────────────────────────────────────────────────────────────┐
│                   Application Layer                          │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────────┐  │
│  │MixerController│  │  PluginHost  │  │  PresetManager   │  │
│  │  (状态/UI桥接) │  │ (扫描/实例化) │  │ (XML/JSON 序列化)│  │
│  └──────┬───────┘  └──────┬───────┘  └────────┬─────────┘  │
└─────────┼────────────────┼──────────────────┼───────────────┘
          │                │                  │
          ▼                ▼                  ▼
┌─────────────────────────────────────────────────────────────┐
│                    Audio Engine Layer                        │
│  ┌──────────────────────────────────────────────────────┐  │
│  │              AudioDeviceManager                       │  │
│  │   (Input: 物理麦克风  ──  Output: VB-CABLE Input)      │  │
│  └────────────────────┬─────────────────────────────────┘  │
│                       │                                      │
│                       ▼                                      │
│  ┌──────────────────────────────────────────────────────┐  │
│  │            AudioProcessorPlayer                       │  │
│  └────────────────────┬─────────────────────────────────┘  │
│                       │                                      │
│                       ▼                                      │
│  ┌──────────────────────────────────────────────────────┐  │
│  │            AudioProcessorGraph                        │  │
│  │  ┌────────┐   ┌────────┐   ┌────────┐   ┌────────┐ │  │
│  │  │ IO Input│ → │Sandbox │ → │Sandbox │ → │IO Output│ │  │
│  │  │  Node   │   │Plugin 1│   │Plugin 2│   │  Node   │ │  │
│  │  └────────┘   │(IPC)   │   │(IPC)   │   └────────┘ │  │
│  │               └────────┘   └────────┘                │  │
│  └──────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
         │                         │
         ▼ Shared Memory + Events   ▼
┌─────────────────────┐   ┌─────────────────────┐
│  PluginHost.exe #1  │   │  PluginHost.exe #2  │
│  ┌───────────────┐  │   │  ┌───────────────┐  │
│  │  VST3 Plugin  │  │   │  │  VST3 Plugin  │  │
│  │  (EQ)         │  │   │  │  (Compressor) │  │
│  └───────────────┘  │   │  └───────────────┘  │
└─────────────────────┘   └─────────────────────┘
```

### 4.2 IPC 机制

| 数据类型 | 机制 | 实现 |
|----------|------|------|
| 音频数据 | 共享内存 + 无锁环形缓冲区 | Windows `CreateFileMapping` / `MapViewOfFile` |
| 控制指令 | 命名管道 / Local Socket | `juce::InterprocessConnection` 或 Windows Named Pipe |
| 同步信号 | 事件对象 | Windows `CreateEvent` / `SetEvent` / `WaitForSingleObject` |

---

## 5. 风险与对策

| 风险 | 影响 | 对策 |
|------|------|------|
| IPC 延迟过高 | 实时性受损，出现卡顿 | 优化无锁环形缓冲区，避免 memcpy；使用事件同步替代轮询 |
| 插件 UI 跨进程嵌入复杂 | 开发周期延长 | 第一阶段使用独立浮动窗口；第二阶段再优化内嵌 |
| 某些 VST3 插件不兼容沙盒 | 功能受限 | 提供"内联模式"开关，对可信插件直接内联处理 |
| 音频线程死锁 | 整个系统音频卡死 | 所有插件操作（加载/删除）必须在音频线程外完成，使用 `MessageManager::callAsync` |

---

## 6. 许可与合规

### 6.1 软件协议
- **Minixer 本体**：AGPLv3 协议开源
- **JUCE**：AGPLv3 / 商业许可（根据使用方式选择）
- **VST3 SDK**：Steinberg 许可（需遵守 VST3 商标和分发规则）

---

## 7. 附录

### 7.1 参考资源
- [JUCE AudioProcessorGraph Tutorial](https://docs.juce.com/tutorial_audio_processor_graph/)
- [JUCE Plugin Host Example](https://github.com/juce-framework/JUCE/tree/master/examples/Audio/PluginHost)
- [Windows Memory Mapped Files](https://docs.microsoft.com/en-us/windows/win32/memory/memory-mapped-files)

### 7.2 术语表
| 术语 | 说明 |
|------|------|
| DAW | Digital Audio Workstation，数字音频工作站 |
| VST | Virtual Studio Technology，Steinberg 开发的音频插件标准 |
| IPC | Inter-Process Communication，进程间通信 |
| WASAPI | Windows Audio Session API，Windows 音频 API |
| VB-CABLE | VB-Audio 开发的虚拟音频线缆驱动 |
| SPSC | Single-Producer Single-Consumer，单生产者单消费者队列 |

---

*本文档为 Minixer 项目开发计划草案，后续根据开发进展迭代更新。*

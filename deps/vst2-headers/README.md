# vst2-headers

VST 2.x 插件宿主 ABI 的净室头文件（Steinberg 命名版），供 JUCE 的
VST2 宿主代码（`JUCE_PLUGINHOST_VST=1`）编译使用。

## 为什么存在

Steinberg 已于 2018 年停止分发 VST2 SDK，其头文件许可证不允许公开
再分发。本目录的头文件以 Xaymar/vst2sdk（BSD-3-Clause 净室逆向工程）
的 ABI 定义为蓝本改写为 Steinberg 命名（JUCE 按此命名访问），因此可以
合法地随源码仓库分发并用于编译。

- 结构体布局、枚举数值：逐一对照 Xaymar/vst2sdk `include/vst.h`
- 宿主操作码（AudioMasterOpcodes）编号：以实战验证过的 VeSTige 系
  线协议实现为准（蓝本在 0x07-0x09 段与线协议有出入）

## 布局

```
pluginterfaces/vst2.x/aeffect.h    # 核心：AEffect、操作码、标志、ERect
pluginterfaces/vst2.x/aeffectx.h   # 扩展：宿主操作码、事件、时间信息、扬声器等
```

该布局匹配 JUCE 的包含方式（`#include <pluginterfaces/vst2.x/aeffect.h>`），
由 CMake 的 `juce_set_vst2_sdk_path()` 指向本目录。

## 替换为官方 SDK

若日后取得 Steinberg VST2 SDK 授权，直接用官方 SDK 头替换本目录内容即可，
构建配置无需变动。

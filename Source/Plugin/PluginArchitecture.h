/*
  ==============================================================================

    PluginArchitecture.h
    判断 VST3 插件文件的目标 CPU 架构，为 32/64-bit 桥接提供决策依据。

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

namespace minixer
{

//==============================================================================
/** VST3 插件可运行的目标架构。 */
enum class PluginArchitecture
{
    unknown,    /**< 无法识别或不受支持的架构。 */
    x86,        /**< 32-bit x86 (IMAGE_FILE_MACHINE_I386)。 */
    x64,        /**< 64-bit x86-64 (IMAGE_FILE_MACHINE_AMD64)。 */
    arm64       /**< ARM64 (IMAGE_FILE_MACHINE_ARM64)，暂不支持。 */
};

//==============================================================================
/** 读取 VST3 文件 PE 头，返回其目标架构。

    Windows VST3 本质上是 PE 格式 DLL。本函数在真正加载插件前做以下预检：
    1. 跳过 macOS 资源分支文件（以 "._" 开头）。
    2. 读取 PE 头，确认文件是有效的 PE 映像；否则返回 unknown。

    非 Windows 平台始终返回 unknown，因为桥接器目前只在 Windows 上实现。
*/
PluginArchitecture detectPluginArchitecture (const juce::File& vst3File);

//==============================================================================
/** 判断指定架构是否可被当前宿主进程直接加载。

    64-bit 宿主可直接加载 64-bit 插件；32-bit 插件需要通过 PluginHost32.exe
    子进程桥接。
*/
bool canHostLoadArchitectureDirectly (PluginArchitecture arch);

} // namespace minixer

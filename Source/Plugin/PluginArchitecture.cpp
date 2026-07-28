/*
  ==============================================================================

    PluginArchitecture.cpp

  ==============================================================================
*/

#include "PluginArchitecture.h"

namespace minixer
{

//==============================================================================
PluginArchitecture detectPluginArchitecture (const juce::File& vst3File)
{
   #if JUCE_WINDOWS
    if (! vst3File.hasFileExtension (".vst3"))
        return PluginArchitecture::unknown;

    // 跳过 macOS AppleDouble / 资源分支文件，例如 "._ReLife.vst3"
    if (vst3File.getFileName().startsWith ("._"))
        return PluginArchitecture::unknown;

    juce::FileInputStream stream (vst3File);

    if (! stream.openedOk())
        return PluginArchitecture::unknown;

    // DOS header magic: 'MZ' (0x5A4D)
    juce::uint16 dosMagic = 0;

    if (stream.read (&dosMagic, sizeof (dosMagic)) != sizeof (dosMagic)
        || dosMagic != 0x5A4D)
    {
        return PluginArchitecture::unknown;
    }

    // e_lfanew 位于 DOS header 偏移 0x3C 处
    if (! stream.setPosition (0x3C))
        return PluginArchitecture::unknown;

    juce::uint32 peOffset = 0;

    if (stream.read (&peOffset, sizeof (peOffset)) != sizeof (peOffset))
        return PluginArchitecture::unknown;

    // PE signature: 'PE\0\0' (0x00004550)
    if (! stream.setPosition (static_cast<juce::int64> (peOffset)))
        return PluginArchitecture::unknown;

    juce::uint32 peSignature = 0;

    if (stream.read (&peSignature, sizeof (peSignature)) != sizeof (peSignature)
        || peSignature != 0x00004550)
    {
        return PluginArchitecture::unknown;
    }

    // COFF file header 的 Machine 字段紧跟在 PE signature 之后
    juce::uint16 machine = 0;

    if (stream.read (&machine, sizeof (machine)) != sizeof (machine))
        return PluginArchitecture::unknown;

    switch (machine)
    {
        case 0x014c: return PluginArchitecture::x86;
        case 0x8664: return PluginArchitecture::x64;
        case 0xaa64: return PluginArchitecture::arm64;
        default:     return PluginArchitecture::unknown;
    }
   #elif JUCE_LINUX
    // Linux 插件为 ELF 映像：VST3 是 bundle 目录，VST2 是裸 .so 文件。
    if (vst3File.isDirectory())
    {
        // VST3 bundle：二进制位于 Contents/<arch>-linux/ 下，目录名直接决定架构。
        // 两者皆存在时优先按 x86_64 判定（与 bundle 内实际加载顺序无关，仅为确定性）。
        const auto x64BinaryDir = vst3File.getChildFile ("Contents/x86_64-linux");
        const auto x86BinaryDir = vst3File.getChildFile ("Contents/i686-linux");

        if (! x64BinaryDir.findChildFiles (juce::File::findFiles, false, "*.so").isEmpty())
            return PluginArchitecture::x64;

        if (! x86BinaryDir.findChildFiles (juce::File::findFiles, false, "*.so").isEmpty())
            return PluginArchitecture::x86;

        return PluginArchitecture::unknown;
    }

    juce::FileInputStream stream (vst3File);

    if (! stream.openedOk())
        return PluginArchitecture::unknown;

    // e_ident[EI_MAG0..3]: 0x7F 'E' 'L' 'F'（逐字节比较，与宿主字节序无关）
    char elfMagic[4] = {};

    if (stream.read (elfMagic, 4) != 4
        || elfMagic[0] != 0x7F || elfMagic[1] != 'E' || elfMagic[2] != 'L' || elfMagic[3] != 'F')
    {
        return PluginArchitecture::unknown;
    }

    // e_ident[EI_CLASS]（偏移 4）：1 = 32-bit，2 = 64-bit
    juce::uint8 eiClass = 0;

    if (stream.read (&eiClass, 1) != 1)
        return PluginArchitecture::unknown;

    // e_ident[EI_DATA]（偏移 5）：仅接受小端（1 = ELFDATA2LSB）
    juce::uint8 eiData = 0;

    if (stream.read (&eiData, 1) != 1 || eiData != 1)
        return PluginArchitecture::unknown;

    // e_machine（偏移 18，uint16 小端）与 EI_CLASS 交叉校验
    if (! stream.setPosition (18))
        return PluginArchitecture::unknown;

    juce::uint16 machine = 0;

    if (stream.read (&machine, 2) != 2)
        return PluginArchitecture::unknown;

    switch (machine)
    {
        case 0x03: // EM_386
            return eiClass == 1 ? PluginArchitecture::x86 : PluginArchitecture::unknown;
        case 0x3E: // EM_X86_64
            return eiClass == 2 ? PluginArchitecture::x64 : PluginArchitecture::unknown;
        case 0xB7: // EM_AARCH64
            return PluginArchitecture::arm64;
        default:
            return PluginArchitecture::unknown;
    }
   #else
    juce::ignoreUnused (vst3File);
    return PluginArchitecture::unknown;
   #endif
}

//==============================================================================
bool canHostLoadArchitectureDirectly (PluginArchitecture arch)
{
   #if JUCE_64BIT
    return arch == PluginArchitecture::x64;
   #else
    return arch == PluginArchitecture::x86;
   #endif
}

} // namespace minixer

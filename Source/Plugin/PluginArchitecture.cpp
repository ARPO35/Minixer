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
   #if ! JUCE_WINDOWS
    juce::ignoreUnused (vst3File);
    return PluginArchitecture::unknown;
   #else
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

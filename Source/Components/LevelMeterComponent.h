#pragma once

#include <JuceHeader.h>
#include <array>
#include <cstddef>
#include <functional>
#include "../LookAndFeel/MixerLookAndFeel.h"

namespace minixer
{

//==============================================================================
/** 电平表计量标准。 */
enum class MeterStandard
{
    dBFS = 0,
    RMS,
    LUFS_Momentary,
    LUFS_ShortTerm
};

//==============================================================================
/** 返回计量标准的显示名称。 */
inline juce::String getMeterStandardName (MeterStandard standard)
{
    switch (standard)
    {
        case MeterStandard::dBFS:           return TRANS("dBFS");
        case MeterStandard::RMS:            return TRANS("RMS");
        case MeterStandard::LUFS_Momentary: return TRANS("LUFS-M");
        case MeterStandard::LUFS_ShortTerm: return TRANS("LUFS-S");
    }

    return TRANS("dBFS");
}

/** 返回默认计量标准。 */
inline MeterStandard getDefaultMeterStandard() noexcept { return MeterStandard::dBFS; }

//==============================================================================
/** 实时双声道电平表组件。

    在组件内并排显示左/右声道的电平，支持 dBFS、RMS、LUFS-M、LUFS-S 等多种
    计量标准。用户可通过右键菜单切换标准，并提供恢复默认的选项。
    电平数据由外部通过 setLevel() / setLevels() 方法从音频线程或主线程更新。
*/
class LevelMeterComponent  : public juce::Component,
                             private juce::Timer
{
public:
    LevelMeterComponent();
    ~LevelMeterComponent() override;

    //==============================================================================
    /** 设置指定声道的当前电平。

        channel 必须为 0（左声道）或 1（右声道）。
        线程安全，可从音频线程调用。
    */
    void setLevel (int channel, float levelDb);

    /** 同时设置左、右声道的当前电平。线程安全，可从音频线程调用。 */
    void setLevels (float leftDb, float rightDb);

    /** 清除两个声道的峰值保持与过载状态。 */
    void reset();

    //==============================================================================
    /** 返回当前使用的计量标准。 */
    MeterStandard getCurrentStandard() const noexcept { return currentStandard; }

    /** 设置计量标准并清空当前峰值状态以便立即切换到新标准。 */
    void setCurrentStandard (MeterStandard newStandard);

    /** 标准切换时的回调，可用于持久化用户选择。 */
    std::function<void (MeterStandard)> onStandardChanged;

    //==============================================================================
    void paint (juce::Graphics& g) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent& event) override;

private:
    //==============================================================================
    void timerCallback() override;
    void drawScale (juce::Graphics& g, const juce::Rectangle<float>& scaleBounds,
                    const juce::Rectangle<float>& meterBounds) const;
    void showStandardMenu();

    //==============================================================================
    static constexpr float minDb = -60.0f;
    static constexpr float maxDb = 6.0f;
    static constexpr float clipThresholdDb = 0.0f;
    static constexpr int holdDecayMs = 1500;      // 峰值保持衰减时间
    static constexpr int timerIntervalMs = 30;    // ~33fps
    static constexpr int numChannels = 2;
    static constexpr float meterGap = 2.0f;       // 左右电平条之间的间距
    static constexpr float labelHeight = 14.0f;   // 底部标准标签高度
    static constexpr int resetStandardMenuId = 100;

    // 当前计量标准。
    MeterStandard currentStandard = getDefaultMeterStandard();

    // 标准 dBFS 刻度值（从 minDb 到 maxDb，按 6 dB 步进）。
    // 为不同高度提供预筛选的子集，确保 0 dB 与 +6 dB 始终可见。
    static constexpr std::array<float, 12> scaleMarksDb = {{
        -60.0f, -54.0f, -48.0f, -42.0f, -36.0f, -30.0f,
        -24.0f, -18.0f, -12.0f, -6.0f, 0.0f, +6.0f
    }};

    static constexpr std::array<float, 7> scaleMarksDbMedium = {{
        -60.0f, -48.0f, -36.0f, -24.0f, -12.0f, 0.0f, +6.0f
    }};

    static constexpr std::array<float, 5> scaleMarksDbSmall = {{
        -60.0f, -36.0f, -12.0f, 0.0f, +6.0f
    }};

    static constexpr std::array<float, 4> scaleMarksDbTiny = {{
        -60.0f, -12.0f, 0.0f, +6.0f
    }};

    juce::Atomic<float> currentLevelDb[numChannels] { { minDb }, { minDb } };
    float displayedLevelDb[numChannels] = { minDb, minDb };
    float peakDb[numChannels] = { minDb, minDb };
    bool isClipping[numChannels] = { false, false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LevelMeterComponent)
};

} // namespace minixer

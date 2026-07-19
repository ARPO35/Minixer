#pragma once

#include <JuceHeader.h>

namespace minixer
{

//==============================================================================
/** 深色主题 LookAndFeel，为 Minixer 提供一致的视觉风格。

    设计目标：
    - 深色背景，减少长时间使用的视觉疲劳
    - 高对比度的控件边缘和激活状态
    - 电平表使用经典的绿/黄/红分段
    - 旋钮和推子具有 DAW 风格的专业外观
*/
class MixerLookAndFeel  : public juce::LookAndFeel_V4
{
public:
    MixerLookAndFeel();
    ~MixerLookAndFeel() override = default;

    //==============================================================================
    // 颜色访问器
    static juce::Colour getBackgroundColour() noexcept       { return juce::Colour (0xFF1A1A1A); }
    static juce::Colour getSurfaceColour() noexcept          { return juce::Colour (0xFF252525); }
    static juce::Colour getElevatedColour() noexcept         { return juce::Colour (0xFF333333); }
    static juce::Colour getBorderColour() noexcept           { return juce::Colour (0xFF444444); }
    static juce::Colour getTextColour() noexcept             { return juce::Colour (0xFFE0E0E0); }
    static juce::Colour getMutedTextColour() noexcept        { return juce::Colour (0xFF888888); }
    static juce::Colour getAccentColour() noexcept           { return juce::Colour (0xFF4A90D9); }
    static juce::Colour getAccentHoverColour() noexcept      { return juce::Colour (0xFF5BA3EC); }
    static juce::Colour getMeterGreen() noexcept             { return juce::Colour (0xFF2ECC71); }
    static juce::Colour getMeterYellow() noexcept            { return juce::Colour (0xFFF1C40F); }
    static juce::Colour getMeterRed() noexcept               { return juce::Colour (0xFFE74C3C); }
    static juce::Colour getClipColour() noexcept             { return juce::Colour (0xFFE74C3C); }

    // 旋钮和滑条的方向性颜色（正值/负值/中性）
    static juce::Colour getPositiveAccentColour() noexcept   { return juce::Colour (0xFFFF8B47); }  // 暖橙色（正值/增加）
    static juce::Colour getNegativeAccentColour() noexcept   { return juce::Colour (0xFF4A90D9); }  // 冷蓝色（负值/减少）
    static juce::Colour getNeutralAccentColour() noexcept    { return juce::Colour (0xFF666666); }  // 中性灰（初始值）

    //==============================================================================
    // LookAndFeel 重写
    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPosProportional, float rotaryStartAngle,
                           float rotaryEndAngle, juce::Slider& slider) override;

    void drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPos, float minSliderPos, float maxSliderPos,
                           const juce::Slider::SliderStyle style, juce::Slider& slider) override;

    void drawButtonBackground (juce::Graphics& g, juce::Button& button,
                               const juce::Colour& backgroundColour,
                               bool shouldDrawButtonAsHighlighted,
                               bool shouldDrawButtonAsDown) override;

    void drawComboBox (juce::Graphics& g, int width, int height, bool isButtonDown,
                       int buttonX, int buttonY, int buttonW, int buttonH,
                       juce::ComboBox& box) override;

    void positionComboBoxText (juce::ComboBox& box, juce::Label& label) override;

    void drawPopupMenuItem (juce::Graphics& g, const juce::Rectangle<int>& area,
                            const bool isSeparator, const bool isActive,
                            const bool isHighlighted, const bool isTicked,
                            const bool hasSubMenu, const juce::String& text,
                            const juce::String& shortcutKeyText,
                            const juce::Drawable* icon, const juce::Colour* textColour) override;

    void fillTextEditorBackground (juce::Graphics& g, int width, int height,
                                   juce::TextEditor& textEditor) override;

    void drawTextEditorOutline (juce::Graphics& g, int width, int height,
                                juce::TextEditor& textEditor) override;

    //==============================================================================
    // 自定义绘制辅助函数
    static void drawMeterBar (juce::Graphics& g, const juce::Rectangle<float>& bounds,
                              float levelDb, float peakDb, bool isClipping,
                              float maxDb = 0.0f);

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MixerLookAndFeel)
};

} // namespace minixer

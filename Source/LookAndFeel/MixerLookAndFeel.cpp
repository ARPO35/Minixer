#include "MixerLookAndFeel.h"
#include <cmath>

namespace minixer
{

//==============================================================================
MixerLookAndFeel::MixerLookAndFeel()
{
    // ========== 动态颜色测试标记 V2 - 已修复淡蓝色问题 ==========
    setColour (juce::ResizableWindow::backgroundColourId, getBackgroundColour());
    setColour (juce::ComboBox::backgroundColourId,         getSurfaceColour());
    setColour (juce::ComboBox::textColourId,               getTextColour());
    setColour (juce::ComboBox::arrowColourId,              getTextColour());
    setColour (juce::ComboBox::outlineColourId,            getBorderColour());
    setColour (juce::PopupMenu::backgroundColourId,        getSurfaceColour());
    setColour (juce::PopupMenu::textColourId,              getTextColour());
    setColour (juce::PopupMenu::highlightedBackgroundColourId, getElevatedColour());
    setColour (juce::PopupMenu::highlightedTextColourId,   getTextColour());
    setColour (juce::TextButton::buttonColourId,           getElevatedColour());
    setColour (juce::TextButton::buttonOnColourId,         getAccentColour());
    setColour (juce::TextButton::textColourOnId,           juce::Colours::white);
    setColour (juce::TextButton::textColourOffId,          getTextColour());
    setColour (juce::TextEditor::backgroundColourId,       getSurfaceColour());
    setColour (juce::TextEditor::textColourId,             getTextColour());
    setColour (juce::TextEditor::outlineColourId,          getBorderColour());
    setColour (juce::TextEditor::focusedOutlineColourId,   getAccentColour());

    // ========== Slider 颜色完全由 drawRotarySlider/drawLinearSlider 动态决定 ==========
    // 不在这里设置任何 Slider 颜色，避免覆盖动态颜色逻辑
    setColour (juce::Slider::backgroundColourId,           getSurfaceColour());
    setColour (juce::Slider::rotarySliderOutlineColourId,  getElevatedColour());  // 旋钮轮廓
    setColour (juce::Slider::rotarySliderFillColourId,     getNeutralAccentColour());  // 默认填充（中性灰）
    setColour (juce::Slider::textBoxTextColourId,          getTextColour());
    setColour (juce::Slider::textBoxBackgroundColourId,    getSurfaceColour());
    setColour (juce::Slider::textBoxOutlineColourId,       getBorderColour());
    setColour (juce::Label::textColourId,                  getTextColour());
    setColour (juce::TooltipWindow::backgroundColourId,    getElevatedColour());
    setColour (juce::TooltipWindow::textColourId,          getTextColour());
}

//==============================================================================
void MixerLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                         float sliderPosProportional, float rotaryStartAngle,
                                         float rotaryEndAngle, juce::Slider& slider)
{
    auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat().reduced (2.0f);
    auto radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    auto centre = bounds.getCentre();
    auto angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

    // 根据 slider 的当前值动态选择颜色
    auto currentValue = slider.getValue();
    auto neutralThreshold = 0.01;  // 接近零的阈值

    juce::Colour accentColour;
    if (std::abs (currentValue) < neutralThreshold)
    {
        // 中性值（接近零）：使用中性灰
        accentColour = getNeutralAccentColour();
    }
    else if (currentValue > 0.0)
    {
        // 正值：使用暖橙色
        accentColour = getPositiveAccentColour();
    }
    else
    {
        // 负值：使用冷蓝色
        accentColour = getNegativeAccentColour();
    }

    // 背景圆环
    g.setColour (getSurfaceColour());
    g.fillEllipse (centre.x - radius, centre.y - radius, radius * 2.0f, radius * 2.0f);

    // 外圈
    g.setColour (getBorderColour());
    g.drawEllipse (centre.x - radius, centre.y - radius, radius * 2.0f, radius * 2.0f, 1.5f);

    // 弧线轨道（中性灰色）
    juce::Path arcBackground;
    arcBackground.addCentredArc (centre.x, centre.y, radius * 0.8f, radius * 0.8f,
                                 0.0f, rotaryStartAngle, rotaryEndAngle, true);
    g.setColour (getElevatedColour());
    g.strokePath (arcBackground, juce::PathStrokeType (4.0f));

    // 计算中间值（默认值）对应的角度
    // 所有旋钮的默认值都是 0（中间位置）
    auto range = slider.getMaximum() - slider.getMinimum();
    constexpr double defaultValue = 0.0;

    double neutralProportion = (range > 0.0) ? (defaultValue - slider.getMinimum()) / range : 0.5;
    neutralProportion = juce::jlimit (0.0, 1.0, neutralProportion);

    auto neutralAngle = static_cast<float> (rotaryStartAngle + neutralProportion * (rotaryEndAngle - rotaryStartAngle));

    // 值弧线：从中间位置向当前值位置填充
    if (std::abs (angle - neutralAngle) > 0.001f)
    {
        juce::Path arcValue;
        auto startAngle = juce::jmin (angle, neutralAngle);
        auto endAngle = juce::jmax (angle, neutralAngle);
        arcValue.addCentredArc (centre.x, centre.y, radius * 0.8f, radius * 0.8f,
                                0.0f, startAngle, endAngle, true);
        g.setColour (accentColour);
        g.strokePath (arcValue, juce::PathStrokeType (4.0f));
    }

    // 指针（使用文本色，不随值变化）
    juce::Line<float> pointerLine (centre, centre.getPointOnCircumference (radius * 0.6f, angle));
    g.setColour (getTextColour());
    g.drawLine (pointerLine, 2.5f);

    // 中心点（使用动态颜色）
    g.setColour (accentColour);
    g.fillEllipse (centre.x - 3.0f, centre.y - 3.0f, 6.0f, 6.0f);
}

//==============================================================================
void MixerLookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                                         float sliderPos, float minSliderPos, float maxSliderPos,
                                         const juce::Slider::SliderStyle style, juce::Slider& slider)
{
    if (style == juce::Slider::LinearVertical)
    {
        auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat().reduced (4.0f, 2.0f);
        auto trackWidth = juce::jmin (12.0f, bounds.getWidth() * 0.35f);
        auto trackBounds = bounds.withSizeKeepingCentre (trackWidth, bounds.getHeight());

        // 根据 slider 的当前值动态选择颜色
        auto currentValue = slider.getValue();
        auto neutralThreshold = 0.01;  // 接近零的阈值

        juce::Colour accentColour;
        if (std::abs (currentValue) < neutralThreshold)
        {
            // 中性值（接近零）：使用中性灰
            accentColour = getNeutralAccentColour();
        }
        else if (currentValue > 0.0)
        {
            // 正值：使用暖橙色
            accentColour = getPositiveAccentColour();
        }
        else
        {
            // 负值：使用冷蓝色
            accentColour = getNegativeAccentColour();
        }

        // 轨道槽
        g.setColour (getSurfaceColour());
        g.fillRoundedRectangle (trackBounds, trackWidth * 0.5f);

        // 轨道边框
        g.setColour (getBorderColour());
        g.drawRoundedRectangle (trackBounds, trackWidth * 0.5f, 1.0f);

        // 值填充（使用动态颜色）
        auto fillBounds = trackBounds.withTop (sliderPos);
        g.setColour (accentColour);
        g.fillRoundedRectangle (fillBounds, trackWidth * 0.5f);

        // 推子头
        auto thumbWidth = juce::jmin (26.0f, bounds.getWidth() * 0.8f);
        auto thumbHeight = 10.0f;
        auto thumbBounds = juce::Rectangle<float> (bounds.getCentreX() - thumbWidth * 0.5f,
                                                   sliderPos - thumbHeight * 0.5f,
                                                   thumbWidth, thumbHeight);
        g.setColour (getElevatedColour());
        g.fillRoundedRectangle (thumbBounds, 2.0f);
        g.setColour (getTextColour());
        g.drawRoundedRectangle (thumbBounds, 2.0f, 1.0f);
        g.setColour (accentColour);
        g.drawHorizontalLine (juce::roundToInt (sliderPos), thumbBounds.getX() + 4.0f, thumbBounds.getRight() - 4.0f);
    }
    else
    {
        // 水平滑块回退到父类实现
        LookAndFeel_V4::drawLinearSlider (g, x, y, width, height, sliderPos,
                                          minSliderPos, maxSliderPos, style, slider);
    }
}

//==============================================================================
void MixerLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& button,
                                             const juce::Colour& /*backgroundColour*/,
                                             bool shouldDrawButtonAsHighlighted,
                                             bool shouldDrawButtonAsDown)
{
    auto bounds = button.getLocalBounds().toFloat().reduced (1.0f);
    auto baseColour = button.getToggleState() ? getAccentColour() : getElevatedColour();

    if (shouldDrawButtonAsDown)
        baseColour = baseColour.darker (0.2f);
    else if (shouldDrawButtonAsHighlighted)
        baseColour = baseColour.brighter (0.1f);

    g.setColour (baseColour);
    g.fillRoundedRectangle (bounds, 4.0f);

    g.setColour (getBorderColour());
    g.drawRoundedRectangle (bounds, 4.0f, 1.0f);
}

//==============================================================================
void MixerLookAndFeel::drawComboBox (juce::Graphics& g, int width, int height, bool /*isButtonDown*/,
                                     int buttonX, int buttonY, int buttonW, int buttonH,
                                     juce::ComboBox& box)
{
    auto bounds = juce::Rectangle<int> (0, 0, width, height).toFloat().reduced (1.0f);

    g.setColour (box.findColour (juce::ComboBox::backgroundColourId));
    g.fillRoundedRectangle (bounds, 4.0f);

    g.setColour (box.findColour (juce::ComboBox::outlineColourId));
    g.drawRoundedRectangle (bounds, 4.0f, 1.0f);

    // 下拉箭头
    auto arrowArea = juce::Rectangle<int> (buttonX, buttonY, buttonW, buttonH).toFloat().reduced (6.0f);
    juce::Path arrow;
    arrow.addTriangle (arrowArea.getX(), arrowArea.getY(),
                       arrowArea.getRight(), arrowArea.getY(),
                       arrowArea.getCentreX(), arrowArea.getBottom());
    g.setColour (box.findColour (juce::ComboBox::arrowColourId));
    g.fillPath (arrow);
}

//==============================================================================
void MixerLookAndFeel::positionComboBoxText (juce::ComboBox& box, juce::Label& label)
{
    label.setBounds (4, 1, box.getWidth() - 24, box.getHeight() - 2);
    label.setFont (juce::Font (juce::FontOptions (13.0f)));
    label.setJustificationType (juce::Justification::centredLeft);
}

//==============================================================================
void MixerLookAndFeel::drawPopupMenuItem (juce::Graphics& g, const juce::Rectangle<int>& area,
                                          const bool isSeparator, const bool /*isActive*/,
                                          const bool isHighlighted, const bool /*isTicked*/,
                                          const bool /*hasSubMenu*/, const juce::String& text,
                                          const juce::String& /*shortcutKeyText*/,
                                          const juce::Drawable* /*icon*/, const juce::Colour* /*textColour*/)
{
    if (isSeparator)
    {
        g.setColour (getBorderColour());
        g.fillRect (area.withSizeKeepingCentre (area.getWidth() - 10, 1));
        return;
    }

    if (isHighlighted)
    {
        g.setColour (findColour (juce::PopupMenu::highlightedBackgroundColourId));
        g.fillRect (area);
    }

    g.setColour (findColour (juce::PopupMenu::textColourId));
    g.setFont (juce::Font (juce::FontOptions (13.0f)));
    g.drawText (text, area.reduced (10, 0), juce::Justification::centredLeft, true);
}

//==============================================================================
void MixerLookAndFeel::fillTextEditorBackground (juce::Graphics& g, int width, int height,
                                                 juce::TextEditor& textEditor)
{
    g.setColour (textEditor.findColour (juce::TextEditor::backgroundColourId));
    g.fillRoundedRectangle (juce::Rectangle<int> (0, 0, width, height).toFloat().reduced (1.0f), 4.0f);
}

//==============================================================================
void MixerLookAndFeel::drawTextEditorOutline (juce::Graphics& g, int width, int height,
                                              juce::TextEditor& textEditor)
{
    auto bounds = juce::Rectangle<int> (0, 0, width, height).toFloat().reduced (1.0f);
    g.setColour (textEditor.hasKeyboardFocus (true)
                     ? textEditor.findColour (juce::TextEditor::focusedOutlineColourId)
                     : textEditor.findColour (juce::TextEditor::outlineColourId));
    g.drawRoundedRectangle (bounds, 4.0f, 1.0f);
}

//==============================================================================
void MixerLookAndFeel::drawMeterBar (juce::Graphics& g, const juce::Rectangle<float>& bounds,
                                     float levelDb, float peakDb, bool isClipping,
                                     float maxDb)
{
    const float minDb = -60.0f;
    const float yellowThresholdDb = -12.0f;
    const float redThresholdDb = -6.0f;
    const auto totalHeight = bounds.getHeight();

    // 复制一份非 const 的 bounds，因为后续需要使用 removeFromBottom
    auto meterBounds = bounds;

    // 背景
    g.setColour (getSurfaceColour());
    g.fillRoundedRectangle (bounds, 2.0f);

    // 分段刻度线：空间充裕时每 6dB 一条，窄空间每 12dB 一条
    g.setColour (getBorderColour());
    const auto tickStepDb = (totalHeight >= 80.0f) ? 6.0f : 12.0f;

    for (float db = minDb; db <= maxDb + 0.001f; db += tickStepDb)
    {
        auto y = bounds.getBottom() - juce::jmap (db, minDb, maxDb, 0.0f, totalHeight);
        g.drawHorizontalLine (juce::roundToInt (y), bounds.getX() + 2.0f, bounds.getRight() - 2.0f);
    }

    // 限制电平值范围
    auto clampedLevel = juce::jlimit (minDb, maxDb, levelDb);
    auto levelHeight = juce::jmap (clampedLevel, minDb, maxDb, 0.0f, totalHeight);

    if (levelHeight > 1.0f)
    {
        auto levelBounds = meterBounds.removeFromBottom (levelHeight);

        // 根据阈值分段着色
        auto greenHeight = juce::jlimit (0.0f, levelBounds.getHeight(),
                                         juce::jmap (yellowThresholdDb, minDb, maxDb, 0.0f, totalHeight));
        auto yellowHeight = juce::jlimit (0.0f, levelBounds.getHeight() - greenHeight,
                                          juce::jmap (redThresholdDb, minDb, maxDb, 0.0f, totalHeight) - greenHeight);

        auto greenBounds = levelBounds.removeFromBottom (greenHeight);
        g.setColour (getMeterGreen());
        g.fillRoundedRectangle (greenBounds, 2.0f);

        if (levelBounds.getHeight() > 0.0f)
        {
            auto yellowBounds = levelBounds.removeFromBottom (juce::jmin (yellowHeight, levelBounds.getHeight()));
            g.setColour (getMeterYellow());
            g.fillRoundedRectangle (yellowBounds, 2.0f);

            if (levelBounds.getHeight() > 0.0f)
            {
                g.setColour (getMeterRed());
                g.fillRoundedRectangle (levelBounds, 2.0f);
            }
        }
    }

    // 峰值保持线
    if (peakDb > minDb)
    {
        auto peakY = bounds.getBottom() - juce::jmap (juce::jlimit (minDb, maxDb, peakDb),
                                                      minDb, maxDb, 0.0f, totalHeight);
        g.setColour (isClipping ? getClipColour() : getTextColour());
        g.drawHorizontalLine (juce::roundToInt (peakY), bounds.getX(), bounds.getRight());
    }
}

} // namespace minixer

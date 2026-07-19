#pragma once

#include <JuceHeader.h>
#include <functional>

namespace minixer
{

//==============================================================================
/** 带标签的旋钮组件。

    基于 juce::Slider，使用 RotaryVerticalDrag 风格，
    并在下方显示参数名和当前值。

    额外功能：
    - 右键菜单：重置为默认值、复制当前值、粘贴值
    - Shift / Alt + 拖动：进入精调模式（灵敏度降低为 0.2x）
*/
class RotaryKnobComponent  : public juce::Component
{
public:
    //==============================================================================
    RotaryKnobComponent (const juce::String& paramName,
                         double minValue,
                         double maxValue,
                         double defaultValue,
                         double interval = 0.1,
                         const juce::String& suffix = {});
    ~RotaryKnobComponent() override = default;

    //==============================================================================
    juce::Slider& getSlider() noexcept { return slider; }

    //==============================================================================
    void paint (juce::Graphics& g) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent& event) override;

    /** 设置数值后缀（如 "dB"、"%"）。 */
    void setValueSuffix (const juce::String& suffix);

    /** 设置自定义值文本格式化函数。传入 nullptr 可恢复默认数值显示。 */
    using ValueFormatter = std::function<juce::String (double)>;
    void setValueFormatter (ValueFormatter formatter);

    /** 获取参数名，用于剪贴板标识。 */
    juce::String getParameterName() const noexcept { return paramName; }

    /** 获取当前格式化后的显示文本。 */
    juce::String getDisplayValueText() const;

    /** 重置为默认值。 */
    void resetToDefault();

    //==============================================================================
    class Listener
    {
    public:
        virtual ~Listener() = default;
        virtual void rotaryKnobValueChanged (RotaryKnobComponent* knob) = 0;
    };

    void addListener (Listener* listener) { listeners.add (listener); }
    void removeListener (Listener* listener) { listeners.remove (listener); }

private:
    //==============================================================================
    /** 支持 Shift/Alt 精调的 Slider 子类。 */
    class FineAdjustSlider  : public juce::Slider
    {
    public:
        FineAdjustSlider() = default;

        void mouseDown (const juce::MouseEvent& e) override
        {
            if (e.getNumberOfClicks() > 1)
            {
                setValue (getDoubleClickReturnValue(), juce::sendNotification);
                return;
            }

            beginDragPos = e.position;
            beginDragValue = getValue();
        }

        void mouseDrag (const juce::MouseEvent& e) override
        {
            auto range = getMaximum() - getMinimum();
            if (range <= 0.0)
                return;

            auto pixelsPerFullRange = 200.0;
            auto sensitivity = (e.mods.isShiftDown() || e.mods.isAltDown()) ? 0.2 : 1.0;

            auto deltaPixels = e.position.y - beginDragPos.y;
            auto delta = -deltaPixels / pixelsPerFullRange * range * sensitivity;

            auto newValue = juce::jlimit (getMinimum(), getMaximum(), beginDragValue + delta);
            setValue (newValue, juce::sendNotificationSync);
        }

        void mouseUp (const juce::MouseEvent& e) override
        {
            juce::Slider::mouseUp (e);
        }

    private:
        juce::Point<float> beginDragPos;
        double beginDragValue = 0.0;
    };

    //==============================================================================
    void updateValueLabel();
    void showContextMenu (juce::Point<int> clickPos);
    void copyValueToClipboard() const;
    void pasteValueFromClipboard();

    //==============================================================================
    FineAdjustSlider slider;
    juce::Label nameLabel;
    juce::Label valueLabel;
    juce::String suffix;
    juce::String paramName;
    ValueFormatter valueFormatter;
    double defaultValue = 0.0;
    juce::ListenerList<Listener> listeners;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RotaryKnobComponent)
};

} // namespace minixer

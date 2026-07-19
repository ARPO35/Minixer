#pragma once

#include <JuceHeader.h>

namespace minixer
{

//==============================================================================
/** 带标签的竖向推子组件。

    基于 juce::Slider，使用 LinearVertical 风格，
    适合用作输出推子或输入增益。

    额外功能：
    - 右键菜单：重置为默认值、复制当前值、粘贴值
    - Shift / Alt + 拖动：进入精调模式（灵敏度降低为 0.2x）
*/
class FaderComponent  : public juce::Component
{
public:
    //==============================================================================
    FaderComponent (const juce::String& paramName,
                    double minValueDb,
                    double maxValueDb,
                    double defaultValueDb);
    ~FaderComponent() override = default;

    //==============================================================================
    juce::Slider& getSlider() noexcept { return slider; }

    //==============================================================================
    void paint (juce::Graphics& g) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent& event) override;

    /** 获取参数名，用于剪贴板标识。 */
    juce::String getParameterName() const noexcept { return paramName; }

    /** 重置为默认值。 */
    void resetToDefault();

    //==============================================================================
    class Listener
    {
    public:
        virtual ~Listener() = default;
        virtual void faderValueChanged (FaderComponent* fader) = 0;
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

            auto deltaPixels = beginDragPos.y - e.position.y;
            auto delta = deltaPixels / pixelsPerFullRange * range * sensitivity;

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
    static juce::String valueToDbString (double valueDb);

    //==============================================================================
    FineAdjustSlider slider;
    juce::Label nameLabel;
    juce::Label valueLabel;
    juce::String paramName;
    double defaultValue = 0.0;
    juce::ListenerList<Listener> listeners;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FaderComponent)
};

} // namespace minixer

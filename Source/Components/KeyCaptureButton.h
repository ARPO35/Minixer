#pragma once

#include <JuceHeader.h>
#include "../Settings/ShortcutInputSource.h"
#include "../Settings/MidiShortcutInputManager.h"
#include "../Settings/HidShortcutInputManager.h"

namespace minixer
{

//==============================================================================
/** 用于捕获单个快捷键的按钮。

    点击后进入捕获模式，下一次按键（含修饰键）、MIDI 消息或 HID 事件即被记录为快捷键；
    按 Escape 取消捕获，按 Backspace 或 Delete 清除已绑定的快捷键。
*/
class KeyCaptureButton  : public juce::TextButton,
                          public juce::Timer,
                          public MidiShortcutInputManager::Listener,
                          public HidShortcutInputManager::Listener
{
public:
    //==============================================================================
    KeyCaptureButton();
    ~KeyCaptureButton() override;

    //==============================================================================
    /** 返回当前绑定的输入源。 */
    ShortcutInputSource getInputSource() const noexcept { return currentSource; }

    /** 返回当前绑定的键盘按键（若不是键盘来源则无效）。 */
    juce::KeyPress getKeyPress() const noexcept { return currentSource.getKeyPress(); }

    /** 设置当前绑定的输入源并刷新显示。 */
    void setInputSource (const ShortcutInputSource& source);

    /** 兼容旧代码：设置当前绑定的键盘按键。 */
    void setKeyPress (const juce::KeyPress& key);

    //==============================================================================
    class Listener
    {
    public:
        virtual ~Listener() = default;

        /** 用户完成一次捕获或清除后回调。 */
        virtual void inputSourceCaptureChanged (KeyCaptureButton* button, const ShortcutInputSource& newSource) = 0;

        /** 兼容旧代码。 */
        virtual void keyCaptureChanged (KeyCaptureButton* button, const juce::KeyPress& /*newKey*/) { juce::ignoreUnused (button); }
    };

    void addListener (Listener* listener)    { listeners.add (listener); }
    void removeListener (Listener* listener) { listeners.remove (listener); }

    //==============================================================================
    void clicked() override;
    bool keyPressed (const juce::KeyPress& key) override;
    void focusLost (FocusChangeType /*cause*/) override;

    //==============================================================================
    void timerCallback() override;

    //==============================================================================
    // MidiShortcutInputManager::Listener
    void midiShortcutMessageReceived (const juce::MidiMessage& message) override;

    // HidShortcutInputManager::Listener
    void hidShortcutEventReceived (const HidShortcutEvent& event) override;

private:
    //==============================================================================
    void startCapturing();
    void stopCapturing (bool notify);
    void commitSource (const ShortcutInputSource& source);
    void updateButtonText();
    juce::String inputSourceToDisplayString (const ShortcutInputSource& source) const;

    //==============================================================================
    ShortcutInputSource currentSource;
    bool isCapturing = false;

    juce::ListenerList<Listener> listeners;

    static constexpr int captureTimeoutMs = 5000;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KeyCaptureButton)
};

} // namespace minixer

#pragma once

#include <JuceHeader.h>

namespace minixer
{

//==============================================================================
/** 监听 MIDI 输入设备并将消息转发给快捷键系统的管理器。

    与音频处理链独立运行，仅用于捕获作为快捷键触发源的 MIDI 消息。
*/
class MidiShortcutInputManager  : public juce::MidiInputCallback
{
public:
    //==============================================================================
    MidiShortcutInputManager();
    ~MidiShortcutInputManager() override;

    //==============================================================================
    class Listener
    {
    public:
        virtual ~Listener() = default;

        /** 收到可用于触发快捷键的 MIDI 消息。 */
        virtual void midiShortcutMessageReceived (const juce::MidiMessage& message) = 0;
    };

    void addListener (Listener* listener)    { listeners.add (listener); }
    void removeListener (Listener* listener) { listeners.remove (listener); }

    //==============================================================================
    /** 返回当前可用的 MIDI 输入设备名称列表。 */
    juce::StringArray getAvailableDevices() const;

    /** 当前正在监听的设备名称；未监听时为空。 */
    juce::String getCurrentDeviceName() const noexcept { return currentDeviceName; }

    /** 开始监听指定名称的 MIDI 输入设备；空字符串表示停止监听。 */
    bool setDevice (const juce::String& deviceName);

    /** 停止所有 MIDI 监听。 */
    void stop();

    /** 重新连接上次使用的设备。 */
    bool reconnect();

    //==============================================================================
    /** 持久化配置用的键名。 */
    static const char* getSettingsKey() noexcept { return "midiShortcutInputDevice"; }

private:
    //==============================================================================
    void handleIncomingMidiMessage (juce::MidiInput* source, const juce::MidiMessage& message) override;

    //==============================================================================
    std::unique_ptr<juce::MidiInput> midiInput;
    juce::String currentDeviceName;
    juce::ListenerList<Listener> listeners;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidiShortcutInputManager)
};

} // namespace minixer

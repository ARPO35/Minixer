#include "MidiShortcutInputManager.h"

namespace minixer
{

//==============================================================================
MidiShortcutInputManager::MidiShortcutInputManager() = default;

//==============================================================================
MidiShortcutInputManager::~MidiShortcutInputManager()
{
    stop();
}

//==============================================================================
juce::StringArray MidiShortcutInputManager::getAvailableDevices() const
{
    juce::StringArray names;

    for (const auto& device : juce::MidiInput::getAvailableDevices())
        names.add (device.name);

    return names;
}

//==============================================================================
bool MidiShortcutInputManager::setDevice (const juce::String& deviceName)
{
    stop();

    if (deviceName.isEmpty())
        return true;

    auto devices = juce::MidiInput::getAvailableDevices();

    for (const auto& device : devices)
    {
        if (device.name == deviceName)
        {
            auto result = juce::MidiInput::openDevice (device.identifier, this);

            if (result != nullptr)
            {
                midiInput = std::move (result);
                midiInput->start();
                currentDeviceName = deviceName;
                return true;
            }

            break;
        }
    }

    return false;
}

//==============================================================================
void MidiShortcutInputManager::stop()
{
    if (midiInput != nullptr)
    {
        midiInput->stop();
        midiInput.reset();
    }

    currentDeviceName.clear();
}

//==============================================================================
bool MidiShortcutInputManager::reconnect()
{
    if (currentDeviceName.isEmpty())
        return false;

    return setDevice (currentDeviceName);
}

//==============================================================================
void MidiShortcutInputManager::handleIncomingMidiMessage (juce::MidiInput* /*source*/,
                                                           const juce::MidiMessage& message)
{
    // 只把可能作为触发源的消息（Note On / CC）转发出去
    if (message.isNoteOn() || message.isController())
    {
        juce::MessageManager::callAsync ([this, message]()
        {
            listeners.call ([&message] (Listener& l)
            {
                l.midiShortcutMessageReceived (message);
            });
        });
    }
}

} // namespace minixer

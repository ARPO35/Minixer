#include "KeyCaptureButton.h"
#include "../Settings/AppSettings.h"

namespace minixer
{

//==============================================================================
KeyCaptureButton::KeyCaptureButton()
{
    setWantsKeyboardFocus (true);
    updateButtonText();
}

//==============================================================================
KeyCaptureButton::~KeyCaptureButton()
{
    if (isCapturing)
        stopCapturing (false);
}

//==============================================================================
void KeyCaptureButton::setInputSource (const ShortcutInputSource& source)
{
    currentSource = source;
    isCapturing = false;
    stopTimer();
    updateButtonText();
}

//==============================================================================
void KeyCaptureButton::setKeyPress (const juce::KeyPress& key)
{
    setInputSource (ShortcutInputSource (key));
}

//==============================================================================
void KeyCaptureButton::clicked()
{
    if (isCapturing)
    {
        stopCapturing (true);
    }
    else
    {
        startCapturing();
    }
}

//==============================================================================
bool KeyCaptureButton::keyPressed (const juce::KeyPress& key)
{
    if (! isCapturing)
        return juce::TextButton::keyPressed (key);

    // Escape 取消捕获
    if (key == juce::KeyPress (juce::KeyPress::escapeKey))
    {
        stopCapturing (true);
        return true;
    }

    // Backspace / Delete 清除绑定
    if (key == juce::KeyPress (juce::KeyPress::backspaceKey)
        || key == juce::KeyPress (juce::KeyPress::deleteKey))
    {
        commitSource ({});
        return true;
    }

    // 忽略单独按下修饰键或无效按键的情况
    auto keyCode = key.getKeyCode();

    if (keyCode == 0)
        return true;

    commitSource (ShortcutInputSource (key));
    return true;
}

//==============================================================================
void KeyCaptureButton::focusLost (FocusChangeType /*cause*/)
{
    if (isCapturing)
        stopCapturing (true);
}

//==============================================================================
void KeyCaptureButton::timerCallback()
{
    if (isCapturing)
        stopCapturing (true);
}

//==============================================================================
void KeyCaptureButton::midiShortcutMessageReceived (const juce::MidiMessage& message)
{
    if (! isCapturing)
        return;

    if (message.isNoteOn())
    {
        commitSource (ShortcutInputSource::midiNote (message.getChannel(),
                                                      message.getNoteNumber(),
                                                      message.getVelocity()));
    }
    else if (message.isController())
    {
        commitSource (ShortcutInputSource::midiCc (message.getChannel(),
                                                    message.getControllerNumber(),
                                                    message.getControllerValue()));
    }
}

//==============================================================================
void KeyCaptureButton::hidShortcutEventReceived (const HidShortcutEvent& event)
{
    if (! isCapturing || ! event.isPressed)
        return;

    commitSource (ShortcutInputSource::hidButton (event.vendorId, event.productId,
                                                   event.usagePage, event.usage,
                                                   event.controlId));
}

//==============================================================================
void KeyCaptureButton::startCapturing()
{
    isCapturing = true;
    grabKeyboardFocus();
    updateButtonText();
    startTimer (captureTimeoutMs);

    auto& midi = AppSettings::getInstance().getMidiShortcutInputManager();
    auto& hid  = AppSettings::getInstance().getHidShortcutInputManager();

    midi.addListener (this);
    hid.addListener (this);
}

//==============================================================================
void KeyCaptureButton::stopCapturing (bool notify)
{
    isCapturing = false;
    stopTimer();
    updateButtonText();

    auto& midi = AppSettings::getInstance().getMidiShortcutInputManager();
    auto& hid  = AppSettings::getInstance().getHidShortcutInputManager();

    midi.removeListener (this);
    hid.removeListener (this);

    if (notify)
    {
        listeners.call ([this] (Listener& l) { l.inputSourceCaptureChanged (this, currentSource); });
        listeners.call ([this] (Listener& l) { l.keyCaptureChanged (this, currentSource.getKeyPress()); });
    }
}

//==============================================================================
void KeyCaptureButton::commitSource (const ShortcutInputSource& source)
{
    currentSource = source;
    stopCapturing (true);
}

//==============================================================================
void KeyCaptureButton::updateButtonText()
{
    if (isCapturing)
    {
        setButtonText (TRANS ("Press a key/MIDI/HID..."));
        return;
    }

    setButtonText (inputSourceToDisplayString (currentSource));
}

//==============================================================================
juce::String KeyCaptureButton::inputSourceToDisplayString (const ShortcutInputSource& source) const
{
    if (! source.isValid())
        return TRANS ("None");

    return source.getDisplayString();
}

} // namespace minixer

#include "ShortcutInputSource.h"

namespace minixer
{

//==============================================================================
ShortcutInputSource::ShortcutInputSource (const juce::KeyPress& key)
    : type (ShortcutInputType::keyPress), keyPress (key)
{
}

//==============================================================================
ShortcutInputSource ShortcutInputSource::midiNote (int channel, int noteNumber, int velocityThreshold)
{
    ShortcutInputSource s;
    s.type       = ShortcutInputType::midiNote;
    s.midiChannel = juce::jlimit (0, 16, channel);
    s.midiNumber  = juce::jlimit (0, 127, noteNumber);
    s.midiValue   = juce::jlimit (0, 127, velocityThreshold);
    return s;
}

//==============================================================================
ShortcutInputSource ShortcutInputSource::midiCc (int channel, int controllerNumber, int valueThreshold)
{
    ShortcutInputSource s;
    s.type       = ShortcutInputType::midiCc;
    s.midiChannel = juce::jlimit (0, 16, channel);
    s.midiNumber  = juce::jlimit (0, 127, controllerNumber);
    s.midiValue   = juce::jlimit (0, 127, valueThreshold);
    return s;
}

//==============================================================================
ShortcutInputSource ShortcutInputSource::hidButton (uint16_t vendorId, uint16_t productId,
                                                    uint16_t usagePage, uint16_t usage,
                                                    uint32_t buttonId)
{
    ShortcutInputSource s;
    s.type          = ShortcutInputType::hidButton;
    s.hidVendorId   = vendorId;
    s.hidProductId  = productId;
    s.hidUsagePage  = usagePage;
    s.hidUsage      = usage;
    s.hidButtonId   = buttonId;
    return s;
}

//==============================================================================
bool ShortcutInputSource::isValid() const noexcept
{
    switch (type)
    {
        case ShortcutInputType::keyPress:  return keyPress.isValid();
        case ShortcutInputType::midiNote:
        case ShortcutInputType::midiCc:    return midiChannel >= 0 && midiChannel <= 16 && midiNumber >= 0 && midiNumber <= 127;
        case ShortcutInputType::hidButton: return hidButtonId != 0;
        default:                           return false;
    }
}

//==============================================================================
bool ShortcutInputSource::matches (const ShortcutInputSource& other) const noexcept
{
    if (type != other.type)
        return false;

    switch (type)
    {
        case ShortcutInputType::keyPress:  return keyPress == other.keyPress;
        case ShortcutInputType::midiNote:
        case ShortcutInputType::midiCc:    return midiChannel == other.midiChannel
                                                   && midiNumber == other.midiNumber
                                                   && midiValue == other.midiValue;
        case ShortcutInputType::hidButton: return hidVendorId  == other.hidVendorId
                                                   && hidProductId == other.hidProductId
                                                   && hidUsagePage == other.hidUsagePage
                                                   && hidUsage     == other.hidUsage
                                                   && hidButtonId  == other.hidButtonId;
        default:                           return false;
    }
}

//==============================================================================
bool ShortcutInputSource::matchesMidiMessage (const juce::MidiMessage& message) const noexcept
{
    if (type == ShortcutInputType::midiNote && message.isNoteOn())
    {
        const int channel = message.getChannel();
        const int note    = message.getNoteNumber();
        const int vel     = message.getVelocity();

        if (midiChannel != 0 && midiChannel != channel)
            return false;
        if (midiNumber != note)
            return false;
        if (midiValue > 0 && vel < midiValue)
            return false;

        return true;
    }

    if (type == ShortcutInputType::midiCc && message.isController())
    {
        const int channel = message.getChannel();
        const int cc      = message.getControllerNumber();
        const int val     = message.getControllerValue();

        if (midiChannel != 0 && midiChannel != channel)
            return false;
        if (midiNumber != cc)
            return false;
        if (midiValue > 0 && val < midiValue)
            return false;

        return true;
    }

    return false;
}

//==============================================================================
juce::String ShortcutInputSource::getDisplayString() const
{
    switch (type)
    {
        case ShortcutInputType::keyPress:
            return keyPress.getTextDescription();

        case ShortcutInputType::midiNote:
        {
            auto text = juce::String ("MIDI Note ") + juce::String (midiNumber)
                      + " (Ch" + juce::String (midiChannel == 0 ? juce::String ("Any") : juce::String (midiChannel)) + ")";
            if (midiValue > 0)
                text += " >= " + juce::String (midiValue);
            return text;
        }

        case ShortcutInputType::midiCc:
        {
            auto text = juce::String ("MIDI CC ") + juce::String (midiNumber)
                      + " (Ch" + juce::String (midiChannel == 0 ? juce::String ("Any") : juce::String (midiChannel)) + ")";
            if (midiValue > 0)
                text += " >= " + juce::String (midiValue);
            return text;
        }

        case ShortcutInputType::hidButton:
        {
            auto text = juce::String ("HID ") + juce::String (hidButtonId);
            if (hidVendorId != 0 || hidProductId != 0)
                text += juce::String::formatted (" [VID:%04X PID:%04X]", hidVendorId, hidProductId);
            return text;
        }

        default:
            return TRANS ("None");
    }
}

//==============================================================================
void ShortcutInputSource::writeToXml (juce::XmlElement& xml) const
{
    xml.setAttribute ("inputType", typeToString (type));

    if (type == ShortcutInputType::keyPress)
    {
        xml.setAttribute ("keyCode",        keyPress.getKeyCode());
        xml.setAttribute ("modifiers",      static_cast<int> (keyPress.getModifiers().getRawFlags()));
        xml.setAttribute ("textCharacter",  juce::String::charToString (keyPress.getTextCharacter()));
    }
    else if (isMidi())
    {
        xml.setAttribute ("midiChannel", midiChannel);
        xml.setAttribute ("midiNumber",  midiNumber);
        xml.setAttribute ("midiValue",   midiValue);
    }
    else if (type == ShortcutInputType::hidButton)
    {
        xml.setAttribute ("hidVendorId",  static_cast<int> (hidVendorId));
        xml.setAttribute ("hidProductId", static_cast<int> (hidProductId));
        xml.setAttribute ("hidUsagePage", static_cast<int> (hidUsagePage));
        xml.setAttribute ("hidUsage",     static_cast<int> (hidUsage));
        xml.setAttribute ("hidButtonId",  static_cast<int> (hidButtonId));
    }
}

//==============================================================================
bool ShortcutInputSource::readFromXml (const juce::XmlElement& xml)
{
    if (! xml.hasAttribute ("inputType"))
        return false;

    auto newType = stringToType (xml.getStringAttribute ("inputType"));

    switch (newType)
    {
        case ShortcutInputType::keyPress:
        {
            auto keyCode   = xml.getIntAttribute ("keyCode");
            auto modifiers = juce::ModifierKeys (xml.getIntAttribute ("modifiers", 0));
            auto textChar  = xml.getStringAttribute ("textCharacter")[0];

            *this = ShortcutInputSource (juce::KeyPress (keyCode, modifiers, textChar));
            return true;
        }

        case ShortcutInputType::midiNote:
        case ShortcutInputType::midiCc:
        {
            ShortcutInputSource s;
            s.type        = newType;
            s.midiChannel = xml.getIntAttribute ("midiChannel", 0);
            s.midiNumber  = xml.getIntAttribute ("midiNumber", 0);
            s.midiValue   = xml.getIntAttribute ("midiValue", 0);
            *this = s;
            return true;
        }

        case ShortcutInputType::hidButton:
        {
            ShortcutInputSource s;
            s.type         = ShortcutInputType::hidButton;
            s.hidVendorId  = static_cast<uint16_t> (xml.getIntAttribute ("hidVendorId", 0));
            s.hidProductId = static_cast<uint16_t> (xml.getIntAttribute ("hidProductId", 0));
            s.hidUsagePage = static_cast<uint16_t> (xml.getIntAttribute ("hidUsagePage", 0));
            s.hidUsage     = static_cast<uint16_t> (xml.getIntAttribute ("hidUsage", 0));
            s.hidButtonId  = static_cast<uint32_t> (xml.getIntAttribute ("hidButtonId", 0));
            *this = s;
            return true;
        }

        default:
        {
            *this = {};
            return true;
        }
    }
}

//==============================================================================
bool ShortcutInputSource::readLegacyKeyboardFromXml (const juce::XmlElement& xml)
{
    if (! xml.hasAttribute ("keyCode"))
        return false;

    auto keyCode   = xml.getIntAttribute ("keyCode");
    auto modifiers = juce::ModifierKeys (xml.getIntAttribute ("modifiers", 0));
    auto textChar  = xml.getStringAttribute ("textCharacter")[0];

    *this = ShortcutInputSource (juce::KeyPress (keyCode, modifiers, textChar));
    return true;
}

//==============================================================================
juce::String ShortcutInputSource::typeToString (ShortcutInputType t)
{
    switch (t)
    {
        case ShortcutInputType::none:       return "none";
        case ShortcutInputType::keyPress:   return "keyPress";
        case ShortcutInputType::midiNote:   return "midiNote";
        case ShortcutInputType::midiCc:     return "midiCc";
        case ShortcutInputType::hidButton:  return "hidButton";
    }

    return "none";
}

//==============================================================================
ShortcutInputType ShortcutInputSource::stringToType (const juce::String& text)
{
    if (text.equalsIgnoreCase ("keyPress"))  return ShortcutInputType::keyPress;
    if (text.equalsIgnoreCase ("midiNote"))  return ShortcutInputType::midiNote;
    if (text.equalsIgnoreCase ("midiCc"))    return ShortcutInputType::midiCc;
    if (text.equalsIgnoreCase ("hidButton")) return ShortcutInputType::hidButton;

    return ShortcutInputType::none;
}

} // namespace minixer

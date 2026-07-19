#include "RotaryKnobComponent.h"
#include "../LookAndFeel/MixerLookAndFeel.h"

namespace minixer
{

namespace
{
    static constexpr const char* clipboardPrefix = "MinixerParam:";
}

//==============================================================================
RotaryKnobComponent::RotaryKnobComponent (const juce::String& name,
                                          double minValue,
                                          double maxValue,
                                          double defaultVal,
                                          double interval,
                                          const juce::String& suffixToUse)
    : suffix (suffixToUse), paramName (name), defaultValue (defaultVal)
{
    slider.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    slider.setScrollWheelEnabled (false);
    slider.setRange (minValue, maxValue, interval);
    slider.setValue (defaultValue, juce::dontSendNotification);
    slider.setDoubleClickReturnValue (true, defaultValue);
    slider.addMouseListener (this, true);
    addAndMakeVisible (slider);

    nameLabel.setText (paramName, juce::dontSendNotification);
    nameLabel.setJustificationType (juce::Justification::centred);
    nameLabel.setFont (juce::Font (juce::FontOptions (11.0f)));
    nameLabel.setColour (juce::Label::textColourId, MixerLookAndFeel::getMutedTextColour());
    addAndMakeVisible (nameLabel);

    valueLabel.setJustificationType (juce::Justification::centred);
    valueLabel.setFont (juce::Font (juce::FontOptions (11.0f)));
    valueLabel.setColour (juce::Label::textColourId, MixerLookAndFeel::getTextColour());
    addAndMakeVisible (valueLabel);

    updateValueLabel();

    slider.onValueChange = [this]
    {
        updateValueLabel();
        listeners.call ([this] (Listener& l) { l.rotaryKnobValueChanged (this); });
    };
}

//==============================================================================
void RotaryKnobComponent::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::transparentBlack);
}

//==============================================================================
void RotaryKnobComponent::resized()
{
    auto bounds = getLocalBounds();
    auto bottomArea = bounds.removeFromBottom (26);

    nameLabel.setBounds (bottomArea.removeFromTop (13));
    valueLabel.setBounds (bottomArea);
    nameLabel.setFont (juce::Font (juce::FontOptions (10.0f)));
    valueLabel.setFont (juce::Font (juce::FontOptions (10.0f)));

    slider.setBounds (bounds.reduced (1));
}

//==============================================================================
void RotaryKnobComponent::mouseDown (const juce::MouseEvent& event)
{
    if (event.mods.isPopupMenu())
    {
        showContextMenu (event.getPosition());
        return;
    }

    Component::mouseDown (event);
}

//==============================================================================
void RotaryKnobComponent::setValueSuffix (const juce::String& newSuffix)
{
    suffix = newSuffix;
    updateValueLabel();
}

//==============================================================================
void RotaryKnobComponent::setValueFormatter (ValueFormatter formatter)
{
    valueFormatter = std::move (formatter);
    updateValueLabel();
}

//==============================================================================
void RotaryKnobComponent::resetToDefault()
{
    slider.setValue (defaultValue, juce::sendNotification);
}

//==============================================================================
void RotaryKnobComponent::updateValueLabel()
{
    valueLabel.setText (getDisplayValueText(), juce::dontSendNotification);
}

//==============================================================================
void RotaryKnobComponent::showContextMenu (juce::Point<int> clickPos)
{
    juce::PopupMenu menu;
    menu.addItem (1, TRANS ("Reset to default") + " (" + getDisplayValueText() + ")", true, false);
    menu.addSeparator();
    menu.addItem (2, TRANS ("Copy value"), true, false);
    menu.addItem (3, TRANS ("Paste value"), true, false);

    auto screenPos = localPointToGlobal (clickPos);
    auto targetArea = juce::Rectangle<int> (screenPos.x, screenPos.y, 1, 1);

    menu.showMenuAsync (juce::PopupMenu::Options().withTargetScreenArea (targetArea)
                                                              .withMinimumWidth (140),
                        [this] (int result)
    {
        switch (result)
        {
            case 1:
                resetToDefault();
                break;
            case 2:
                copyValueToClipboard();
                break;
            case 3:
                pasteValueFromClipboard();
                break;
            default:
                break;
        }
    });
}

//==============================================================================
void RotaryKnobComponent::copyValueToClipboard() const
{
    juce::SystemClipboard::copyTextToClipboard (clipboardPrefix + paramName + ":" + juce::String (slider.getValue(), 6));
}

//==============================================================================
void RotaryKnobComponent::pasteValueFromClipboard()
{
    auto text = juce::SystemClipboard::getTextFromClipboard();
    auto token = clipboardPrefix + paramName + ":";

    if (text.startsWith (token))
    {
        auto valueText = text.substring (token.length());
        auto value = valueText.getDoubleValue();

        if (value >= slider.getMinimum() && value <= slider.getMaximum())
            slider.setValue (value, juce::sendNotification);
    }
}

//==============================================================================
juce::String RotaryKnobComponent::getDisplayValueText() const
{
    if (valueFormatter != nullptr)
        return valueFormatter (slider.getValue());

    auto text = juce::String (slider.getValue(), 1);

    if (suffix.isNotEmpty())
        text << " " << suffix;

    return text;
}

} // namespace minixer

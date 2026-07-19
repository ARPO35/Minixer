#include "FaderComponent.h"
#include "../LookAndFeel/MixerLookAndFeel.h"

namespace minixer
{

namespace
{
    static constexpr const char* clipboardPrefix = "MinixerParam:";
}

//==============================================================================
FaderComponent::FaderComponent (const juce::String& name,
                                double minValueDb,
                                double maxValueDb,
                                double defaultValueDb)
    : paramName (name), defaultValue (defaultValueDb)
{
    slider.setSliderStyle (juce::Slider::LinearVertical);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    slider.setScrollWheelEnabled (false);
    slider.setRange (minValueDb, maxValueDb, 0.1);
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
    valueLabel.setFont (juce::Font (juce::FontOptions (11.0f)).boldened());
    valueLabel.setColour (juce::Label::textColourId, MixerLookAndFeel::getTextColour());
    addAndMakeVisible (valueLabel);

    updateValueLabel();

    slider.onValueChange = [this]
    {
        updateValueLabel();
        listeners.call ([this] (Listener& l) { l.faderValueChanged (this); });
    };
}

//==============================================================================
void FaderComponent::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::transparentBlack);
}

//==============================================================================
void FaderComponent::resized()
{
    auto bounds = getLocalBounds();
    auto topArea = bounds.removeFromTop (14);
    auto bottomArea = bounds.removeFromBottom (14);

    nameLabel.setBounds (topArea);
    valueLabel.setBounds (bottomArea);
    nameLabel.setFont (juce::Font (juce::FontOptions (10.0f)));
    valueLabel.setFont (juce::Font (juce::FontOptions (10.0f)).boldened());

    slider.setBounds (bounds.reduced (4, 0));
}

//==============================================================================
void FaderComponent::mouseDown (const juce::MouseEvent& event)
{
    if (event.mods.isPopupMenu())
    {
        showContextMenu (event.getPosition());
        return;
    }

    Component::mouseDown (event);
}

//==============================================================================
void FaderComponent::resetToDefault()
{
    slider.setValue (defaultValue, juce::sendNotification);
}

//==============================================================================
void FaderComponent::updateValueLabel()
{
    valueLabel.setText (valueToDbString (slider.getValue()), juce::dontSendNotification);
}

//==============================================================================
void FaderComponent::showContextMenu (juce::Point<int> clickPos)
{
    juce::PopupMenu menu;
    menu.addItem (1, TRANS ("Reset to default") + " (" + valueToDbString (defaultValue) + ")", true, false);
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
void FaderComponent::copyValueToClipboard() const
{
    juce::SystemClipboard::copyTextToClipboard (clipboardPrefix + paramName + ":" + juce::String (slider.getValue(), 6));
}

//==============================================================================
void FaderComponent::pasteValueFromClipboard()
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
juce::String FaderComponent::valueToDbString (double valueDb)
{
    if (valueDb <= -59.9)
        return "-inf dB";

    return juce::String (valueDb, 1) + " dB";
}

} // namespace minixer

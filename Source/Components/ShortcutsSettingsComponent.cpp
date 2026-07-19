#include "ShortcutsSettingsComponent.h"
#include "../LookAndFeel/MixerLookAndFeel.h"
#include "../Settings/AppSettings.h"

namespace minixer
{

//==============================================================================
ShortcutsSettingsComponent::ShortcutsSettingsComponent()
{
    setSize (560, 960);

    workingSettings = AppSettings::getInstance().getShortcutSettings();

    setupSectionLabel (midiDeviceSectionLabel, TRANS ("MIDI Shortcut Input"));
    setupLabel (midiDeviceDescriptionLabel, TRANS ("Select a MIDI input device to use its notes/CC as slot shortcuts."));
    setupMidiDeviceComboBox (midiDeviceComboBox);
    midiDeviceComboBox.addListener (this);

    setupSectionLabel (globalSectionLabel, TRANS ("Global Shortcuts"));
    setupLabel (globalDescriptionLabel, TRANS ("These shortcuts work across the entire Minixer window."));

    for (size_t i = 0; i < globalRows.size(); ++i)
    {
        auto action = static_cast<GlobalShortcutAction> (i);
        auto& row = globalRows[i];

        setupLabel (row.nameLabel, ShortcutSettings::getGlobalShortcutActionName (action));
        setupKeyCaptureButton (row.captureButton);
        setupLabel (row.descLabel, ShortcutSettings::getGlobalShortcutActionDescription (action));
        row.descLabel.setFont (juce::Font (juce::FontOptions (11.0f)));
        row.descLabel.setColour (juce::Label::textColourId, MixerLookAndFeel::getMutedTextColour());
        row.captureButton.addListener (this);
    }

    setupSectionLabel (slotSectionLabel, TRANS ("Slot Bypass Shortcuts"));
    setupLabel (slotDescriptionLabel, TRANS ("Each slot can have its own bypass shortcut. Multiple slots may share the same source."));

    for (int i = 0; i < static_cast<int> (slotRows.size()); ++i)
    {
        auto& row = slotRows[static_cast<size_t> (i)];

        setupLabel (row.slotLabel, TRANS ("Slot ") + juce::String (i + 1));
        setupKeyCaptureButton (row.captureButton);
        setupClearButton (row.clearButton);
        setupToggleButton (row.defaultBypassToggle, TRANS ("Default bypass"));
        setupModeComboBox (row.modeComboBox);

        row.captureButton.addListener (this);
        row.clearButton.addListener (this);
        row.defaultBypassToggle.addListener (this);
        row.modeComboBox.addListener (this);
    }

    resetDefaultsButton.setLookAndFeel (&getLookAndFeel());
    resetDefaultsButton.setColour (juce::TextButton::buttonColourId, MixerLookAndFeel::getSurfaceColour());
    resetDefaultsButton.setColour (juce::TextButton::textColourOffId, MixerLookAndFeel::getTextColour());
    resetDefaultsButton.addListener (this);
    addAndMakeVisible (resetDefaultsButton);

    applyButton.setLookAndFeel (&getLookAndFeel());
    applyButton.setColour (juce::TextButton::buttonColourId, MixerLookAndFeel::getAccentColour());
    applyButton.setColour (juce::TextButton::textColourOffId, juce::Colours::white);
    applyButton.addListener (this);
    addAndMakeVisible (applyButton);

    cancelButton.setLookAndFeel (&getLookAndFeel());
    cancelButton.setColour (juce::TextButton::buttonColourId, MixerLookAndFeel::getSurfaceColour());
    cancelButton.setColour (juce::TextButton::textColourOffId, MixerLookAndFeel::getTextColour());
    cancelButton.addListener (this);
    addAndMakeVisible (cancelButton);

    loadSettings();
}

//==============================================================================
void ShortcutsSettingsComponent::paint (juce::Graphics& g)
{
    g.fillAll (MixerLookAndFeel::getBackgroundColour());
}

//==============================================================================
void ShortcutsSettingsComponent::resized()
{
    auto bounds = getLocalBounds().reduced (16, 16);
    const auto gap = 8;
    const auto rowHeight = 28;

    auto layoutRow = [&bounds, rowHeight, gap] (juce::Component& comp)
    {
        comp.setBounds (bounds.removeFromTop (rowHeight));
        bounds.removeFromTop (gap);
    };

    layoutRow (midiDeviceSectionLabel);
    layoutRow (midiDeviceDescriptionLabel);
    layoutRow (midiDeviceComboBox);

    bounds.removeFromTop (12);
    layoutRow (globalSectionLabel);
    layoutRow (globalDescriptionLabel);

    const auto nameWidth = 150;
    const auto captureWidth = 160;

    for (auto& row : globalRows)
    {
        auto rowBounds = bounds.removeFromTop (rowHeight);
        row.nameLabel.setBounds (rowBounds.removeFromLeft (nameWidth));
        rowBounds.removeFromLeft (gap);
        row.captureButton.setBounds (rowBounds.removeFromLeft (captureWidth));
        rowBounds.removeFromLeft (gap);
        row.descLabel.setBounds (rowBounds);
        bounds.removeFromTop (gap);
    }

    bounds.removeFromTop (12);
    layoutRow (slotSectionLabel);
    layoutRow (slotDescriptionLabel);

    const auto slotLabelWidth = 60;
    const auto slotCaptureWidth = 170;
    const auto clearWidth = 56;
    const auto modeWidth = 100;

    for (auto& row : slotRows)
    {
        auto rowBounds = bounds.removeFromTop (rowHeight);
        row.slotLabel.setBounds (rowBounds.removeFromLeft (slotLabelWidth));
        rowBounds.removeFromLeft (gap);
        row.captureButton.setBounds (rowBounds.removeFromLeft (slotCaptureWidth));
        rowBounds.removeFromLeft (gap);
        row.clearButton.setBounds (rowBounds.removeFromLeft (clearWidth));
        rowBounds.removeFromLeft (gap);
        row.defaultBypassToggle.setBounds (rowBounds.removeFromLeft (modeWidth));
        rowBounds.removeFromLeft (gap);
        row.modeComboBox.setBounds (rowBounds.removeFromLeft (modeWidth));
        bounds.removeFromTop (gap);
    }

    bounds.removeFromTop (12);
    layoutRow (resetDefaultsButton);

    // Apply / Cancel 按钮并排放置在底部
    auto buttonRow = bounds.removeFromTop (rowHeight);
    const auto buttonWidth = juce::jmin (120, (buttonRow.getWidth() - gap) / 2);
    applyButton.setBounds (buttonRow.removeFromRight (buttonWidth));
    buttonRow.removeFromRight (gap);
    cancelButton.setBounds (buttonRow.removeFromRight (buttonWidth));
}

//==============================================================================
void ShortcutsSettingsComponent::visibilityChanged()
{
    if (isVisible())
    {
        // 每次显示窗口时从 AppSettings 重新加载，确保 Cancel 后丢弃未应用更改
        workingSettings = AppSettings::getInstance().getShortcutSettings();
        loadSettings();
    }
}

//==============================================================================
void ShortcutsSettingsComponent::inputSourceCaptureChanged (KeyCaptureButton* button,
                                                             const ShortcutInputSource& /*newSource*/)
{
    if (updatingUI)
        return;

    for (size_t i = 0; i < globalRows.size(); ++i)
    {
        if (button == &globalRows[i].captureButton)
        {
            workingSettings.setGlobalShortcut (static_cast<GlobalShortcutAction> (i), button->getKeyPress());
            return;
        }
    }

    for (int i = 0; i < static_cast<int> (slotRows.size()); ++i)
    {
        if (button == &slotRows[static_cast<size_t> (i)].captureButton)
        {
            updateSlotShortcutFromUI (i);
            return;
        }
    }
}

//==============================================================================
void ShortcutsSettingsComponent::buttonClicked (juce::Button* button)
{
    if (updatingUI)
        return;

    if (button == &applyButton)
    {
        applyMidiDeviceSelection();
        applySettings();
        return;
    }

    if (button == &cancelButton)
    {
        cancelSettings();
        return;
    }

    if (button == &resetDefaultsButton)
    {
        resetToDefaults();
        return;
    }

    for (int i = 0; i < static_cast<int> (slotRows.size()); ++i)
    {
        if (button == &slotRows[static_cast<size_t> (i)].clearButton)
        {
            slotRows[static_cast<size_t> (i)].captureButton.setInputSource ({});
            updateSlotShortcutFromUI (i);
            return;
        }
    }

    for (int i = 0; i < static_cast<int> (slotRows.size()); ++i)
    {
        if (button == &slotRows[static_cast<size_t> (i)].defaultBypassToggle)
        {
            updateSlotShortcutFromUI (i);
            return;
        }
    }
}

//==============================================================================
void ShortcutsSettingsComponent::comboBoxChanged (juce::ComboBox* comboBox)
{
    if (updatingUI)
        return;

    if (comboBox == &midiDeviceComboBox)
        return; // 直到 Apply 才生效

    for (int i = 0; i < static_cast<int> (slotRows.size()); ++i)
    {
        if (comboBox == &slotRows[static_cast<size_t> (i)].modeComboBox)
        {
            updateSlotShortcutFromUI (i);
            return;
        }
    }
}

//==============================================================================
void ShortcutsSettingsComponent::cancelSettings()
{
    // 丢弃本地修改，恢复到 AppSettings 中的值
    workingSettings = AppSettings::getInstance().getShortcutSettings();
    loadSettings();

    listeners.call ([] (Listener& l) { l.shortcutsSettingsCancelled(); });
}

//==============================================================================
void ShortcutsSettingsComponent::setupSectionLabel (juce::Label& label, const juce::String& text)
{
    label.setText (text, juce::dontSendNotification);
    label.setFont (juce::Font (juce::FontOptions (14.0f)).boldened());
    label.setColour (juce::Label::textColourId, MixerLookAndFeel::getAccentColour());
    label.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (label);
}

//==============================================================================
void ShortcutsSettingsComponent::setupLabel (juce::Label& label, const juce::String& text)
{
    label.setText (text, juce::dontSendNotification);
    label.setFont (juce::Font (juce::FontOptions (12.0f)));
    label.setColour (juce::Label::textColourId, MixerLookAndFeel::getTextColour());
    label.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (label);
}

//==============================================================================
void ShortcutsSettingsComponent::setupToggleButton (juce::ToggleButton& button, const juce::String& text)
{
    button.setButtonText (text);
    button.setLookAndFeel (&getLookAndFeel());
    button.setColour (juce::ToggleButton::textColourId, MixerLookAndFeel::getTextColour());
    addAndMakeVisible (button);
}

//==============================================================================
void ShortcutsSettingsComponent::setupKeyCaptureButton (KeyCaptureButton& button)
{
    button.setLookAndFeel (&getLookAndFeel());
    button.setColour (juce::TextButton::buttonColourId, MixerLookAndFeel::getSurfaceColour());
    button.setColour (juce::TextButton::textColourOffId, MixerLookAndFeel::getTextColour());
    addAndMakeVisible (button);
}

//==============================================================================
void ShortcutsSettingsComponent::setupClearButton (juce::TextButton& button)
{
    button.setLookAndFeel (&getLookAndFeel());
    button.setColour (juce::TextButton::buttonColourId, MixerLookAndFeel::getSurfaceColour());
    button.setColour (juce::TextButton::textColourOffId, MixerLookAndFeel::getTextColour());
    addAndMakeVisible (button);
}

//==============================================================================
void ShortcutsSettingsComponent::setupModeComboBox (juce::ComboBox& comboBox)
{
    comboBox.setLookAndFeel (&getLookAndFeel());
    comboBox.addItem (TRANS ("Cycle toggle"), 1);
    comboBox.addItem (TRANS ("Hold toggle"), 2);
    comboBox.setSelectedId (1, juce::dontSendNotification);
    addAndMakeVisible (comboBox);
}

//==============================================================================
void ShortcutsSettingsComponent::setupMidiDeviceComboBox (juce::ComboBox& comboBox)
{
    comboBox.setLookAndFeel (&getLookAndFeel());
    addAndMakeVisible (comboBox);
    refreshMidiDeviceList();
}

//==============================================================================
void ShortcutsSettingsComponent::refreshMidiDeviceList()
{
    auto& manager = AppSettings::getInstance().getMidiShortcutInputManager();
    auto devices  = manager.getAvailableDevices();
    auto current  = manager.getCurrentDeviceName();

    midiDeviceComboBox.clear (juce::dontSendNotification);
    midiDeviceComboBox.addItem (TRANS ("None (disable MIDI shortcuts)"), 1);

    int selectedId = current.isEmpty() ? 1 : 0;
    int id = 2;

    for (const auto& name : devices)
    {
        midiDeviceComboBox.addItem (name, id);

        if (name == current)
            selectedId = id;

        ++id;
    }

    midiDeviceComboBox.setSelectedId (selectedId > 0 ? selectedId : 1, juce::dontSendNotification);
}

//==============================================================================
void ShortcutsSettingsComponent::applyMidiDeviceSelection()
{
    auto selectedName = midiDeviceComboBox.getText();

    if (midiDeviceComboBox.getSelectedId() == 1)
        selectedName.clear();

    AppSettings::getInstance().setMidiShortcutDeviceName (selectedName);
}

//==============================================================================
void ShortcutsSettingsComponent::loadSettings()
{
    updatingUI = true;

    refreshMidiDeviceList();

    for (size_t i = 0; i < globalRows.size(); ++i)
        globalRows[i].captureButton.setKeyPress (workingSettings.getGlobalShortcut (static_cast<GlobalShortcutAction> (i)));

    for (int i = 0; i < static_cast<int> (slotRows.size()); ++i)
        updateUIFromSlotShortcut (i);

    updatingUI = false;
}

//==============================================================================
void ShortcutsSettingsComponent::applySettings()
{
    AppSettings::getInstance().getShortcutSettings() = workingSettings;
    AppSettings::getInstance().saveShortcutSettings();

    listeners.call ([] (Listener& l) { l.shortcutsSettingsApplied(); });
}

//==============================================================================
void ShortcutsSettingsComponent::resetToDefaults()
{
    workingSettings.resetToDefaults();
    loadSettings();
}

//==============================================================================
void ShortcutsSettingsComponent::updateSlotShortcutFromUI (int slotIndex)
{
    if (! juce::isPositiveAndBelow (slotIndex, static_cast<int> (slotRows.size())))
        return;

    auto& row = slotRows[static_cast<size_t> (slotIndex)];
    SlotShortcut shortcut;

    shortcut.inputSource    = row.captureButton.getInputSource();
    shortcut.defaultBypassed = row.defaultBypassToggle.getToggleState();
    shortcut.mode            = row.modeComboBox.getSelectedId() == 2 ? SlotShortcutMode::holdToggle
                                                                     : SlotShortcutMode::cycleToggle;

    workingSettings.setSlotShortcut (slotIndex, shortcut);
}

//==============================================================================
void ShortcutsSettingsComponent::updateUIFromSlotShortcut (int slotIndex)
{
    if (! juce::isPositiveAndBelow (slotIndex, static_cast<int> (slotRows.size())))
        return;

    auto& shortcut = workingSettings.getSlotShortcut (slotIndex);
    auto& row = slotRows[static_cast<size_t> (slotIndex)];

    row.captureButton.setInputSource (shortcut.inputSource);
    row.defaultBypassToggle.setToggleState (shortcut.defaultBypassed, juce::dontSendNotification);
    row.modeComboBox.setSelectedId (shortcut.mode == SlotShortcutMode::holdToggle ? 2 : 1,
                                    juce::dontSendNotification);
}

} // namespace minixer

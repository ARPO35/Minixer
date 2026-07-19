#include "PresetBarComponent.h"
#include "../LookAndFeel/MixerLookAndFeel.h"

namespace minixer
{

//==============================================================================
PresetBarComponent::PresetBarComponent()
    : lastPresetDirectory (AppSettings::getInstance().getPresetsDirectory())
{
    presetNameLabel.setFont (juce::Font (juce::FontOptions (13.0f)).boldened());
    presetNameLabel.setColour (juce::Label::textColourId, MixerLookAndFeel::getTextColour());
    presetNameLabel.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (presetNameLabel);

    auto setupButton = [this] (juce::TextButton& button)
    {
        button.setLookAndFeel (&getLookAndFeel());
        button.setColour (juce::TextButton::buttonColourId, MixerLookAndFeel::getElevatedColour());
        button.setColour (juce::TextButton::textColourOffId, MixerLookAndFeel::getTextColour());
        button.addListener (this);
        addAndMakeVisible (button);
    };

    setupButton (saveButton);
    setupButton (loadButton);
    setupButton (renameButton);
    setupButton (deleteButton);
    setupButton (settingsButton);
}

//==============================================================================
void PresetBarComponent::setCurrentPresetName (const juce::String& presetName,
                                               const juce::File& sourceFile)
{
    currentPresetName = presetName;
    currentPresetFile = sourceFile;
    pendingPresetName.clear();

    if (sourceFile.existsAsFile())
        lastPresetDirectory = sourceFile.getParentDirectory();

    updatePresetNameLabel();
}

//==============================================================================
void PresetBarComponent::clearCurrentPreset()
{
    setCurrentPresetName ({}, {});
}

//==============================================================================
void PresetBarComponent::updatePresetNameLabel()
{
    if (pendingPresetName.isNotEmpty())
    {
        presetNameLabel.setText (pendingPresetName, juce::dontSendNotification);
        return;
    }

    if (currentPresetName.isNotEmpty())
    {
        presetNameLabel.setText (currentPresetName, juce::dontSendNotification);
        return;
    }

    presetNameLabel.setText (TRANS("--"), juce::dontSendNotification);
}

//==============================================================================
void PresetBarComponent::paint (juce::Graphics& g)
{
    g.fillAll (MixerLookAndFeel::getSurfaceColour());

    g.setColour (MixerLookAndFeel::getBorderColour());
    g.drawHorizontalLine (getHeight() - 1, 0.0f, static_cast<float> (getWidth()));
}

//==============================================================================
void PresetBarComponent::resized()
{
    auto bounds = getLocalBounds().reduced (4, 4);
    const auto rowHeight = bounds.getHeight() / 3;

    // 第 1 行：当前 preset 名称
    presetNameLabel.setBounds (bounds.removeFromTop (rowHeight));

    // 第 2 行：Save + Load
    auto row2 = bounds.removeFromTop (rowHeight);
    auto buttonWidth = (row2.getWidth() - 4) / 2;
    saveButton.setBounds (row2.removeFromLeft (buttonWidth));
    row2.removeFromLeft (4);
    loadButton.setBounds (row2);

    // 第 3 行：Rename + Delete + Settings
    auto row3 = bounds.removeFromTop (rowHeight);
    auto buttonWidth3 = (row3.getWidth() - 8) / 3;
    renameButton.setBounds (row3.removeFromLeft (buttonWidth3));
    row3.removeFromLeft (4);
    deleteButton.setBounds (row3.removeFromLeft (buttonWidth3));
    row3.removeFromLeft (4);
    settingsButton.setBounds (row3);
}

//==============================================================================
void PresetBarComponent::buttonClicked (juce::Button* button)
{
    if (button == &saveButton)
    {
        launchSavePresetDialog();
    }
    else if (button == &loadButton)
    {
        launchLoadPresetDialog();
    }
    else if (button == &renameButton)
    {
        auto baseName = pendingPresetName.isNotEmpty() ? pendingPresetName
                                                       : currentPresetName;

        auto* alert = new juce::AlertWindow (TRANS("Rename Preset"), TRANS("Enter new name:"),
                                             juce::AlertWindow::NoIcon, this);
        alert->addTextEditor ("name", baseName.isEmpty() ? TRANS("New Preset") : baseName);
        alert->addButton (TRANS("Rename"), 1, juce::KeyPress (juce::KeyPress::returnKey));
        alert->addButton (TRANS("Cancel"), 0, juce::KeyPress (juce::KeyPress::escapeKey));

        alert->enterModalState (true, juce::ModalCallbackFunction::create ([this, alert] (int result)
        {
            if (result == 1)
            {
                auto newName = alert->getTextEditorContents ("name").trim();

                if (newName.isNotEmpty() && newName != currentPresetName)
                {
                    pendingPresetName = newName;
                    updatePresetNameLabel();
                }
            }

            delete alert;
        }), false);
    }
    else if (button == &deleteButton)
    {
        if (! currentPresetFile.existsAsFile())
        {
            juce::AlertWindow::showMessageBoxAsync (juce::AlertWindow::InfoIcon,
                                                    TRANS ("Delete Preset"),
                                                    TRANS ("Please save or load a preset first."));
            return;
        }

        juce::AlertWindow::showOkCancelBox (juce::AlertWindow::WarningIcon,
                                            TRANS("Delete Preset"),
                                            TRANS("Are you sure you want to delete \"") + currentPresetName + "\"?",
                                            TRANS("Delete"), TRANS("Cancel"), this,
                                            juce::ModalCallbackFunction::create ([this] (int result)
        {
            if (result == 1)
                listeners.call ([this] (Listener& l) { l.deletePresetRequested (currentPresetFile); });
        }));
    }
    else if (button == &settingsButton)
    {
        listeners.call ([] (Listener& l) { l.settingsRequested(); });
    }
}

//==============================================================================
juce::File PresetBarComponent::getInitialPresetDirectory() const
{
    if (lastPresetDirectory.isDirectory())
        return lastPresetDirectory;

    auto fallback = AppSettings::getInstance().getPresetsDirectory();
    lastPresetDirectory = fallback;
    return fallback;
}

//==============================================================================
void PresetBarComponent::launchSavePresetDialog()
{
    auto initialDir = getInitialPresetDirectory();

    auto defaultName = pendingPresetName.isNotEmpty()      ? pendingPresetName
                       : currentPresetName.isNotEmpty()    ? currentPresetName
                                                           : TRANS("New Preset");

    auto initialFile = currentPresetFile.existsAsFile()
                           ? currentPresetFile.getParentDirectory().getChildFile (defaultName + ".minixer")
                           : initialDir.getChildFile (defaultName + ".minixer");

    auto chooser = std::make_shared<juce::FileChooser> (TRANS("Save Preset"),
                                                        initialFile,
                                                        "*.minixer",
                                                        true);

    chooser->launchAsync (juce::FileBrowserComponent::saveMode
                          | juce::FileBrowserComponent::canSelectFiles,
                          [this, chooser] (const juce::FileChooser& fc)
    {
        auto result = fc.getResult();

        if (result == juce::File{})
            return;

        if (! result.hasFileExtension (".minixer"))
            result = result.withFileExtension (".minixer");

        auto doSave = [this, chooser, result]()
        {
            pendingPresetName.clear();
            currentPresetName = result.getFileNameWithoutExtension();
            currentPresetFile = result;
            lastPresetDirectory = result.getParentDirectory();
            presetNameLabel.setText (currentPresetName, juce::dontSendNotification);
            listeners.call ([&result] (Listener& l) { l.savePresetRequested (result); });
        };

        if (result.existsAsFile())
        {
            juce::AlertWindow::showOkCancelBox (juce::AlertWindow::WarningIcon,
                                                TRANS("Overwrite Preset"),
                                                TRANS("A preset named \"") + result.getFileName()
                                                    + TRANS("\" already exists. Replace it?"),
                                                TRANS("Replace"), TRANS("Cancel"), this,
                                                juce::ModalCallbackFunction::create ([this, chooser, doSave] (int res)
            {
                if (res == 1)
                    doSave();
            }));
        }
        else
        {
            doSave();
        }
    });
}

//==============================================================================
void PresetBarComponent::launchLoadPresetDialog()
{
    auto initialDir = getInitialPresetDirectory();

    auto chooser = std::make_shared<juce::FileChooser> (TRANS("Load Preset"),
                                                        initialDir,
                                                        "*.minixer",
                                                        true);

    chooser->launchAsync (juce::FileBrowserComponent::openMode
                          | juce::FileBrowserComponent::canSelectFiles,
                          [this, chooser] (const juce::FileChooser& fc)
    {
        auto result = fc.getResult();

        if (result == juce::File{})
            return;

        pendingPresetName.clear();
        currentPresetName = result.getFileNameWithoutExtension();
        currentPresetFile = result;
        lastPresetDirectory = result.getParentDirectory();
        presetNameLabel.setText (currentPresetName, juce::dontSendNotification);

        listeners.call ([&result] (Listener& l) { l.loadPresetRequested (result); });
    });
}

} // namespace minixer

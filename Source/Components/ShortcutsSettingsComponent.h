#pragma once

#include <JuceHeader.h>
#include "KeyCaptureButton.h"
#include "../Settings/KeyboardShortcut.h"

namespace minixer
{

//==============================================================================
/** 快捷键设置对话框的内容组件。

    包含两部分：
    - Global shortcuts：可配置现有全局动作（呼出窗口、全局旁通等）
    - Slot shortcuts：12 个插件槽的独立 bypass 快捷键，可设置默认旁通与触发模式
*/
class ShortcutsSettingsComponent  : public juce::Component,
                                    public KeyCaptureButton::Listener,
                                    public juce::Button::Listener,
                                    public juce::ComboBox::Listener
{
public:
    //==============================================================================
    class Listener
    {
    public:
        virtual ~Listener() = default;

        /** 用户点击 Apply/OK 后调用。 */
        virtual void shortcutsSettingsApplied() = 0;

        /** 用户点击 Cancel 或关闭窗口后调用。 */
        virtual void shortcutsSettingsCancelled() = 0;
    };

    //==============================================================================
    ShortcutsSettingsComponent();
    ~ShortcutsSettingsComponent() override = default;

    //==============================================================================
    void paint (juce::Graphics& g) override;
    void resized() override;
    void visibilityChanged() override;

    //==============================================================================
    void addListener (Listener* listener)     { listeners.add (listener); }
    void removeListener (Listener* listener)  { listeners.remove (listener); }

    //==============================================================================
    // KeyCaptureButton::Listener
    void inputSourceCaptureChanged (KeyCaptureButton* button, const ShortcutInputSource& newSource) override;

    //==============================================================================
    // juce::Button::Listener
    void buttonClicked (juce::Button* button) override;

    //==============================================================================
    // juce::ComboBox::Listener
    void comboBoxChanged (juce::ComboBox* comboBox) override;

    /** 丢弃当前编辑并恢复到打开时的设置。 */
    void cancelSettings();

private:
    //==============================================================================
    void setupSectionLabel (juce::Label& label, const juce::String& text);
    void setupLabel (juce::Label& label, const juce::String& text);
    void setupToggleButton (juce::ToggleButton& button, const juce::String& text);
    void setupKeyCaptureButton (KeyCaptureButton& button);
    void setupClearButton (juce::TextButton& button);
    void setupModeComboBox (juce::ComboBox& comboBox);
    void setupMidiDeviceComboBox (juce::ComboBox& comboBox);
    void refreshMidiDeviceList();
    void applyMidiDeviceSelection();

    void loadSettings();
    void applySettings();
    void resetToDefaults();

    void updateSlotShortcutFromUI (int slotIndex);
    void updateUIFromSlotShortcut (int slotIndex);

    //==============================================================================
    juce::Label midiDeviceSectionLabel;
    juce::Label midiDeviceDescriptionLabel;
    juce::ComboBox midiDeviceComboBox;

    //==============================================================================
    juce::Label globalSectionLabel;
    juce::Label globalDescriptionLabel;

    struct GlobalShortcutRow
    {
        juce::Label nameLabel;
        KeyCaptureButton captureButton;
        juce::Label descLabel;
    };

    std::array<GlobalShortcutRow, static_cast<size_t> (GlobalShortcutAction::numActions)> globalRows;

    //==============================================================================
    juce::Label slotSectionLabel;
    juce::Label slotDescriptionLabel;

    struct SlotShortcutRow
    {
        juce::Label slotLabel;
        KeyCaptureButton captureButton;
        juce::TextButton clearButton { TRANS("Clear") };
        juce::ToggleButton defaultBypassToggle { TRANS("Default bypass") };
        juce::ComboBox modeComboBox;
    };

    std::array<SlotShortcutRow, defaultNumPluginSlots> slotRows;

    //==============================================================================
    juce::TextButton resetDefaultsButton { TRANS("Reset to Defaults") };
    juce::TextButton applyButton { TRANS("Apply") };
    juce::TextButton cancelButton { TRANS("Cancel") };

    ShortcutSettings workingSettings;
    bool updatingUI = false;

    juce::ListenerList<Listener> listeners;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ShortcutsSettingsComponent)
};

} // namespace minixer

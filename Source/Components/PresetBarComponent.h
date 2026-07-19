#pragma once

#include <JuceHeader.h>
#include "../Settings/AppSettings.h"

namespace minixer
{

//==============================================================================
/** 预设工具栏组件。

    显示当前 preset 名称，并提供 Save / Load / Rename / Delete / Settings 入口。
    具体状态序列化由 Listener 实现。
*/
class PresetBarComponent  : public juce::Component,
                            public juce::Button::Listener
{
public:
    //==============================================================================
    class Listener
    {
    public:
        virtual ~Listener() = default;

        /** 用户通过文件保存对话框选定目标文件后触发。 */
        virtual void savePresetRequested (const juce::File& presetFile) = 0;

        /** 用户通过文件打开对话框选定源文件后触发。 */
        virtual void loadPresetRequested (const juce::File& presetFile) = 0;

        /** 删除当前 preset 文件。 */
        virtual void deletePresetRequested (const juce::File& presetFile) = 0;

        virtual void settingsRequested() = 0;
    };

    //==============================================================================
    PresetBarComponent();
    ~PresetBarComponent() override = default;

    //==============================================================================
    void addListener (Listener* listener) { listeners.add (listener); }
    void removeListener (Listener* listener) { listeners.remove (listener); }

    //==============================================================================
    /** 设置当前 preset 名称与来源文件。 */
    void setCurrentPresetName (const juce::String& presetName,
                               const juce::File& sourceFile = {});

    /** 清空当前 preset 显示。 */
    void clearCurrentPreset();

    //==============================================================================
    void paint (juce::Graphics& g) override;
    void resized() override;
    void buttonClicked (juce::Button* button) override;

private:
    //==============================================================================
    void launchSavePresetDialog();
    void launchLoadPresetDialog();
    void updatePresetNameLabel();

    /** 返回文件选择器应使用的起始目录：优先上一次目录，失效则回退默认目录。 */
    juce::File getInitialPresetDirectory() const;

    //==============================================================================
    juce::Label presetNameLabel { {}, TRANS("--") };
    juce::TextButton saveButton { TRANS("Save") };
    juce::TextButton loadButton { TRANS("Load") };
    juce::TextButton renameButton { TRANS("Rename") };
    juce::TextButton deleteButton { TRANS("Delete") };
    juce::TextButton settingsButton { TRANS("Settings") };

    juce::String currentPresetName;
    juce::File currentPresetFile;
    juce::String pendingPresetName;   // Rename 设置后、真正 Save 前使用的目标名称
    mutable juce::File lastPresetDirectory;

    juce::ListenerList<Listener> listeners;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PresetBarComponent)
};

} // namespace minixer

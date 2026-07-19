/*
  ==============================================================================

    PluginEditorWindow.h
    承载 AudioProcessorEditor 的独立浮动窗口。

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

namespace minixer
{

//==============================================================================
/** 插件编辑器浮动窗口。

    不拥有 AudioProcessor 的生命周期，仅拥有并显示其编辑器。
    窗口关闭时通知监听者，由 MainComponent 负责从窗口列表中移除。
*/
class PluginEditorWindow  : public juce::DocumentWindow
{
public:
    //==============================================================================
    class Listener
    {
    public:
        virtual ~Listener() = default;
        virtual void pluginEditorWindowClosed (PluginEditorWindow* window) = 0;
    };

    //==============================================================================
    PluginEditorWindow (juce::AudioProcessor* processorToEdit,
                        const juce::String& pluginName);
    ~PluginEditorWindow() override = default;

    //==============================================================================
    juce::AudioProcessor* getProcessor() const noexcept { return processor; }

    //==============================================================================
    void addListener (Listener* listener)       { listeners.add (listener); }
    void removeListener (Listener* listener)    { listeners.remove (listener); }

    //==============================================================================
    void closeButtonPressed() override;

private:
    //==============================================================================
    juce::AudioProcessor* processor = nullptr;
    juce::ListenerList<Listener> listeners;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginEditorWindow)
};

} // namespace minixer

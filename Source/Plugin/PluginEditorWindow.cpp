/*
  ==============================================================================

    PluginEditorWindow.cpp

  ==============================================================================
*/

#include "PluginEditorWindow.h"
#include "../LookAndFeel/MixerLookAndFeel.h"

namespace minixer
{

//==============================================================================
PluginEditorWindow::PluginEditorWindow (juce::AudioProcessor* processorToEdit,
                                        const juce::String& pluginName)
    : juce::DocumentWindow (pluginName.isEmpty() ? TRANS("Plugin Editor") : pluginName,
                            MixerLookAndFeel::getBackgroundColour(),
                            DocumentWindow::closeButton),
      processor (processorToEdit)
{
    juce::Component* editor = nullptr;

    if (processor != nullptr)
        editor = processor->createEditorIfNeeded();

    if (editor == nullptr)
    {
        auto* label = new juce::Label ("noEditor", TRANS("This plugin has no editor."));
        label->setJustificationType (juce::Justification::centred);
        label->setColour (juce::Label::textColourId, MixerLookAndFeel::getTextColour());
        editor = label;
    }

    setContentOwned (editor, true);
    setUsingNativeTitleBar (true);
    setResizable (true, false);

    auto bounds = editor->getBounds();
    if (bounds.isEmpty())
        bounds = juce::Rectangle<int> (0, 0, 400, 300);

    centreWithSize (bounds.getWidth(), bounds.getHeight());
}

//==============================================================================
void PluginEditorWindow::closeButtonPressed()
{
    setVisible (false);

    // 异步通知监听者关闭窗口，避免监听者在此函数内销毁本对象后继续访问成员。
    juce::Component::SafePointer<PluginEditorWindow> safePtr (this);

    juce::MessageManager::callAsync ([safePtr]() mutable
    {
        if (auto* window = safePtr.getComponent())
            window->listeners.call ([window] (Listener& l) { l.pluginEditorWindowClosed (window); });
    });
}

} // namespace minixer

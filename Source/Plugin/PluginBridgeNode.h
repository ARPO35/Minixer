/*
  ==============================================================================

    PluginBridgeNode.h
    AudioProcessorGraph 中的插件桥接节点。

    将音频处理、参数/状态读写转发到独立的 PluginHost 子进程，实现崩溃隔离
    与 32-bit 插件桥接。

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginHostClient.h"
#include "PluginHostLauncher.h"
#include "PluginArchitecture.h"
#include "PluginBlacklist.h"

namespace minixer
{

//==============================================================================
/** 插件桥接处理器。

    外观上是一个 JUCE AudioProcessor，可加入 AudioProcessorGraph；内部通过
    PluginHostClient 与子进程通信。
*/
class PluginBridgeNode  : public juce::AudioProcessor
{
public:
    //==============================================================================
    class Listener
    {
    public:
        virtual ~Listener() = default;

        /** 子进程崩溃或挂起时被调用（消息线程）。 */
        virtual void pluginBridgeNodeCrashed (PluginBridgeNode* node) = 0;
    };

    //==============================================================================
    PluginBridgeNode (const juce::PluginDescription& description,
                      PluginArchitecture arch);
    ~PluginBridgeNode() override;

    //==============================================================================
    /** 启动子进程并建立 IPC。 */
    bool initialize (double sampleRate, int bufferSize, juce::String& errorMessage);

    /** 主动关闭子进程。 */
    void shutdown();

    //==============================================================================
    void addListener (Listener* listener);
    void removeListener (Listener* listener);

    /** 返回子进程是否已经崩溃/挂起。 */
    bool hasCrashed() const noexcept { return crashed.load(); }

    /** 返回崩溃/挂起原因。 */
    juce::String getCrashReason() const { return crashReason; }

    //==============================================================================
    const juce::String getName() const override { return pluginName; }

    void prepareToPlay (double sampleRate, int samplesPerBlockExpected) override;
    void releaseResources() override;
    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;

    double getTailLengthSeconds() const override { return 0.0; }

    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }

    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    bool hasEditor() const override { return false; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    //==============================================================================
    bool isPluginRunning() const;
    juce::String getLastError() const;

    void showEditorWindow (juce::Component* parent = nullptr);
    void hideEditorWindow();

private:
    //==============================================================================
    juce::PluginDescription pluginDescription;
    juce::String pluginName;
    PluginArchitecture architecture;

    uint32_t currentInputChannels = 2;
    uint32_t currentOutputChannels = 2;

    std::unique_ptr<PluginHostClient> client;
    std::unique_ptr<PluginHostLauncher> launcher;

    double currentSampleRate = 44100.0;
    int currentBufferSize = 512;
    bool initialized = false;
    bool isShuttingDown = false;

    struct CrashState { std::atomic<bool> alive { true }; };

    std::atomic<bool> crashed { false };
    juce::String crashReason;
    juce::ListenerList<Listener> listeners;
    std::shared_ptr<CrashState> crashState;

    void handleProcessFailure (const juce::String& reason);
    void notifyCrashAsync();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginBridgeNode)
};

} // namespace minixer

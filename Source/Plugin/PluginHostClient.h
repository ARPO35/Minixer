/*
  ==============================================================================

    PluginHostClient.h
    主进程中与 PluginHost 子进程通信的客户端封装。

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "../IPC/IpcTransport.h"
#include "../IPC/SharedMemoryRegion.h"
#include "../IPC/IpcProtocol.h"

namespace minixer
{

//==============================================================================
/** 主进程侧 PluginHost 客户端。

    负责建立控制通道与共享内存，将插件的 AudioProcessor 调用转换为
    IPC 消息，并同步音频数据。
*/
class PluginHostClient
{
public:
    //==============================================================================
    PluginHostClient();
    ~PluginHostClient();

    //==============================================================================
    /** 连接到指定 ipc-key 的子进程。创建共享内存并映射音频布局。 */
    bool connect (const juce::String& ipcKey,
                  uint32_t maxFrames,
                  uint32_t numInputs,
                  uint32_t numOutputs);

    /** 断开连接并清理资源。 */
    void disconnect();

    //==============================================================================
    /** 初始化插件。返回是否成功。 */
    bool initPlugin (double sampleRate, int bufferSize);

    /** 准备播放。 */
    bool prepareToPlay (double sampleRate, int bufferSize);

    /** 释放资源。 */
    bool releaseResources();

    /** 触发一帧处理。调用前需将输入写入共享内存输入缓冲区。 */
    bool processBlock (int numSamples);

    //==============================================================================
    /** 写入输入采样到共享内存。 */
    void writeInput (const juce::AudioBuffer<float>& inputBuffer, int numSamples);

    /** 从共享内存读取输出采样。 */
    void readOutput (juce::AudioBuffer<float>& outputBuffer, int numSamples);

    //==============================================================================
    bool setState (const juce::MemoryBlock& state);
    bool getState (juce::MemoryBlock& state);
    bool setParameter (int index, float value);
    int  getLatencySamples();

    //==============================================================================
    bool showEditor (void* parentWindowHandle);
    bool hideEditor();

    //==============================================================================
    bool shutdown();

    //==============================================================================
    bool isConnected() const;
    juce::String getLastError() const { return lastError; }

    //==============================================================================
    AudioSharedMemoryLayout* getAudioLayout() const { return audioLayout; }

private:
    //==============================================================================
    bool sendCommand (ControlMessageType type,
                      const juce::MemoryBlock& payload,
                      uint64_t requestId);

    bool readResponse (juce::MemoryBlock& payload,
                       ControlMessageType expectedType,
                       uint64_t requestId,
                       int timeoutMs);

    uint64_t nextRequestId();

    //==============================================================================
    std::unique_ptr<IpcTransport> transport;
    std::unique_ptr<SharedMemoryRegion> sharedMemory;
    AudioSharedMemoryLayout* audioLayout = nullptr;

    uint32_t maxFramesPerBlock = 0;
    uint32_t numInputChannels = 0;
    uint32_t numOutputChannels = 0;
    uint64_t currentRequestId = 1;

    juce::String lastError;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginHostClient)
};

} // namespace minixer

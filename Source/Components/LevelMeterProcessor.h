#pragma once

#include <JuceHeader.h>
#include <array>
#include <functional>
#include <memory>

namespace minixer
{

class MeterMeasurement;

//==============================================================================
/** 用于 AudioProcessorGraph 中的双声道电平探测处理器。

    同时计算 dBFS 峰值、RMS、LUFS-M（400ms）和 LUFS-S（3s），
    并通过回调函数将多标准电平数据写回给 UI 线程。
    注意：回调会从音频线程调用，接收方需保证线程安全。
*/
class LevelMeterProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    //==============================================================================
    /** 多标准电平数据，由音频线程产生并通过回调写回 UI。 */
    struct MeterData
    {
        std::array<float, 2> dbfs  { { -60.0f, -60.0f } };
        std::array<float, 2> rms   { { -60.0f, -60.0f } };
        float                lufsM = -60.0f;
        float                lufsS = -60.0f;
    };

    using LevelCallback = std::function<void (const MeterData&)>;

    //==============================================================================
    LevelMeterProcessor (LevelCallback callback);
    ~LevelMeterProcessor() override;

    const juce::String getName() const override { return "Level Meter"; }
    void prepareToPlay (double sampleRate, int /*maximumExpectedSamplesPerBlock*/) override;
    void releaseResources() override {}
    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& /*midiMessages*/) override;
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
    void getStateInformation (juce::MemoryBlock&) override {}
    void setStateInformation (const void*, int) override {}

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override
    {
        return (layouts.getMainInputChannelSet() == juce::AudioChannelSet::mono()
                || layouts.getMainInputChannelSet() == juce::AudioChannelSet::stereo())
            && (layouts.getMainOutputChannelSet() == juce::AudioChannelSet::mono()
                || layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo());
    }

private:
    //==============================================================================
    LevelCallback levelCallback;
    std::unique_ptr<MeterMeasurement> measurement;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LevelMeterProcessor)
};

} // namespace minixer

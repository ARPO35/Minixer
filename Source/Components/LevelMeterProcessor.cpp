#include "LevelMeterProcessor.h"
#include <algorithm>
#include <cmath>
#include <vector>

namespace minixer
{

//==============================================================================
/** 滑动窗口的均方值缓冲区。 */
struct SquaredRingBuffer
{
    void setSize (size_t newSize)
    {
        data.assign (newSize, 0.0);
        size = newSize;
        pos = 0;
        count = 0;
        sum = 0.0;
    }

    void reset()
    {
        std::fill (data.begin(), data.end(), 0.0);
        pos = 0;
        count = 0;
        sum = 0.0;
    }

    void push (double value) noexcept
    {
        if (size == 0)
            return;

        if (count < size)
        {
            data[pos] = value;
            sum += value;
            ++count;
        }
        else
        {
            sum += value - data[pos];
            data[pos] = value;
        }

        pos = (pos + 1) % size;
    }

    double mean() const noexcept
    {
        return count > 0 ? sum / static_cast<double> (count) : 0.0;
    }

    std::vector<double> data;
    size_t size = 0;
    size_t pos = 0;
    size_t count = 0;
    double sum = 0.0;
};

//==============================================================================
/** 多标准电平测量引擎。

    支持：
    - dBFS：每声道峰值
    - RMS：每声道 200ms 窗口均方根
    - LUFS-M：400ms 瞬间响度（EBU R128 K 计权）
    - LUFS-S：3s 短期响度（EBU R128 K 计权）
*/
class MeterMeasurement
{
public:
    using Result = LevelMeterProcessor::MeterData;

    void prepare (double newSampleRate)
    {
        sampleRate = newSampleRate;

        if (sampleRate <= 0.0)
        {
            prepared = false;
            return;
        }

        prepared = true;

        const auto rmsWindow   = static_cast<size_t> (std::ceil (sampleRate * rmsWindowMs   / 1000.0));
        const auto lufsMWindow = static_cast<size_t> (std::ceil (sampleRate * lufsMWindowMs / 1000.0));
        const auto lufsSWindow = static_cast<size_t> (std::ceil (sampleRate * lufsSWindowMs / 1000.0));

        rmsBuffer[0].setSize (rmsWindow);
        rmsBuffer[1].setSize (rmsWindow);
        lufsMBuffer.setSize (lufsMWindow);
        lufsSBuffer.setSize (lufsSWindow);

        for (int ch = 0; ch < 2; ++ch)
        {
            kHp[ch].setCoefficients (juce::IIRCoefficients::makeHighPass  (sampleRate, 41.0, 1.0));
            kHs[ch].setCoefficients (juce::IIRCoefficients::makeHighShelf (sampleRate, 1500.0, 1.0,
                                                                           juce::Decibels::decibelsToGain (4.0f)));
            kHp[ch].reset();
            kHs[ch].reset();
        }

        rmsBuffer[0].reset();
        rmsBuffer[1].reset();
        lufsMBuffer.reset();
        lufsSBuffer.reset();
    }

    Result process (const float* const* channelData, int numChannels, int numSamples)
    {
        Result result;

        if (! prepared || numSamples == 0 || numChannels == 0)
            return result;

        const int activeChannels = juce::jlimit (1, 2, numChannels);

        // dBFS 峰值
        for (int ch = 0; ch < activeChannels; ++ch)
        {
            float peak = 0.0f;
            auto* data = channelData[ch];

            for (int i = 0; i < numSamples; ++i)
                peak = juce::jmax (peak, std::abs (data[i]));

            result.dbfs[ch] = (peak <= 0.0f) ? silenceDb
                                             : juce::Decibels::gainToDecibels (peak, silenceDb);
        }

        // RMS / LUFS：逐样本更新滑动窗口
        for (int i = 0; i < numSamples; ++i)
        {
            double lufsSum = 0.0;

            for (int ch = 0; ch < activeChannels; ++ch)
            {
                const float sample = channelData[ch][i];
                const float k = kHs[ch].processSingleSampleRaw (kHp[ch].processSingleSampleRaw (sample));
                const double sq = static_cast<double> (k) * static_cast<double> (k);

                rmsBuffer[ch].push (sq);
                lufsSum += sq;
            }

            const double meanK = lufsSum / static_cast<double> (activeChannels);
            lufsMBuffer.push (meanK);
            lufsSBuffer.push (meanK);
        }

        for (int ch = 0; ch < activeChannels; ++ch)
            result.rms[ch] = meanSquaredToDb (rmsBuffer[ch].mean());

        result.lufsM = lufsMeanToLufs (lufsMBuffer.mean());
        result.lufsS = lufsMeanToLufs (lufsSBuffer.mean());

        return result;
    }

private:
    static constexpr float rmsWindowMs   = 200.0f;
    static constexpr float lufsMWindowMs = 400.0f;
    static constexpr float lufsSWindowMs = 3000.0f;
    static constexpr float silenceDb     = -60.0f;

    double sampleRate = 44100.0;
    bool prepared = false;

    std::array<SquaredRingBuffer, 2> rmsBuffer;
    SquaredRingBuffer lufsMBuffer;
    SquaredRingBuffer lufsSBuffer;

    std::array<juce::IIRFilter, 2> kHp;
    std::array<juce::IIRFilter, 2> kHs;

    static float meanSquaredToDb (double meanSq) noexcept
    {
        return (meanSq > 0.0) ? static_cast<float> (10.0 * std::log10 (meanSq)) : silenceDb;
    }

    static float lufsMeanToLufs (double meanSq) noexcept
    {
        return (meanSq > 0.0) ? static_cast<float> (-0.691 + 10.0 * std::log10 (meanSq)) : silenceDb;
    }
};

//==============================================================================
LevelMeterProcessor::LevelMeterProcessor (LevelCallback callback)
    : juce::AudioProcessor (juce::AudioProcessor::BusesProperties()
                                .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                                .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      levelCallback (std::move (callback)),
      measurement (std::make_unique<MeterMeasurement>())
{
}

LevelMeterProcessor::~LevelMeterProcessor() = default;

//==============================================================================
void LevelMeterProcessor::prepareToPlay (double sampleRate, int /*maximumExpectedSamplesPerBlock*/)
{
    measurement->prepare (sampleRate);
}

//==============================================================================
void LevelMeterProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& /*midiMessages*/)
{
    auto result = measurement->process (buffer.getArrayOfReadPointers(),
                                        buffer.getNumChannels(),
                                        buffer.getNumSamples());

    if (levelCallback != nullptr)
        levelCallback (result);
}

} // namespace minixer

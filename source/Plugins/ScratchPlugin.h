#pragma once

#include <juce_core/juce_core.h>
#include <juce_dsp/juce_dsp.h>
#include <tracktion_engine/tracktion_engine.h>


class ScratchPlugin : public tracktion::engine::Plugin
{
public:
    ScratchPlugin(tracktion::engine::PluginCreationInfo info);
    ~ScratchPlugin() override;

    static const char* getPluginName() { return NEEDS_TRANS("Scratch"); }
    static const char* xmlTypeName;

    juce::String getName() const override { return TRANS("Scratch"); }
    juce::String getPluginType() override { return xmlTypeName; }
    juce::String getSelectableDescription() override { return TRANS("Scratch Plugin"); }

    void initialise(const tracktion::engine::PluginInitialisationInfo& info) override;
    void deinitialise() override;
    void reset() override;
    void applyToBuffer(const tracktion::engine::PluginRenderContext& context) override;
    void restorePluginStateFromValueTree(const juce::ValueTree& valueTree) override;

    juce::CachedValue<float> scratchValue, mixValue;
    tracktion::engine::AutomatableParameter::Ptr scratchParam;
    tracktion::engine::AutomatableParameter::Ptr mixParam;

private:
    double sampleRate = 44100.0;
    float lastScratchValue = 0.0f;
    juce::SmoothedValue<float> scratchSmoother;
    juce::AudioBuffer<float> delayBuffer;
    int delayBufferPos = 0;
    int delayBufferLength = 0;
    static constexpr int maxDelayLength = 48000; // 1 second at 48kHz
    
    float calculatePlaybackRate(float scratchValue);
    void updateDelayBuffer(const float* inputData, int numSamples, int channel);
    float getInterpolatedSample(float delayInSamples, int channel);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScratchPlugin)
}; 
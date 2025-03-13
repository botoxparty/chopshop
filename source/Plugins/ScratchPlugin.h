#pragma once

#include <juce_core/juce_core.h>
#include <juce_dsp/juce_dsp.h>
#include <tracktion_engine/tracktion_engine.h>

//==============================================================================
struct ScratchBufferBase
{
    ScratchBufferBase() {}
    
    void ensureMaxBufferSize(int size)
    {
        if (++size > bufferSamples)
        {
            bufferSamples = size;
            buffers[0].ensureSize((size_t)bufferSamples * sizeof(float) + 32, true);
            buffers[1].ensureSize((size_t)bufferSamples * sizeof(float) + 32, true);
            
            if (bufferPos >= bufferSamples)
                bufferPos = 0;
        }
    }
    
    void clearBuffer()
    {
        buffers[0].fillWith(0);
        buffers[1].fillWith(0);
    }
    
    void releaseBuffer()
    {
        bufferSamples = 0;
        bufferPos = 0;
        buffers[0].setSize(0);
        buffers[1].setSize(0);
    }
    
    int bufferPos = 0, bufferSamples = 0;
    juce::MemoryBlock buffers[2];
    
private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScratchBufferBase)
};

//==============================================================================
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
    
    int getNumOutputChannelsGivenInputs(int numInputChannels) override { return juce::jmin(numInputChannels, 2); }
    void initialise(const tracktion::engine::PluginInitialisationInfo&) override;
    void deinitialise() override;
    void reset() override;
    void applyToBuffer(const tracktion::engine::PluginRenderContext&) override;
    void restorePluginStateFromValueTree(const juce::ValueTree&) override;

    juce::CachedValue<float> scratchValue, mixValue;
    tracktion::engine::AutomatableParameter::Ptr scratchParam, mixParam;

private:
    float interpolateHermite4pt3oX(float x, float y0, float y1, float y2, float y3);
    float getSampleAtPosition(float* buf, int bufferLength, float position);
    float processNonLinearScratch(float input) const;
    
    ScratchBufferBase scratchBuffer;
    double sampleRate = 44100.0;
    juce::SmoothedValue<float> smoothedScratchPos;
    juce::SmoothedValue<float> smoothedAcceleration;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScratchPlugin)
}; 
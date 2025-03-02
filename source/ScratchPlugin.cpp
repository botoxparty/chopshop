#include "ScratchPlugin.h"

const char* ScratchPlugin::xmlTypeName = "scratch";

ScratchPlugin::ScratchPlugin(tracktion::engine::PluginCreationInfo info) : tracktion::engine::Plugin(info)
{
    auto um = getUndoManager();

    // Initialize cached values
    scratchValue.referTo(state, "scratch", um, 0.0f);
    mixValue.referTo(state, "mix", um, 1.0f);

    // Create automatable parameters
    scratchParam = addParam("scratch", TRANS("Scratch"), { -1.0f, 1.0f },
                           [](float value) { return juce::String(value, 2); },
                           [](const juce::String& s) { return s.getFloatValue(); });
    scratchParam->attachToCurrentValue(scratchValue);

    mixParam = addParam("mix", TRANS("Mix"), { 0.0f, 1.0f },
                       [](float value) { return juce::String(value * 100.0f, 0) + "%" ; },
                       [](const juce::String& s) { return s.getFloatValue() / 100.0f; });
    mixParam->attachToCurrentValue(mixValue);

    // Initialize smoothed value
    scratchSmoother.reset(sampleRate, 0.05); // 50ms smoothing
}

ScratchPlugin::~ScratchPlugin()
{
    notifyListenersOfDeletion();
    
    scratchParam.reset();
    mixParam.reset();
}

void ScratchPlugin::initialise(const tracktion::engine::PluginInitialisationInfo& info)
{
    sampleRate = info.sampleRate;
    scratchSmoother.reset(sampleRate, 0.05);
    delayBuffer.setSize(2, maxDelayLength);
    delayBuffer.clear();
    delayBufferPos = 0;
}

void ScratchPlugin::deinitialise()
{
    delayBuffer.setSize(0, 0);
}

void ScratchPlugin::reset()
{
    delayBuffer.clear();
    delayBufferPos = 0;
    scratchSmoother.reset(sampleRate, 0.05);
}

void ScratchPlugin::applyToBuffer(const tracktion::engine::PluginRenderContext& fc)
{
    if (fc.destBuffer == nullptr)
        return;

    const float scratchTarget = scratchParam->getCurrentValue();
    const float mix = mixParam->getCurrentValue();
    
    // Process each channel
    for (int channel = 0; channel < fc.destBuffer->getNumChannels(); ++channel)
    {
        float* const channelData = fc.destBuffer->getWritePointer(channel, fc.bufferStartSample);
        float* const delayData = delayBuffer.getWritePointer(channel);
        
        for (int i = 0; i < fc.bufferNumSamples; ++i)
        {
            // Update scratch value with smoothing
            scratchSmoother.setTargetValue(scratchTarget);
            const float currentScratch = scratchSmoother.getNextValue();
            
            // Calculate read position with scratch effect
            float readPos = delayBufferPos - i;
            readPos += currentScratch * 1000.0f; // Adjust scratch intensity
            
            // Wrap around delay buffer
            while (readPos < 0)
                readPos += maxDelayLength;
            while (readPos >= maxDelayLength)
                readPos -= maxDelayLength;
            
            // Linear interpolation for smooth playback
            const int pos1 = static_cast<int>(readPos);
            const int pos2 = (pos1 + 1) % maxDelayLength;
            const float frac = readPos - pos1;
            
            const float value1 = delayData[pos1];
            const float value2 = delayData[pos2];
            const float interpolatedValue = value1 + frac * (value2 - value1);
            
            // Mix dry/wet
            const float in = channelData[i];
            delayData[(delayBufferPos + i) % maxDelayLength] = in;
            channelData[i] = in * (1.0f - mix) + interpolatedValue * mix;
        }
    }
    
    // Update delay buffer position
    delayBufferPos = (delayBufferPos + fc.bufferNumSamples) % maxDelayLength;
}

void ScratchPlugin::restorePluginStateFromValueTree(const juce::ValueTree& v)
{
    Plugin::restorePluginStateFromValueTree(v);
    
    for (auto p : getAutomatableParameters())
        p->updateFromAttachedValue();
} 
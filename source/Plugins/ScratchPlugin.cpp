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
    scratchSmoother.setCurrentAndTargetValue(0.0f);
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
    delayBufferLength = (int)(sampleRate * 2.0); // 2 seconds buffer
    delayBuffer.setSize(2, delayBufferLength);
    delayBuffer.clear();
    delayBufferPos = 0;
    scratchSmoother.reset(sampleRate, 0.05);
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

float ScratchPlugin::calculatePlaybackRate(float scratchValue)
{
    // Convert scratch value (-4 to 4) to playback rate
    return juce::jlimit(-4.0f, 4.0f, scratchValue);
}

void ScratchPlugin::updateDelayBuffer(const float* inputData, int numSamples, int channel)
{
    auto* delayData = delayBuffer.getWritePointer(channel);
    
    for (int i = 0; i < numSamples; ++i)
    {
        delayData[delayBufferPos] = inputData[i];
        delayBufferPos = (delayBufferPos + 1) % delayBufferLength;
    }
}

float ScratchPlugin::getInterpolatedSample(float delayInSamples, int channel)
{
    const float* delayData = delayBuffer.getReadPointer(channel);
    
    int pos = delayBufferPos - (int)delayInSamples;
    if (pos < 0) pos += delayBufferLength;
    
    const int pos1 = pos;
    const int pos2 = (pos1 + 1) % delayBufferLength;
    
    const float frac = delayInSamples - (int)delayInSamples;
    
    return delayData[pos1] + frac * (delayData[pos2] - delayData[pos1]);
}

void ScratchPlugin::applyToBuffer(const tracktion::engine::PluginRenderContext& context)
{
    if (context.destBuffer == nullptr) return;
    
    auto& destBuffer = *context.destBuffer;
    auto numChannels = destBuffer.getNumChannels();
    auto numSamples = destBuffer.getNumSamples();
    
    // Update scratch value smoothing
    scratchSmoother.setTargetValue(scratchValue.get());
    
    // Process each channel
    for (int channel = 0; channel < numChannels; ++channel)
    {
        auto* channelData = destBuffer.getWritePointer(channel);
        
        // Store input in delay buffer
        updateDelayBuffer(channelData, numSamples, channel);
        
        // Process samples
        for (int sample = 0; sample < numSamples; ++sample)
        {
            float currentScratch = scratchSmoother.getNextValue();
            float playbackRate = calculatePlaybackRate(currentScratch);
            
            // Calculate delay time based on playback rate
            float delayTime = delayBufferLength * 0.5f; // Center point
            float scratchOffset = delayTime * (1.0f - playbackRate);
            
            // Get scratched sample using interpolation
            float scratchedSample = getInterpolatedSample(scratchOffset, channel);
            
            // Mix dry and wet signals
            float mix = mixValue.get();
            channelData[sample] = scratchedSample * mix + channelData[sample] * (1.0f - mix);
        }
    }
}

void ScratchPlugin::restorePluginStateFromValueTree(const juce::ValueTree& valueTree)
{
    if (valueTree.hasProperty("scratch"))
        scratchValue = (float)valueTree.getProperty("scratch");
    if (valueTree.hasProperty("mix"))
        mixValue = (float)valueTree.getProperty("mix");
}
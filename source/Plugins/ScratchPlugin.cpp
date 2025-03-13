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
    
    // Initialize smoothed values with longer smoothing time for more natural feel
    smoothedScratchPos.reset(sampleRate, 0.15); // 150ms smoothing
    smoothedScratchPos.setCurrentAndTargetValue(0.0);
    
    // Initialize additional smoothing for scratch acceleration
    smoothedAcceleration.reset(sampleRate, 0.08); // 80ms smoothing
    smoothedAcceleration.setCurrentAndTargetValue(0.0);
}

ScratchPlugin::~ScratchPlugin()
{
    notifyListenersOfDeletion();
    
    scratchParam->detachFromCurrentValue();
    mixParam->detachFromCurrentValue();
}

void ScratchPlugin::initialise(const tracktion::engine::PluginInitialisationInfo& info)
{
    sampleRate = info.sampleRate;
    const int lengthInSamples = (int)(sampleRate * 4.0); // 4 second buffer
    scratchBuffer.ensureMaxBufferSize(lengthInSamples);
    scratchBuffer.clearBuffer();
    
    // Update smoothing time based on sample rate
    smoothedScratchPos.reset(sampleRate, 0.05);
}

void ScratchPlugin::deinitialise()
{
    scratchBuffer.releaseBuffer();
}

void ScratchPlugin::reset()
{
    scratchBuffer.clearBuffer();
    smoothedScratchPos.reset(sampleRate, 0.05);
}

// Hermite interpolation for smooth sample playback
float ScratchPlugin::interpolateHermite4pt3oX(float x, float y0, float y1, float y2, float y3)
{
    // 4-point, 3rd-order Hermite (x-form)
    float c0 = y1;
    float c1 = 0.5f * (y2 - y0);
    float c2 = y0 - 2.5f * y1 + 2.f * y2 - 0.5f * y3;
    float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);
    return ((c3 * x + c2) * x + c1) * x + c0;
}

float ScratchPlugin::getSampleAtPosition(float* buf, int bufferLength, float position)
{
    int pos0 = (int)std::floor(position - 1);
    int pos1 = (int)std::floor(position);
    int pos2 = (int)std::floor(position + 1);
    int pos3 = (int)std::floor(position + 2);
    
    // Wrap positions
    while (pos0 < 0) pos0 += bufferLength;
    while (pos1 < 0) pos1 += bufferLength;
    while (pos2 < 0) pos2 += bufferLength;
    while (pos3 < 0) pos3 += bufferLength;
    
    pos0 %= bufferLength;
    pos1 %= bufferLength;
    pos2 %= bufferLength;
    pos3 %= bufferLength;
    
    float frac = position - std::floor(position);
    return interpolateHermite4pt3oX(frac, buf[pos0], buf[pos1], buf[pos2], buf[pos3]);
}

// Add new helper function for non-linear scratch response
float ScratchPlugin::processNonLinearScratch(float input) const
{
    // Apply exponential curve for more natural scratch response
    float sign = input < 0 ? -1.0f : 1.0f;
    float absInput = std::abs(input);
    
    // Enhanced non-linear curve with dead zone and exponential response
    if (absInput < 0.1f) return 0.0f; // Dead zone for stability
    
    float processed = sign * (std::pow(absInput, 1.5f) + absInput * 0.5f);
    return processed;
}

void ScratchPlugin::applyToBuffer(const tracktion::engine::PluginRenderContext& fc)
{
    if (fc.destBuffer == nullptr)
        return;
        
    SCOPED_REALTIME_CHECK
    
    const float wetGain = mixParam->getCurrentValue();
    const float dryGain = 1.0f - wetGain;
    
    const int lengthInSamples = (int)(sampleRate * 4.0); // 4 second buffer
    scratchBuffer.ensureMaxBufferSize(lengthInSamples);
    
    const int offset = scratchBuffer.bufferPos;
    
    // Clear any channels we're not using
    tracktion::engine::clearChannels(*fc.destBuffer, 2, -1, fc.bufferStartSample, fc.bufferNumSamples);
    
    // Get the current scratch position (-1 to 1) and apply non-linear processing
    const float rawScratch = scratchParam->getCurrentValue();
    const float processedScratch = processNonLinearScratch(rawScratch);
    
    smoothedScratchPos.setTargetValue(processedScratch);
    smoothedAcceleration.setTargetValue(std::abs(processedScratch - smoothedScratchPos.getCurrentValue()));
    
    const float currentScratch = rawScratch; // Use raw value for bypass check
    
    // Debug print scratch values
    static int debugCounter = 0;
    if (++debugCounter % 1000 == 0)
    {
        DBG("Scratch param value: " << rawScratch 
            << " Smoothed value: " << smoothedScratchPos.getCurrentValue() 
            << " Mix: " << wetGain);
    }
    
    // If scratch is at neutral position (very close to 0), just pass through the audio
    if (std::abs(currentScratch) < 0.05f) // Slightly larger dead zone
    {
        if (debugCounter % 1000 == 0)
        {
            DBG("Bypassing - scratch near zero");
        }
        scratchBuffer.bufferPos = (scratchBuffer.bufferPos + fc.bufferNumSamples) % lengthInSamples;
        return;
    }
    
    if (debugCounter % 1000 == 0)
    {
        DBG("Processing scratch effect");
    }
    
    // Process up to 2 channels
    for (int chan = std::min(2, fc.destBuffer->getNumChannels()); --chan >= 0;)
    {
        float* const d = fc.destBuffer->getWritePointer(chan, fc.bufferStartSample);
        float* const buf = (float*)scratchBuffer.buffers[chan].getData();
        
        // Keep track of our read position
        float readPos = offset;
        
        for (int i = 0; i < fc.bufferNumSamples; ++i)
        {
            const float in = d[i];
            float* const b = buf + ((i + offset) % lengthInSamples);
            
            // Store input in buffer
            *b = in;
            
            // Get smoothed scratch value and calculate read position with improved mechanics
            const float currentScratchValue = smoothedScratchPos.getNextValue();
            const float scratchAcceleration = smoothedAcceleration.getNextValue();
            
            // Enhanced playback speed calculation
            float scratchDelta = currentScratchValue * (1.0f + scratchAcceleration * 0.5f);
            
            // Add slight pitch variation based on scratch speed
            float pitchVariation = 1.0f + (scratchAcceleration * 0.1f);
            readPos += (1.0f - scratchDelta) * pitchVariation;
            
            // Wrap read position
            while (readPos >= lengthInSamples) readPos -= lengthInSamples;
            while (readPos < 0) readPos += lengthInSamples;
            
            // Get interpolated sample with enhanced smoothing
            const float wet = getSampleAtPosition(buf, lengthInSamples, readPos);
            
            // Debug occasional sample values
            if (debugCounter % 100 == 0 && i == 0)
            {
                DBG("Sample " << i << " - ReadPos: " << readPos 
                    << " ScratchDelta: " << scratchDelta 
                    << " In: " << in 
                    << " Wet: " << juce::String(wet));
            }
            
            // Mix dry and wet signals
            d[i] = wet * wetGain + in * dryGain;
        }
    }
    
    scratchBuffer.bufferPos = (scratchBuffer.bufferPos + fc.bufferNumSamples) % lengthInSamples;
    
    tracktion::engine::zeroDenormalisedValuesIfNeeded(*fc.destBuffer);
}

void ScratchPlugin::restorePluginStateFromValueTree(const juce::ValueTree& v)
{
    tracktion::engine::copyPropertiesToCachedValues(v, scratchValue, mixValue);
    
    for (auto p : getAutomatableParameters())
        p->updateFromAttachedValue();
}
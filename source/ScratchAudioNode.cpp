#include "ScratchAudioNode.h"

ScratchAudioNode::ScratchAudioNode(std::unique_ptr<tracktion::graph::Node> inputNode)
    : input(std::move(inputNode))
{
    nodeProperties = input->getNodeProperties();
}

tracktion::graph::NodeProperties ScratchAudioNode::getNodeProperties()
{
    return nodeProperties;
}

void ScratchAudioNode::prepareToPlay(const tracktion::graph::PlaybackInitialisationInfo& info)
{
    // Don't call input->prepareToPlay(info) - it's protected
    
    // Just initialize our own state
    smoothedSpeed.reset(info.sampleRate, 0.05);
}

bool ScratchAudioNode::isReadyToProcess()
{
    return input->isReadyToProcess();
}

void ScratchAudioNode::process(ProcessContext& pc)
{
    // We can't call input->process(pc) directly as it's protected
    // The graph system has already processed our inputs for us
    
    // If scratching is not active, just pass through the audio
    if (!isActive.load(std::memory_order_acquire))
        return;
    
    auto& inputBlock = pc.buffers.audio;
    const auto numFrames = inputBlock.getNumFrames();
    const auto numChannels = inputBlock.getNumChannels();
    
    if (numChannels == 0 || numFrames == 0)
        return;
    
    // Get current scratch parameters
    float speed = scratchSpeed.load(std::memory_order_acquire);
    float depth = scratchDepth.load(std::memory_order_acquire);
    
    // Smooth the speed parameter to avoid clicks
    smoothedSpeed.setTargetValue(speed);
    
    // Process each frame
    for (choc::buffer::FrameCount i = 0; i < numFrames; ++i)
    {
        // Get the current smoothed speed
        float currentSpeed = smoothedSpeed.getNextValue();
        
        // Calculate the effective playback rate
        // When currentSpeed is 0, playback is normal (rate = 1.0)
        // Positive speed increases rate, negative speed plays backwards
        float effectiveRate = 1.0f - (currentSpeed * depth);
        
        // Store input samples in ring buffer
        if (numChannels >= 1)
            ringBufferL[writePos] = inputBlock.getSample(0, i);
        if (numChannels >= 2)
            ringBufferR[writePos] = inputBlock.getSample(1, i);
        
        // Update write position
        writePos = (writePos + 1) % bufferSize;
        
        // Calculate read position (can go backwards for reverse playback)
        readPos += effectiveRate;
        
        // Wrap read position within buffer bounds
        while (readPos < 0)
            readPos += bufferSize;
        while (readPos >= bufferSize)
            readPos -= bufferSize;
        
        // Simple linear interpolation for smoother audio
        int readPos1 = static_cast<int>(readPos);
        int readPos2 = (readPos1 + 1) % bufferSize;
        float frac = static_cast<float>(readPos - readPos1);
        
        // Read from ring buffer with interpolation
        float outL = ringBufferL[readPos1] * (1.0f - frac) + ringBufferL[readPos2] * frac;
        float outR = numChannels >= 2 ? 
                    (ringBufferR[readPos1] * (1.0f - frac) + ringBufferR[readPos2] * frac) : 
                    outL;
        
        // Apply a slight low-pass filter when scratching is intense
        // to simulate vinyl sound
        if (std::abs(currentSpeed) > 0.5f) {
            const float filterCoeff = 0.7f;
            outL = outL * (1.0f - filterCoeff) + lastSampleL * filterCoeff;
            outR = outR * (1.0f - filterCoeff) + lastSampleR * filterCoeff;
            lastSampleL = outL;
            lastSampleR = outR;
        }
        
        // Write output
        if (numChannels >= 1)
            inputBlock.getChannel(0).data.data[i] = outL;
        if (numChannels >= 2)
            inputBlock.getChannel(1).data.data[i] = outR;
    }
}

void ScratchAudioNode::setScratchSpeed(float speed)
{
    scratchSpeed.store(speed, std::memory_order_release);
}

void ScratchAudioNode::setScratchDepth(float depth)
{
    scratchDepth.store(depth, std::memory_order_release);
}

void ScratchAudioNode::setActive(bool active)
{
    isActive.store(active, std::memory_order_release);
} 
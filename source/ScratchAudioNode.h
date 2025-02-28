#pragma once

#include <tracktion_engine/tracktion_engine.h>
#include <tracktion_graph/tracktion_graph.h>

class ScratchAudioNode : public tracktion::graph::Node
{
public:
    ScratchAudioNode(std::unique_ptr<tracktion::graph::Node> inputNode);
    ~ScratchAudioNode() override = default;
    
    tracktion::graph::NodeProperties getNodeProperties() override;
    void prepareToPlay(const tracktion::graph::PlaybackInitialisationInfo&) override;
    bool isReadyToProcess() override;
    void process(ProcessContext&) override;
    
    // Control interface for scratching
    void setScratchSpeed(float speed);
    void setScratchDepth(float depth);
    void setActive(bool isActive);
    
private:
    std::unique_ptr<tracktion::graph::Node> input;
    tracktion::graph::NodeProperties nodeProperties;
    
    // Scratch parameters
    std::atomic<float> scratchSpeed { 0.0f };
    std::atomic<float> scratchDepth { 0.5f };
    std::atomic<bool> isActive { false };
    
    // Audio processing state
    float lastSampleL = 0.0f;
    float lastSampleR = 0.0f;
    
    // Ring buffer for time stretching
    static constexpr int bufferSize = 32768;
    std::vector<float> ringBufferL { bufferSize, 0.0f };
    std::vector<float> ringBufferR { bufferSize, 0.0f };
    int writePos = 0;
    double readPos = 0.0;
    
    // For smooth parameter changes
    juce::SmoothedValue<float> smoothedSpeed { 0.0f };
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ScratchAudioNode)
}; 
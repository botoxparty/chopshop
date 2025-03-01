#pragma once

#include "BaseEffectComponent.h"
#include "RampedValue.h"
#include "ScratchAudioNode.h"

class ScratcherComponent : public BaseEffectComponent,
                           private juce::Timer  // Inherit from Timer directly
{
public:
    explicit ScratcherComponent(tracktion::engine::Edit& edit);
    ~ScratcherComponent() override;
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    
    // Mouse interaction methods
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    
    // Apply scratch effect with given parameters
    void applyScratch(float speed, float depth);
    
    // Reset playback to normal
    void resetPlayback();
    
    // Set callback for when scratching begins/ends
    std::function<void(bool)> onScratchingStateChanged;
    
    // Add this method to create and return a scratch node
    std::unique_ptr<tracktion::graph::Node> createScratchNode(std::unique_ptr<tracktion::graph::Node> inputNode);
    
private:
    // Timer callback implementation
    void timerCallback() override;
    
    juce::Path turntablePath;
    juce::Path recordPath;
    juce::Path groovesPath;
    
    juce::Rectangle<float> turntableArea;
    juce::Point<float> lastDragPosition;
    juce::Point<float> turntableCenter;
    
    float currentAngle = 0.0f;
    float lastAngle = 0.0f;
    float rotationSpeed = 0.0f;
    
    bool isDragging = false;
    bool isScratchActive = false;
    
    juce::Label scratchSpeedLabel;
    juce::Label scratchDepthLabel;
    juce::Slider scratchSpeedSlider;
    juce::Slider scratchDepthSlider;
    
    // Calculate angle from center to point
    float getAngleFromCenter(juce::Point<float> point);
    
    // Update visual rotation
    void updateRotation(float newAngle);
    
    // Draw the turntable graphics
    void drawTurntable(juce::Graphics& g);
    
    // Apply physics to slow down turntable when released
    void applyTurntablePhysics();
    
    // Add a pointer to the scratch node for direct control
    std::shared_ptr<ScratchAudioNode> scratchNode;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScratcherComponent)
}; 
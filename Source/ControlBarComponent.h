#pragma once

#include <JuceHeader.h>
#include "Utilities.h"
#include "CustomLookAndFeel.h"

class ControlBarComponent : public juce::Component, private juce::Timer
{
public:
    ControlBarComponent(tracktion_engine::Edit& edit);
    ~ControlBarComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    
    void updatePositionLabel();
    
    // Callback functions
    std::function<void()> onPlayButtonClicked;
    std::function<void()> onStopButtonClicked;
    
    // Public methods to update UI state
    void setPlayButtonState(bool isPlaying);
    void setStopButtonState(bool isStopped);
    void setTrackName(const juce::String& name);
    
    // Accessor methods for UI components
    juce::Label& getCurrentTrackLabel() { return currentTrackLabel; }
    juce::Label& getPositionLabel() { return positionLabel; }
    juce::TextButton& getPlayButton() { return playButton; }
    juce::TextButton& getStopButton() { return stopButton; }

private:
    void setupButton(juce::TextButton& button, const juce::String& text, juce::Colour baseColour);
    void timerCallback() override;
    
    tracktion_engine::Edit& edit;
    
    juce::Label currentTrackLabel;
    juce::Label positionLabel;
    
    juce::TextButton playButton;
    juce::TextButton stopButton;
    
    float pulsePhase = 0.0f; // For animation effects
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ControlBarComponent)
}; 
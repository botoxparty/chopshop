#pragma once

#include <JuceHeader.h>


class ReverbComponent : public juce::Component
{
public:
    explicit ReverbComponent(tracktion_engine::Edit&);
    void resized() override;

private:
    juce::Slider reverbRoomSizeSlider;
    juce::Slider reverbWetSlider;
    tracktion_engine::Plugin::Ptr reverbPlugin;
    
    // Add labels
    juce::Label roomSizeLabel;
    juce::Label wetLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ReverbComponent)
}; 
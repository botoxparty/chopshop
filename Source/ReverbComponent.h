#pragma once

#include "BaseEffectComponent.h"

class ReverbComponent : public BaseEffectComponent
{
public:
    explicit ReverbComponent(tracktion_engine::Edit&);
    void resized() override;

private:
    juce::Slider reverbRoomSizeSlider;
    juce::Slider reverbWetSlider;
    
    juce::Label roomSizeLabel;
    juce::Label wetLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ReverbComponent)
}; 
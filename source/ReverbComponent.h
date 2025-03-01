#pragma once

#include "BaseEffectComponent.h"
#include "RampedValue.h"

class ReverbComponent : public BaseEffectComponent
{
public:
    explicit ReverbComponent(tracktion::engine::Edit&);
    void resized() override;
    void rampMixLevel(bool rampUp);
    void restoreMixLevel() override;

private:
    juce::Slider reverbRoomSizeSlider;
    juce::Slider reverbWetSlider;
    
    juce::Label roomSizeLabel;
    juce::Label wetLabel;

    RampedValue mixRamp;
    float storedMixValue = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ReverbComponent)
}; 
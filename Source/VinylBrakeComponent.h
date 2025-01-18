#pragma once

#include "BaseEffectComponent.h"

class VinylBrakeComponent : public BaseEffectComponent
{
public:
    explicit VinylBrakeComponent(tracktion_engine::Edit&);
    void resized() override;
    
    void triggerBrakeEffect();

private:
    juce::Slider decayTimeSlider;
    juce::TextButton brakeButton { "Brake" };
    juce::Label decayTimeLabel;
    
    void applyBrakeEffect();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VinylBrakeComponent)
};
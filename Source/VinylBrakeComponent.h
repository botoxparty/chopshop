#pragma once

#include "BaseEffectComponent.h"

class VinylBrakeComponent : public BaseEffectComponent,
                           public juce::Slider::Listener
{
public:
    explicit VinylBrakeComponent(tracktion_engine::Edit&);
    void resized() override;
    void sliderValueChanged(juce::Slider* slider) override;
    
    // Add callback for getting parent's tempo adjustment
    std::function<double()> getCurrentTempoAdjustment;

private:
    juce::Label titleLabel;
    juce::Slider brakeSlider;
    
    double originalTempoAdjustment = 0.0;
    bool hasStoredAdjustment = false;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VinylBrakeComponent)
};
/*
  ==============================================================================

    DelayComponent.h
    Created: 18 Jan 2025 9:27:36am
    Author:  Adam Hammad

  ==============================================================================
*/

#pragma once

#include "BaseEffectComponent.h"
#include "RampedValue.h"
class DelayComponent : public BaseEffectComponent
{
public:
    explicit DelayComponent(tracktion_engine::Edit&);
    void resized() override;
    void setDelayTime(double milliseconds);
    void rampMixLevel(bool rampUp);

private:
    juce::Slider feedbackSlider;
    juce::Slider mixSlider;
    juce::Slider lengthSlider;
    
    juce::Label feedbackLabel;
    juce::Label mixLabel;
    juce::Label lengthLabel;

    RampedValue mixRamp;
    float storedMixValue = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DelayComponent)
};

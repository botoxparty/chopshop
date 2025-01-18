/*
  ==============================================================================

    DelayComponent.h
    Created: 18 Jan 2025 9:27:36am
    Author:  Adam Hammad

  ==============================================================================
*/

#pragma once

#include "BaseEffectComponent.h"

class DelayComponent : public BaseEffectComponent
{
public:
    explicit DelayComponent(tracktion_engine::Edit&);
    void resized() override;

private:
    juce::Slider feedbackSlider;
    juce::Slider mixSlider;
    juce::Slider lengthSlider;
    
    juce::Label feedbackLabel;
    juce::Label mixLabel;
    juce::Label lengthLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DelayComponent)
};

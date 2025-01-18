/*
  ==============================================================================

    FlangerComponent.h
    Created: 17 Jan 2025 10:12:43pm
    Author:  Adam Hammad

  ==============================================================================
*/

#pragma once

#include "BaseEffectComponent.h"

class FlangerComponent : public BaseEffectComponent
{
public:
    explicit FlangerComponent(tracktion_engine::Edit&);
    void resized() override;

private:
    juce::Slider depthSlider;
    juce::Slider speedSlider;
    juce::Slider widthSlider;
    juce::Slider mixSlider;
    
    juce::Label depthLabel;
    juce::Label speedLabel;
    juce::Label widthLabel;
    juce::Label mixLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FlangerComponent)
};

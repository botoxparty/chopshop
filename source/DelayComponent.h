/*
  ==============================================================================

    DelayComponent.h
    Created: 18 Jan 2025 9:27:36am
    Author:  Adam Hammad

  ==============================================================================
*/

#pragma once

#include "BaseEffectComponent.h"
#include "Plugins/AutoDelayPlugin.h"
#include "RampedValue.h"
#include "RotarySliderComponent.h"

class DelayComponent : public BaseEffectComponent
{
public:
    explicit DelayComponent(tracktion::engine::Edit&);
    void resized() override;
    void setDelayTime(double milliseconds);
    void rampMixLevel(bool rampUp);
    void setTempo(double newTempo) { tempo = newTempo; updateDelayTimeFromNote(); }

private:
    RotarySliderComponent feedbackSlider { "Feedback" };
    RotarySliderComponent mixSlider { "Mix" };
    juce::ComboBox noteValueBox;
    juce::Label timeLabel;

    RampedValue mixRamp;
    double storedMixValue = 0.0;
    double tempo = 120.0;

    void updateDelayTimeFromNote();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DelayComponent)
};

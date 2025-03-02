/*
  ==============================================================================

    FlangerComponent.h
    Created: 17 Jan 2025 10:12:43pm
    Author:  Adam Hammad

  ==============================================================================
*/

#pragma once

#include "BaseEffectComponent.h"
#include "Plugins/FlangerPlugin.h"
#include "RampedValue.h"
#include "RotarySliderComponent.h"

class FlangerComponent : public BaseEffectComponent
{
public:
    explicit FlangerComponent(tracktion::engine::Edit&);
    void resized() override;
    void setDepth(float value);
    void setSpeed(float value);
    void setWidth(float value);
    void setMix(float value);
    void rampMixLevel(bool rampUp);

private:
    RotarySliderComponent depthSlider { "Depth" };
    RotarySliderComponent speedSlider { "Speed" };
    RotarySliderComponent widthSlider { "Width" };
    RotarySliderComponent mixSlider { "Mix" };

    RampedValue mixRamp;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FlangerComponent)
};

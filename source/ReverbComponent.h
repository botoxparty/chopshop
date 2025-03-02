#pragma once

#include "BaseEffectComponent.h"
#include "RotarySliderComponent.h"

class ReverbComponent : public BaseEffectComponent
{
public:
    explicit ReverbComponent(tracktion::engine::Edit& edit);
    void resized() override;
    void rampMixLevel(bool rampUp);
    void restoreMixLevel() override;

private:
    RotarySliderComponent roomSizeSlider { "Room Size" };
    RotarySliderComponent wetSlider { "Wet Level" };
    float storedMixValue { 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ReverbComponent)
}; 
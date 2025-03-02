#pragma once

#include "BaseEffectComponent.h"
#include "Plugins/AutoPhaserPlugin.h"
#include "RotarySliderComponent.h"

class PhaserComponent : public BaseEffectComponent
{
public:
    explicit PhaserComponent(tracktion::engine::Edit&);
    void resized() override;
    void setDepth(float value);
    void setRate(float value);
    void setFeedback(float value);

private:
    RotarySliderComponent depthSlider { "Depth" };
    RotarySliderComponent rateSlider { "Rate" };
    RotarySliderComponent feedbackSlider { "Feedback" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PhaserComponent)
}; 
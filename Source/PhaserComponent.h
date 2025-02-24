#pragma once

#include "BaseEffectComponent.h"
#include "Plugins/AutoPhaserPlugin.h"

class PhaserComponent : public BaseEffectComponent
{
public:
    explicit PhaserComponent(tracktion_engine::Edit&);
    void resized() override;
    void setDepth(float value);
    void setRate(float value);
    void setFeedback(float value);

private:
    juce::Slider depthSlider;
    juce::Slider rateSlider;
    juce::Slider feedbackSlider;
    juce::Slider mixSlider;
    
    juce::Label depthLabel;
    juce::Label rateLabel;
    juce::Label feedbackLabel;
    juce::Label mixLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PhaserComponent)
}; 
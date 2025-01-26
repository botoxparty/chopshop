#pragma once

#include "BaseEffectComponent.h"

class ChopComponent : public BaseEffectComponent
{
public:
    explicit ChopComponent(tracktion_engine::Edit&);
    void resized() override;
    
    std::function<void()> onChopButtonPressed;
    std::function<void()> onChopButtonReleased;
    std::function<void(float)> onCrossfaderValueChanged;

    double getChopDurationInMs(double currentTempo) const;
    float getCrossfaderValue() const { return crossfaderSlider.getValue(); }
    void setCrossfaderValue(float value) { crossfaderSlider.setValue(value, juce::sendNotification); }

private:
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    juce::TextButton chopButton{"Chop"};
    juce::ComboBox chopDurationComboBox;
    juce::Label durationLabel;
    juce::Slider crossfaderSlider;
    juce::Label crossfaderLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChopComponent)
}; 
#pragma once

#include "BaseEffectComponent.h"
#include "Utilities.h"

class VinylBrakeComponent : public BaseEffectComponent,
                           public juce::Slider::Listener,
                           public juce::Timer
{
public:
    explicit VinylBrakeComponent(tracktion::engine::Edit&);
    void resized() override;
    void sliderValueChanged(juce::Slider* slider) override;
    void timerCallback() override;
    
    // Add callback for getting parent's tempo adjustment
    std::function<double()> getCurrentTempoAdjustment;

    void setBrakeValue(double value)
    {
        if (!isSpringAnimating)  // Only set value if not currently animating
        {
            brakeSlider.setValue(value, juce::sendNotification);
        }
    }

    double getBrakeValue() const
    {
        return brakeSlider.getValue();
    }

    void startSpringAnimation();

    std::function<double()> getEffectiveTempo;

private:
    class SpringSlider : public juce::Slider
    {
    public:
        SpringSlider() : juce::Slider() {}
        
        void mouseUp(const juce::MouseEvent& event) override
        {
            if (VinylBrakeComponent* parent = dynamic_cast<VinylBrakeComponent*>(getParentComponent()))
                parent->startSpringAnimation();
                
            juce::Slider::mouseUp(event);
        }
    };
    
    SpringSlider brakeSlider;

    void setSpeed(double value);
    
    double originalTempoAdjustment = 0.0;
    bool hasStoredAdjustment = false;
    bool isSpringAnimating = false;
    double currentSpringValue = 0.0;
    double springStartTime = 0.0;
    double springStartValue = 0.0;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VinylBrakeComponent)
};
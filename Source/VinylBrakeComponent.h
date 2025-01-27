#pragma once

#include "BaseEffectComponent.h"

class VinylBrakeComponent : public BaseEffectComponent
{
public:
    explicit VinylBrakeComponent(tracktion_engine::Edit&);
    void resized() override;
    
    void triggerBrakeEffect();

private:
    struct BrakeTimer : public juce::Timer
    {
        BrakeTimer(tracktion_engine::Edit& e, double startTempo, double decayTime);
        void timerCallback() override;
        void startRelease();
        
        tracktion_engine::Edit& edit;
        double initialTempo;
        double decayTimeMs;
        double releaseTimeMs;
        juce::uint32 startTime;
        bool isReleasing;
        double currentTempoMultiplier = 1.0;
    };

    void mouseDown(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;

    juce::Slider decayTimeSlider;
    juce::TextButton brakeButton { "Brake" };
    juce::Label decayTimeLabel;
    
    BrakeTimer* currentBrakeTimer = nullptr;
    
    void applyBrakeEffect();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VinylBrakeComponent)
};
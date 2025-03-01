#pragma once

#include "BaseEffectComponent.h"

class ScrewComponent : public BaseEffectComponent
{
public:
    explicit ScrewComponent(tracktion::engine::Edit&);
    void resized() override;
    
    std::function<void(double)> onTempoChanged;
    std::function<void(double)> onTempoPercentageChanged;
    
    void setTempo(double tempo, juce::NotificationType notification = juce::sendNotification);
    void setTempoPercentage(double percentage);
    double getTempo() const { return tempoSlider.getValue(); }
    
    void setBaseTempo(double tempo);
    
private:
    juce::Slider tempoSlider;
    juce::TextButton tempo70Button{"70%"};
    juce::TextButton tempo75Button{"75%"};
    juce::TextButton tempo80Button{"80%"};
    juce::TextButton tempo85Button{"85%"};
    juce::TextButton tempo100Button{"100%"};
    
    double baseTempo = 120.0;
    
    void updateTempoButtonStates();
    bool isTempoPercentageActive(double percentage) const;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScrewComponent)
}; 
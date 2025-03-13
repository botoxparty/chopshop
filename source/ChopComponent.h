#pragma once

#include "BaseEffectComponent.h"
#include "ChopTrackLane.h"

class ChopComponent : public BaseEffectComponent,
                     public juce::Timer
{
public:
    explicit ChopComponent(tracktion::engine::Edit&);
    void resized() override;
    
    std::function<double()> getTempoCallback;

    double getChopDurationInBeats() const;
    ~ChopComponent() override;

    void handleChopButtonPressed();
    void handleChopButtonReleased();

    // Timer callback
    void timerCallback() override;

private:
    juce::Label durationLabel;
    juce::ComboBox chopDurationComboBox;
    juce::TextButton chopButton;
    tracktion::engine::AudioTrack::Ptr chopTrack;

    double chopStartTime = 0.0;
    double chopReleaseDelay = 0.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChopComponent)
}; 
#pragma once

#include <juce_core/juce_core.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <tracktion_engine/tracktion_engine.h>

#include "Utilities.h"

class TransportComponent : public juce::Component,
                         public juce::Timer,
                         public juce::ChangeListener
{
public:
    TransportComponent(tracktion::engine::Edit& e);
    ~TransportComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void changeListenerCallback(juce::ChangeBroadcaster*) override;
    void timerCallback() override;

private:
    tracktion::engine::Edit& edit;
    
    // Transport controls
    juce::TextButton playButton{"Play"};
    juce::TextButton stopButton{"Stop"};
    juce::TextButton recordButton{"Record"};
    juce::TextButton loopButton{"Loop"};
    
    // Timeline and position display
    juce::Label timeDisplay;
    std::unique_ptr<juce::DrawableRectangle> playhead;
    
    // Waveform thumbnail
    tracktion::SmartThumbnail thumbnail;
    
    void updateTimeDisplay();
    void updatePlayheadPosition();
    void updateTransportState();
    void updateThumbnail();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TransportComponent)
}; 
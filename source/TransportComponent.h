#pragma once

#include <juce_core/juce_core.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <tracktion_engine/tracktion_engine.h>

#include "Utilities.h"
#include "AutomationLane.h"
#include "CustomLookAndFeel.h"
#include "Plugins/ChopPlugin.h"
#include "CrossfaderAutomationLane.h"
class TransportComponent : public juce::Component,
                         public juce::Timer,
                         public juce::ChangeListener,
                         public tracktion::engine::AutomationRecordManager::Listener
{
public:
    TransportComponent(tracktion::engine::Edit& e);
    ~TransportComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void changeListenerCallback(juce::ChangeBroadcaster*) override;
    void timerCallback() override;
    void mouseDown(juce::MouseEvent const& event) override;
    void mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) override;
    
    void updateThumbnail();
    void automationModeChanged() override;

private:
    tracktion::engine::Edit& edit;
    tracktion::engine::TransportControl& transport;
    
    // Transport controls
    juce::TextButton playButton{"Play"};
    juce::TextButton stopButton{"Stop"};
    juce::TextButton loopButton{"Loop"};
    juce::TextButton automationReadButton{"Auto Read"};
    juce::TextButton automationWriteButton{"Auto Write"};
    
    // Zoom controls
    juce::TextButton zoomInButton{"+"}; 
    juce::TextButton zoomOutButton{"-"};
    
    // Timeline and position display
    juce::Label timeDisplay;
    std::unique_ptr<juce::DrawableRectangle> playhead;
    
    // Waveform thumbnail
    tracktion::engine::SmartThumbnail thumbnail;
    
    std::unique_ptr<AutomationLane> automationLane;
    std::unique_ptr<CrossfaderAutomationLane> crossfaderAutomationLane;
    
    // Zoom and scroll state
    double zoomLevel = 1.0;
    double scrollPosition = 0.0; // 0.0 to 1.0
    static constexpr double minZoom = 1.0; // Changed from 0.1 to 1.0 (100%)
    static constexpr double maxZoom = 10.0;

    // Current clip reference
    tracktion::engine::WaveAudioClip* currentClip = nullptr;
    
    void updateTimeDisplay();
    void updatePlayheadPosition();
    void updateTransportState();
    void setZoomLevel(double newLevel);
    void setScrollPosition(double newPosition);
    double getMaxScrollPosition() const;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TransportComponent)
}; 
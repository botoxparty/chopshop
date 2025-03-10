#pragma once

#include <juce_core/juce_core.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_events/juce_events.h>
#include <tracktion_engine/tracktion_engine.h>

#include "Utilities.h"
#include "Plugins/ChopPlugin.h"
#include "ZoomState.h"

class ThumbnailComponent : public juce::Component,
                          public juce::Timer,
                          public juce::ChangeListener,
                          public ZoomStateListener
{
public:
    ThumbnailComponent(tracktion::engine::Edit&, ZoomState&);
    ~ThumbnailComponent() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;
    void changeListenerCallback(juce::ChangeBroadcaster*) override;
    
    // ZoomStateListener implementation
    void zoomLevelChanged(double newZoomLevel) override;
    void scrollPositionChanged(double newScrollPosition) override;
    
    void mouseDown(const juce::MouseEvent&) override;
    void mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    
    void updateThumbnail();

private:
    tracktion::engine::Edit& edit;
    tracktion::engine::TransportControl& transport;
    tracktion::engine::SmartThumbnail thumbnail;
    tracktion::engine::WaveAudioClip* currentClip;
    
    ZoomState& zoomState;
    std::unique_ptr<juce::DrawableRectangle> playhead;
    
    void updatePlayheadPosition();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ThumbnailComponent)
}; 
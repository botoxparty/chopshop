#pragma once

#include <juce_core/juce_core.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_events/juce_events.h>
#include <tracktion_engine/tracktion_engine.h>

#include "Utilities.h"
#include "Plugins/ChopPlugin.h"

class ThumbnailComponent : public juce::Component,
                          public juce::Timer,
                          public juce::ChangeListener
{
public:
    ThumbnailComponent(tracktion::engine::Edit& e);
    ~ThumbnailComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void changeListenerCallback(juce::ChangeBroadcaster*) override;
    void timerCallback() override;

    void setZoomLevel(double newLevel);
    void setScrollPosition(double newPosition);
    void updateThumbnail();
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) override;

    double getMaxScrollPosition() const;

private:
    tracktion::engine::Edit& edit;
    tracktion::engine::TransportControl& transport;
    tracktion::SmartThumbnail thumbnail;
    tracktion::engine::WaveAudioClip* currentClip;
    std::unique_ptr<juce::DrawableRectangle> playhead;
    
    double zoomLevel;
    double scrollPosition;
    static constexpr double minZoom = 1.0;
    static constexpr double maxZoom = 100.0;

    void updatePlayheadPosition();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ThumbnailComponent)
}; 
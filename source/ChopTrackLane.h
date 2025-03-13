#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <tracktion_engine/tracktion_engine.h>
#include "ZoomState.h"
#include <vector>
#include <optional>
#include <functional>

class ChopTrackLane : public juce::Component,
                      public ZoomStateListener
{
public:
    ChopTrackLane(tracktion::engine::Edit&, ZoomState&);
    ~ChopTrackLane() override;

    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    void mouseDoubleClick(const juce::MouseEvent& event) override;
    void mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) override;

    // Grid and pattern methods
    void setSnapToGrid(bool shouldSnap);
    double snapTimeToGrid(double time) const;
    
    // Clip management
    void clearClips();
    void deleteSelectedClip();
    void addClip(double startTime, double endTime);
    void launchClip(tracktion::engine::Clip* clip);

    // ZoomStateListener implementation
    void zoomLevelChanged(double newZoomLevel) override { repaint(); }
    void scrollPositionChanged(double newScrollPosition) override { repaint(); }
    void gridSizeChanged(float) override { repaint(); }

    // Coordinate conversion helpers
    juce::Point<float> timeToXY(double time, double value) const;
    std::pair<double, double> XYToTime(float x, float y) const;

private:
    tracktion::engine::Edit& edit;
    ZoomState& zoomState;
    tracktion::engine::AudioTrack::Ptr chopTrack;
    bool snapEnabled = true;
    bool isDragging = false;
    tracktion::engine::Clip* selectedClip = nullptr;

    // Drag state tracking
    double dragStartTime = 0.0;
    double dragOffsetTime = 0.0;

    double getSourceLength() const;
    tracktion::engine::AudioTrack::Ptr getOrCreateChopTrack();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChopTrackLane)
}; 
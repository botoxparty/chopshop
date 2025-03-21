#pragma once

#include <juce_core/juce_core.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <tracktion_engine/tracktion_engine.h>
#include "ZoomState.h"
#include "Utilities.h"

class AutomationLane : public juce::Component,
                     public virtual ZoomStateListener,
                     public tracktion::engine::AutomatableParameter::Listener
{
public:
    AutomationLane(tracktion::engine::Edit&, ZoomState&);
    ~AutomationLane() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    
    // ZoomStateListener implementation
    void zoomLevelChanged(double newZoomLevel) override;
    void scrollPositionChanged(double newScrollPosition) override;

    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;
    void mouseDoubleClick(const juce::MouseEvent&) override;

    virtual void setParameter(tracktion::engine::AutomatableParameter*);
    virtual void updatePoints();
    
    double getSourceLength() const 
    { 
        auto clip = EngineHelpers::getCurrentClip(edit);
        if (clip != nullptr)
            return clip->getPosition().getLength().inSeconds();

        DBG("No clip found, returning default length");
        return 60.0; // Default length if no clip
    }

    // AutomatableParameter::Listener overrides
    void curveHasChanged(tracktion::engine::AutomatableParameter&) override;
    void currentValueChanged(tracktion::engine::AutomatableParameter&) override;
    void parameterChanged(tracktion::engine::AutomatableParameter&, float) override;
    void parameterChangeGestureBegin(tracktion::engine::AutomatableParameter&) override;
    void parameterChangeGestureEnd(tracktion::engine::AutomatableParameter&) override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AutomationLane)

protected:
    tracktion::engine::Edit& edit;
    tracktion::engine::AutomatableParameter* parameter;
    tracktion::engine::WaveAudioClip* currentClip = nullptr;
    std::vector<std::pair<double, double>> automationPoints; // <time, value> pairs
    
    ZoomState& zoomState;
    
    int draggedPointIndex = -1;
    static constexpr float pointHitRadius = 5.0f; // Radius in pixels for hit detection
    int findPointNear(float x, float y) const;

    // Debounce timer to prevent rapid updates
    juce::int64 lastUpdateTime = 0;
    static constexpr juce::int64 minimumUpdateInterval = 50; // milliseconds

    juce::Point<float> timeToXY(double timeInSeconds, double value) const;
    std::pair<double, double> XYToTime(float x, float y) const;
    void addPoint(double timeInSeconds, double value);
    void updateValueAtTime(double timeInSeconds, double value);

    virtual void onParameterChanged(tracktion::engine::AutomatableParameter*) {}
}; 
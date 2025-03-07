#pragma once

#include <juce_core/juce_core.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <tracktion_engine/tracktion_engine.h>

class AutomationLane : public juce::Component,
                     public tracktion::engine::AutomatableParameter::Listener
{
public:
    AutomationLane(tracktion::engine::Edit& e, tracktion::engine::AutomatableParameter* param = nullptr);
    ~AutomationLane() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;

    void setParameter(tracktion::engine::AutomatableParameter* param);
    void updatePoints();

    // AutomatableParameter::Listener methods
    void curveHasChanged(tracktion::engine::AutomatableParameter&) override;
    void currentValueChanged(tracktion::engine::AutomatableParameter&) override;
    void parameterChanged(tracktion::engine::AutomatableParameter&, float) override;
    void parameterChangeGestureBegin(tracktion::engine::AutomatableParameter&) override;
    void parameterChangeGestureEnd(tracktion::engine::AutomatableParameter&) override;

private:
    tracktion::engine::Edit& edit;
    tracktion::engine::AutomatableParameter* parameter;
    std::vector<std::pair<double, double>> automationPoints; // <time, value> pairs
    
    juce::Point<float> timeToXY(double timeInSeconds, double value) const;
    std::pair<double, double> XYToTime(float x, float y) const;
    void addPoint(double timeInSeconds, double value);
    void updateValueAtTime(double timeInSeconds, double value);

    // Debounce timer to prevent rapid updates
    juce::int64 lastUpdateTime = 0;
    static constexpr juce::int64 minimumUpdateInterval = 50; // milliseconds

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AutomationLane)
}; 
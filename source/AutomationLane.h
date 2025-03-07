#pragma once

#include <juce_core/juce_core.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <tracktion_engine/tracktion_engine.h>

class AutomationLane : public juce::Component
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

private:
    tracktion::engine::Edit& edit;
    tracktion::engine::AutomatableParameter* parameter;
    std::vector<std::pair<double, double>> automationPoints; // <time, value> pairs
    
    juce::Point<float> timeToXY(double timeInSeconds, double value) const;
    std::pair<double, double> XYToTime(float x, float y) const;
    void addPoint(double timeInSeconds, double value);
    void updateValueAtTime(double timeInSeconds, double value);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AutomationLane)
}; 
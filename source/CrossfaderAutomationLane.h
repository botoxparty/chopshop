#pragma once

#include "AutomationLane.h"
#include <vector>

class CrossfaderAutomationLane : public AutomationLane
{
public:
    struct ChopRegion {
        double startTime;
        double endTime;
        bool isASide;
        float mixAmount;  // 0.0 = full B, 1.0 = full A

        ChopRegion(double start, double end, bool a, float mix = 1.0f)
            : startTime(start), endTime(end), isASide(a), mixAmount(mix) {}
    };

    CrossfaderAutomationLane(tracktion::engine::Edit& e, tracktion::engine::AutomatableParameter* param = nullptr);
    ~CrossfaderAutomationLane() override;

    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    void updatePoints() override;

    // AutomatableParameter::Listener overrides
    void curveHasChanged(tracktion::engine::AutomatableParameter&) override;
    void currentValueChanged(tracktion::engine::AutomatableParameter&) override;
    void parameterChanged(tracktion::engine::AutomatableParameter&, float) override;

    // Grid and pattern methods
    void setGridDivision(float division);
    void setSnapToGrid(bool shouldSnap);
    double snapTimeToGrid(double time) const;
    
    // Chop region management
    void addChopRegion(const ChopRegion& region);
    void removeChopRegion(size_t index);
    void clearChopRegions();
    
    // Pattern methods
    void applyAlternatingPattern(double startTime, double endTime, double interval);
    void duplicatePattern(double startTime, double endTime, double targetTime);

private:
    std::vector<ChopRegion> chopRegions;
    float gridDivision = 0.25f;  // Default to quarter notes
    bool snapEnabled = true;
    
    // For drag operations
    bool isDragging = false;
    ChopRegion* activeRegion = nullptr;
    
    // Helper methods
    ChopRegion* getRegionAtTime(double time);
    void updateAutomationPoints();
    void convertRegionsToAutomation();
    void updateChopRegionsFromCurve();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CrossfaderAutomationLane)
}; 
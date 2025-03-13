#pragma once

#include "AutomationLane.h"
#include <vector>
#include <optional>
#include "ZoomState.h"
#include "RegionManager.h"
class CrossfaderAutomationLane : public AutomationLane
{
public:
    CrossfaderAutomationLane(tracktion::engine::Edit&, ZoomState&);
    ~CrossfaderAutomationLane() override;

    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    void mouseDoubleClick(const juce::MouseEvent& event) override;
    void updatePoints() override;
    void mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) override;

    // AutomatableParameter::Listener overrides
    void curveHasChanged(tracktion::engine::AutomatableParameter&) override;
    void currentValueChanged(tracktion::engine::AutomatableParameter&) override;
    void parameterChanged(tracktion::engine::AutomatableParameter&, float) override;

    // Grid and pattern methods
    void setSnapToGrid(bool shouldSnap);
    double snapTimeToGrid(double time) const;
    
    // Chop region management
    void clearChopRegions();

    void deleteSelectedRegion();

    // ZoomStateListener implementation
    void zoomLevelChanged(double newZoomLevel) override { repaint(); }
    void scrollPositionChanged(double newScrollPosition) override { repaint(); }
    void gridSizeChanged(float) override;
    void setParameter(tracktion::engine::AutomatableParameter* param) override;
    using Region = RegionManager::Region;
protected:
    void onParameterChanged(tracktion::engine::AutomatableParameter* param) override;
private:
    std::unique_ptr<RegionManager> regionManager;
    bool snapEnabled = true;
    bool isDragging = false;
    std::optional<size_t> selectedRegionIndex;
    void updateAutomationPoints();
    void convertRegionsToAutomation();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CrossfaderAutomationLane)
}; 
#include "CrossfaderAutomationLane.h"

CrossfaderAutomationLane::CrossfaderAutomationLane(tracktion::engine::Edit& e, ZoomState& zs)
    : AutomationLane(e, zs)
{
}

CrossfaderAutomationLane::~CrossfaderAutomationLane()
{
    clearChopRegions();
}

void CrossfaderAutomationLane::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    auto& tempoSequence = edit.tempoSequence;
    
    // Draw background
    g.setColour(juce::Colours::darkgrey);
    g.fillRect(bounds);
    
    // Draw grid lines
    g.setColour(juce::Colours::grey.withAlpha(0.5f));
    
    // Calculate visible beat range
    auto visibleTimeStart = getSourceLength() * zoomState.getScrollPosition();
    auto visibleTimeStartBeats = tempoSequence.toBeats(tracktion::TimePosition::fromSeconds(visibleTimeStart));
    auto visibleTimeEnd = visibleTimeStart + (getSourceLength() / zoomState.getZoomLevel());
    auto visibleTimeEndBeats = tempoSequence.toBeats(tracktion::TimePosition::fromSeconds(visibleTimeEnd));
    
    // Draw vertical grid lines based on gridDivision
    double gridTimeBeats = std::floor(visibleTimeStartBeats.inBeats() / gridDivision) * gridDivision;
    while (gridTimeBeats <= visibleTimeEndBeats.inBeats())
    {
        auto gridTimeSeconds = tempoSequence.toTime(tracktion::BeatPosition::fromBeats(gridTimeBeats)).inSeconds();
        auto xPos = timeToXY(gridTimeSeconds, 0.0).x;
        g.drawVerticalLine(static_cast<int>(xPos), bounds.getY(), bounds.getBottom());
        gridTimeBeats += gridDivision;
    }
    
    // Draw only A-side chop regions
    for (size_t i = 0; i < chopRegions.size(); ++i)
    {
        const auto& region = chopRegions[i];
        if (!region.isASide) continue; // Skip non-A-side regions
        
        // Convert region times through beats for proper tempo handling
        auto startBeats = tempoSequence.toBeats(tracktion::TimePosition::fromSeconds(region.startTime));
        auto endBeats = tempoSequence.toBeats(tracktion::TimePosition::fromSeconds(region.endTime));
        
        auto startTimeSeconds = tempoSequence.toTime(startBeats).inSeconds();
        auto endTimeSeconds = tempoSequence.toTime(endBeats).inSeconds();
        
        auto startPoint = timeToXY(startTimeSeconds, 1.0);
        auto endPoint = timeToXY(endTimeSeconds, 1.0);

        DBG("region.startTime: " << startTimeSeconds << " region.endTime: " << endTimeSeconds);
        
        // Calculate rectangle bounds for A-side
        juce::Rectangle<float> regionBounds(
            startPoint.x,
            bounds.getY(),
            endPoint.x - startPoint.x,
            bounds.getHeight()
        );
        
        // Draw region with different color if selected
        g.setColour(selectedRegionIndex && *selectedRegionIndex == i 
            ? juce::Colours::orangered 
            : juce::Colours::orange);
        g.fillRect(regionBounds);
        
        // Draw border with different color if selected
        g.setColour(selectedRegionIndex && *selectedRegionIndex == i 
            ? juce::Colours::white 
            : juce::Colours::white.withAlpha(0.5f));
        g.drawRect(regionBounds, 1.0f);
    }
}

void CrossfaderAutomationLane::mouseDown(const juce::MouseEvent& event)
{
    if (parameter == nullptr)
        return;
        
    auto [time, value] = XYToTime(event.position.x, event.position.y);
    
    // Check if we clicked on a region
    selectedRegionIndex = std::nullopt;
    for (size_t i = 0; i < chopRegions.size(); ++i)
    {
        const auto& region = chopRegions[i];
        if (time >= region.startTime && time <= region.endTime)
        {
            selectedRegionIndex = i;
            repaint();
            return;
        }
    }
    
    // If we didn't click on a region, add a new point
    if (snapEnabled)
        time = snapTimeToGrid(time);
        
    auto& curve = parameter->getCurve();
    
    // If there are existing points, ensure we terminate the last region
    if (curve.getNumPoints() > 0)
    {
        auto lastPoint = curve.getPoint(curve.getNumPoints() - 1);
        if (lastPoint.value > 0.5f)  // If last point was A-side
        {
            // Add a small gap before the new region
            curve.addPoint(tracktion::TimePosition::fromSeconds(time - 0.001), 0.0f, 0.0f);
        }
    }
    
    // Add the new point for the region
    curve.addPoint(tracktion::TimePosition::fromSeconds(time), 1.0f, 0.0f);
    
    isDragging = true;
    updateChopRegionsFromCurve();
    repaint();
}

void CrossfaderAutomationLane::mouseDrag(const juce::MouseEvent& event)
{
    if (!isDragging || parameter == nullptr)
        return;
        
    auto [time, value] = XYToTime(event.position.x, event.position.y);
    if (snapEnabled)
        time = snapTimeToGrid(time);
    
    auto& curve = parameter->getCurve();
    
    int numPoints = curve.getNumPoints();
    if (numPoints >= 2)
    {
        // Move the end point and its terminator
        curve.movePoint(numPoints - 2, tracktion::TimePosition::fromSeconds(time), 1.0f, false);
        curve.movePoint(numPoints - 1, tracktion::TimePosition::fromSeconds(time + 0.001), 0.0f, false);
    }
    
    updateChopRegionsFromCurve();
    repaint();
}

void CrossfaderAutomationLane::mouseUp(const juce::MouseEvent&)
{
    isDragging = false;
    activeRegion = nullptr;
}

void CrossfaderAutomationLane::setGridDivision(float division)
{
    gridDivision = division;
    repaint();
}

void CrossfaderAutomationLane::setSnapToGrid(bool shouldSnap)
{
    snapEnabled = shouldSnap;
}

double CrossfaderAutomationLane::snapTimeToGrid(double time) const
{
    return std::round(time / gridDivision) * gridDivision;
}

void CrossfaderAutomationLane::addChopRegion(const ChopRegion& region)
{
    if (parameter == nullptr)
        return;

    // Add points directly to automation curve
    auto& curve = parameter->getCurve();
    
    if (region.startTime > 0)
        curve.addPoint(tracktion::TimePosition::fromSeconds(region.startTime - 0.001), 
                      region.isASide ? 0.0f : 1.0f, 0.0f);
                      
    curve.addPoint(tracktion::TimePosition::fromSeconds(region.startTime), 
                  region.isASide ? 1.0f : 0.0f, 0.0f);
    curve.addPoint(tracktion::TimePosition::fromSeconds(region.endTime), 
                  region.isASide ? 1.0f : 0.0f, 0.0f);
    
    if (region.endTime < getSourceLength())
        curve.addPoint(tracktion::TimePosition::fromSeconds(region.endTime + 0.001),
                      region.isASide ? 0.0f : 1.0f, 0.0f);

    // Regions will be updated from curve
    updateChopRegionsFromCurve();
    repaint();
}

void CrossfaderAutomationLane::removeChopRegion(size_t index)
{
    if (index >= chopRegions.size() || parameter == nullptr)
        return;

    const auto& regionToRemove = chopRegions[index];
    auto& curve = parameter->getCurve();

    // Find and remove points within the region's time range
    for (int i = curve.getNumPoints() - 1; i >= 0; --i)
    {
        auto point = curve.getPoint(i);
        auto time = point.time.inSeconds();
        if (time >= regionToRemove.startTime - 0.002 && time <= regionToRemove.endTime + 0.002)
        {
            curve.removePoint(i);
        }
    }
}

void CrossfaderAutomationLane::clearChopRegions()
{
    if (parameter == nullptr)
        return;
        
    // Clear automation curve
    parameter->getCurve().clear();
    chopRegions.clear();
    repaint();
}

CrossfaderAutomationLane::ChopRegion* CrossfaderAutomationLane::getRegionAtTime(double time)
{
    for (auto& region : chopRegions)
    {
        if (time >= region.startTime && time <= region.endTime)
            return &region;
    }
    return nullptr;
}

void CrossfaderAutomationLane::updateChopRegionsFromCurve()
{
    if (parameter == nullptr)
        return;

    auto& curve = parameter->getCurve();
    chopRegions.clear();

    // Convert curve points to regions
    bool isInRegion = false;
    double regionStart = 0.0;
    bool currentSide = false;

    for (int i = 0; i < curve.getNumPoints(); ++i)
    {
        auto point = curve.getPoint(i);
        double time = point.time.inSeconds();
        bool isASide = point.value > 0.5f;

        if (!isInRegion && i < curve.getNumPoints() - 1)
        {
            isInRegion = true;
            regionStart = time;
            currentSide = isASide;
        }
        else if (isInRegion && (isASide != currentSide || i == curve.getNumPoints() - 1))
        {
            // End current region
            chopRegions.emplace_back(regionStart, time, currentSide);
            // Start new region
            regionStart = time;
            currentSide = isASide;
        }
    }
}

void CrossfaderAutomationLane::curveHasChanged(tracktion::engine::AutomatableParameter& param)
{
    // AutomationLane::curveHasChanged(param);
    // Update our regions from the curve
    // updateChopRegionsFromCurve();
    updateChopRegionsFromCurve();
    repaint();
}

void CrossfaderAutomationLane::currentValueChanged(tracktion::engine::AutomatableParameter& param)
{
    AutomationLane::currentValueChanged(param);
    repaint();
}

void CrossfaderAutomationLane::parameterChanged(tracktion::engine::AutomatableParameter& param, float newValue)
{
    AutomationLane::parameterChanged(param, newValue);
    repaint();
}

void CrossfaderAutomationLane::updatePoints()
{
    // Call base class implementation to handle basic automation point updates
    // AutomationLane::updatePoints();
    DBG("updatePoints");
    
    // Update our visual representation based on the current automation data
    updateChopRegionsFromCurve();
    repaint();
}

void CrossfaderAutomationLane::deleteSelectedRegion()
{
    if (selectedRegionIndex)
    {
        removeChopRegion(*selectedRegionIndex);
        selectedRegionIndex = std::nullopt;
        repaint();
    }
} 
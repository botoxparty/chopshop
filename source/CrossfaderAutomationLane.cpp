#include "CrossfaderAutomationLane.h"

CrossfaderAutomationLane::CrossfaderAutomationLane(tracktion::engine::Edit& e, tracktion::engine::AutomatableParameter* param)
    : AutomationLane(e, param)
{
}

CrossfaderAutomationLane::~CrossfaderAutomationLane()
{
    clearChopRegions();
}

void CrossfaderAutomationLane::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    
    // Draw background
    g.setColour(juce::Colours::darkgrey);
    g.fillRect(bounds);
    
    // Draw grid lines
    g.setColour(juce::Colours::grey.withAlpha(0.5f));
    
    // Calculate visible time range
    auto visibleTimeStart = sourceLength * scrollPosition;
    auto visibleTimeEnd = visibleTimeStart + (sourceLength / zoomLevel);
    
    // Draw vertical grid lines based on gridDivision
    double gridTime = std::floor(visibleTimeStart / gridDivision) * gridDivision;
    while (gridTime <= visibleTimeEnd)
    {
        auto xPos = timeToXY(gridTime, 0.0).x;
        g.drawVerticalLine(static_cast<int>(xPos), bounds.getY(), bounds.getBottom());
        gridTime += gridDivision;
    }
    
    // Draw horizontal center line
    g.setColour(juce::Colours::grey);
    g.drawHorizontalLine(static_cast<int>(bounds.getCentreY()), bounds.getX(), bounds.getRight());
    
    // Draw chop regions
    for (const auto& region : chopRegions)
    {
        auto startPoint = timeToXY(region.startTime, region.isASide ? 1.0 : 0.0);
        auto endPoint = timeToXY(region.endTime, region.isASide ? 1.0 : 0.0);
        
        // Calculate rectangle bounds
        juce::Rectangle<float> regionBounds;
        if (region.isASide)
        {
            regionBounds = { startPoint.x, bounds.getY(),
                           endPoint.x - startPoint.x,
                           bounds.getCentreY() - bounds.getY() };
        }
        else
        {
            regionBounds = { startPoint.x, bounds.getCentreY(),
                           endPoint.x - startPoint.x,
                           bounds.getBottom() - bounds.getCentreY() };
        }
        
        // Draw region
        g.setColour(region.isASide ? juce::Colours::orange : juce::Colours::blue);
        g.fillRect(regionBounds);
        
        // Draw border
        g.setColour(juce::Colours::white.withAlpha(0.5f));
        g.drawRect(regionBounds, 1.0f);
    }
}

void CrossfaderAutomationLane::mouseDown(const juce::MouseEvent& event)
{
    if (parameter == nullptr)
        return;
        
    auto [time, value] = XYToTime(event.position.x, event.position.y);
    if (snapEnabled)
        time = snapTimeToGrid(time);
        
    // Check if we clicked on an existing region
    activeRegion = getRegionAtTime(time);
    
    if (activeRegion == nullptr)
    {
        // Create new region
        bool isASide = value >= 0.5;  // Above center line = A side
        chopRegions.emplace_back(time, time, isASide);
        activeRegion = &chopRegions.back();
    }
    
    isDragging = true;
}

void CrossfaderAutomationLane::mouseDrag(const juce::MouseEvent& event)
{
    if (!isDragging || activeRegion == nullptr)
        return;
        
    auto [time, value] = XYToTime(event.position.x, event.position.y);
    if (snapEnabled)
        time = snapTimeToGrid(time);
        
    // Update region end time
    activeRegion->endTime = time;
    activeRegion->isASide = value >= 0.5;  // Allow changing sides during drag
    
    convertRegionsToAutomation();
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
    chopRegions.push_back(region);
    convertRegionsToAutomation();
    repaint();
}

void CrossfaderAutomationLane::removeChopRegion(size_t index)
{
    if (index < chopRegions.size())
    {
        chopRegions.erase(chopRegions.begin() + index);
        convertRegionsToAutomation();
        repaint();
    }
}

void CrossfaderAutomationLane::clearChopRegions()
{
    chopRegions.clear();
    convertRegionsToAutomation();
    repaint();
}

void CrossfaderAutomationLane::applyAlternatingPattern(double startTime, double endTime, double interval)
{
    clearChopRegions();
    
    bool isASide = true;
    for (double time = startTime; time < endTime; time += interval)
    {
        chopRegions.emplace_back(time, time + interval, isASide);
        isASide = !isASide;
    }
    
    convertRegionsToAutomation();
    repaint();
}

void CrossfaderAutomationLane::duplicatePattern(double startTime, double endTime, double targetTime)
{
    std::vector<ChopRegion> patternRegions;
    double patternLength = endTime - startTime;
    
    // Collect regions within the pattern bounds
    for (const auto& region : chopRegions)
    {
        if (region.startTime >= startTime && region.endTime <= endTime)
        {
            ChopRegion newRegion = region;
            newRegion.startTime = targetTime + (region.startTime - startTime);
            newRegion.endTime = targetTime + (region.endTime - startTime);
            patternRegions.push_back(newRegion);
        }
    }
    
    // Add duplicated regions
    chopRegions.insert(chopRegions.end(), patternRegions.begin(), patternRegions.end());
    convertRegionsToAutomation();
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

void CrossfaderAutomationLane::convertRegionsToAutomation()
{
    if (parameter == nullptr)
        return;
        
    auto& curve = parameter->getCurve();
    curve.clear();
    
    // Sort regions by start time
    std::sort(chopRegions.begin(), chopRegions.end(),
              [](const ChopRegion& a, const ChopRegion& b) {
                  return a.startTime < b.startTime;
              });
    
    // Convert regions to automation points
    for (const auto& region : chopRegions)
    {
        float value = region.isASide ? 1.0f : 0.0f;
        
        // Add points for sharp transitions
        if (region.startTime > 0)
            curve.addPoint(tracktion::TimePosition::fromSeconds(region.startTime - 0.001), 
                         value == 1.0f ? 0.0f : 1.0f, 0.0f);
                         
        curve.addPoint(tracktion::TimePosition::fromSeconds(region.startTime), value, 0.0f);
        curve.addPoint(tracktion::TimePosition::fromSeconds(region.endTime), value, 0.0f);
        
        if (region.endTime < sourceLength)
            curve.addPoint(tracktion::TimePosition::fromSeconds(region.endTime + 0.001),
                         value == 1.0f ? 0.0f : 1.0f, 0.0f);
    }
    
    updatePoints();
}

void CrossfaderAutomationLane::updatePoints()
{
    // AutomationLane::updatePoints(); // Call base class implementation
    // convertRegionsToAutomation();   // Update our chop regions
    repaint();                      // Trigger redraw
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
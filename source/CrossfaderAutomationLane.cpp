#include "CrossfaderAutomationLane.h"
#include "Utilities.h"
CrossfaderAutomationLane::CrossfaderAutomationLane(tracktion::engine::Edit& e, ZoomState& zs)
    : AutomationLane(e, zs)
{
    // Register as a zoom state listener
    zoomState.addListener(this);

    auto chopTrack = EngineHelpers::getChopTrack(edit);
    if (chopTrack == nullptr)
    {
        DBG("No chop track found");
        return;
    } else {
        DBG("Chop track found");
    }
    
}

CrossfaderAutomationLane::~CrossfaderAutomationLane()
{
    // Remove zoom state listener
    zoomState.removeListener(this);
    clearChopRegions();
}

void CrossfaderAutomationLane::setParameter(tracktion::engine::AutomatableParameter* param)
{
    if (param == nullptr)
    {
        // Clear existing parameter and regions
        if (parameter != nullptr)
            parameter->removeListener(this);
        parameter = nullptr;
        regionManager.reset();
        return;
    }

    AutomationLane::setParameter(param);
    regionManager = std::make_unique<RegionManager>(param);
    updatePoints();  // Call updatePoints after everything is initialized
}

void CrossfaderAutomationLane::onParameterChanged(tracktion::engine::AutomatableParameter* param)
{
    if (param == nullptr)
    {
        regionManager.reset();
        return;
    }
    regionManager = std::make_unique<RegionManager>(param);
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
    
    // Calculate visible time range in seconds
    auto visibleTimeStart = getSourceLength() * zoomState.getScrollPosition();
    auto visibleTimeEnd = visibleTimeStart + (getSourceLength() / zoomState.getZoomLevel());
    
    // Convert to beats for grid drawing
    auto visibleTimeStartBeats = tempoSequence.toBeats(tracktion::TimePosition::fromSeconds(visibleTimeStart));
    auto visibleTimeEndBeats = tempoSequence.toBeats(tracktion::TimePosition::fromSeconds(visibleTimeEnd));
    
    // Draw vertical grid lines based on current grid size
    double gridTimeBeats = std::floor(visibleTimeStartBeats.inBeats() / zoomState.getGridSize()) * zoomState.getGridSize();
    while (gridTimeBeats <= visibleTimeEndBeats.inBeats())
    {
        auto gridTimeSeconds = tempoSequence.toTime(tracktion::BeatPosition::fromBeats(gridTimeBeats)).inSeconds();
        auto xPos = timeToXY(gridTimeSeconds, 0.0).x;
        g.drawVerticalLine(static_cast<int>(xPos), bounds.getY(), bounds.getBottom());
        gridTimeBeats += zoomState.getGridSize();
    }
    
    // Only proceed if we have a valid RegionManager
    if (!regionManager)
        return;
        
    // Draw regions from the RegionManager
    const auto& regions = regionManager->getRegions();
    for (size_t i = 0; i < regions.size(); ++i) {
        const auto& region = regions[i];
        
        DBG("DRAW REGION, START TIME: " + juce::String(region.startTime) + ", END TIME: " + juce::String(region.endTime));

        DBG("IS A SIDE: " + juce::String(region.isASide ? "true" : "false"));
        // Only draw A-side regions
        if (!region.isASide)
            continue;
            
        
        // Skip regions that are completely outside the visible range
        if (region.endTime < visibleTimeStart || region.startTime > visibleTimeEnd)
            continue;
        
        // Clamp region times to visible range
        double clampedStartTime = juce::jlimit(visibleTimeStart, visibleTimeEnd, region.startTime);
        double clampedEndTime = juce::jlimit(visibleTimeStart, visibleTimeEnd, region.endTime);
        
        // Convert region times through beats for proper tempo handling
        auto startBeats = tempoSequence.toBeats(tracktion::TimePosition::fromSeconds(clampedStartTime));
        auto endBeats = tempoSequence.toBeats(tracktion::TimePosition::fromSeconds(clampedEndTime));
        
        auto startTimeSeconds = tempoSequence.toTime(startBeats).inSeconds();
        auto endTimeSeconds = tempoSequence.toTime(endBeats).inSeconds();
        
        auto startPoint = timeToXY(startTimeSeconds, 1.0);
        auto endPoint = timeToXY(endTimeSeconds, 1.0);
        
        // Calculate rectangle bounds for the region
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
    if (parameter == nullptr) return;
    
    auto [time, value] = XYToTime(event.position.x, event.position.y);
    if (snapEnabled)
        time = snapTimeToGrid(time);
    
    // Check if we clicked on a region
    selectedRegionIndex = std::nullopt;
    for (size_t i = 0; i < regionManager->getRegions().size(); ++i)
    {
        const auto& region = regionManager->getRegions()[i];
        if (time >= region.startTime && time <= region.endTime)
        {
            if(!region.isASide) {
                break;
            }
            selectedRegionIndex = i;
            isDragging = true;
            dragStartTime = time;
            dragOffsetTime = time - region.startTime;
            repaint();
            return;
        }
    }
    
    // If we didn't click on a region, add a new 1-second region
    if (snapEnabled)
    {
        time = snapTimeToGrid(time);
    }

    DBG("CROSSFADER AUTOMATION LANE, MOUSE DOWN, TIME: " + juce::String(time));
        
    auto& curve = parameter->getCurve();
    
    // Convert 1 beat to seconds using the tempo sequence
    // Add new region
    auto startBeat = edit.tempoSequence.toBeats(tracktion::TimePosition::fromSeconds(time));
    auto endBeat = tracktion::BeatPosition::fromBeats(startBeat.inBeats() + 1.0);
    auto endTime = edit.tempoSequence.toTime(endBeat).inSeconds();
    
    regionManager->addRegion(Region(time, endTime, true));

    isDragging = true;
    dragStartTime = time;
    dragOffsetTime = 0.0;
    repaint();
}

void CrossfaderAutomationLane::mouseDrag(const juce::MouseEvent& event)
{
    if (!isDragging || parameter == nullptr || !selectedRegionIndex)
        return;
        
    auto [time, value] = XYToTime(event.position.x, event.position.y);
    if (snapEnabled)
        time = snapTimeToGrid(time);
    
    // Calculate new start time, accounting for drag offset
    double newStartTime = time - dragOffsetTime;
    
    // Ensure we don't drag before 0
    if (newStartTime < 0)
        newStartTime = 0;
    
    // Move the region
    regionManager->moveRegion(*selectedRegionIndex, newStartTime);

    repaint();
}

void CrossfaderAutomationLane::mouseUp(const juce::MouseEvent&)
{
    isDragging = false;
}

void CrossfaderAutomationLane::setSnapToGrid(bool shouldSnap)
{
    snapEnabled = shouldSnap;
}

double CrossfaderAutomationLane::snapTimeToGrid(double time) const
{
    if (!snapEnabled)
        return time;
        
    auto& tempoSequence = edit.tempoSequence;
    auto timeInBeats = tempoSequence.toBeats(tracktion::TimePosition::fromSeconds(time));
    auto snappedBeats = std::round(timeInBeats.inBeats() / zoomState.getGridSize()) * zoomState.getGridSize();
    return tempoSequence.toTime(tracktion::BeatPosition::fromBeats(snappedBeats)).inSeconds();
}

void CrossfaderAutomationLane::clearChopRegions()
{
    if (parameter == nullptr)
        return;
        
    // Clear automation curve
    parameter->getCurve().clear();
    regionManager->clearRegions();
    if (onCurveChanged)
        onCurveChanged();
    repaint();
}

void CrossfaderAutomationLane::curveHasChanged(tracktion::engine::AutomatableParameter&)
{
    if (onCurveChanged)
        onCurveChanged();
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
    repaint();
}

void CrossfaderAutomationLane::deleteSelectedRegion()
{
    if (selectedRegionIndex)
    {
        regionManager->removeRegion(*selectedRegionIndex);
        selectedRegionIndex = std::nullopt;
        repaint();
    }
}

void CrossfaderAutomationLane::gridSizeChanged(float)
{
    repaint();
}

void CrossfaderAutomationLane::mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel)
{
    auto bounds = getLocalBounds();

    if (bounds.contains(event.getPosition()))
    {
        // Handle horizontal scrolling with shift key or horizontal wheel
        if (event.mods.isShiftDown() || wheel.deltaX != 0.0f)
        {
            auto delta = wheel.deltaX != 0.0f ? wheel.deltaX : wheel.deltaY;
            auto newScrollPos = zoomState.getScrollPosition() - (delta * 0.1);
            zoomState.setScrollPosition(newScrollPos);
        }
        // Handle zooming
        else if (wheel.deltaY != 0.0f)
        {
            // Calculate zoom factor based on wheel direction
            auto zoomFactor = wheel.deltaY > 0 ? 0.9 : 1.1; // Inverted for more natural feel

            // Calculate the mouse position as a proportion of the visible width
            auto mouseXProportion = (event.position.x - bounds.getX()) / (float)bounds.getWidth();

            // Calculate the absolute time position under the mouse
            auto timeUnderMouse = (zoomState.getScrollPosition() + (mouseXProportion / zoomState.getZoomLevel()));

            // Calculate new zoom level
            auto newZoomLevel = zoomState.getZoomLevel() * zoomFactor;

            // Calculate new scroll position that keeps the time under mouse at the same screen position
            auto newScrollPos = timeUnderMouse - (mouseXProportion / newZoomLevel);

            // Apply the changes
            zoomState.setZoomLevel(newZoomLevel);
            zoomState.setScrollPosition(newScrollPos);
        }
    }
}

void CrossfaderAutomationLane::mouseDoubleClick(const juce::MouseEvent& event)
{
    if (parameter == nullptr)
        return;
        
    auto [time, value] = XYToTime(event.position.x, event.position.y);
    
    // Check if we double-clicked on a region
    for (size_t i = 0; i < regionManager->getRegions().size(); ++i)
    {
        const auto& region = regionManager->getRegions()[i];
        if (time >= region.startTime && time <= region.endTime && region.isASide)
        {
            regionManager->removeRegion(i);
            selectedRegionIndex = std::nullopt;
            repaint();
            return;
        }
    }
} 
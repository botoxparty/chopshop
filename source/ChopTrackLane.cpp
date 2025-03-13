#include "ChopTrackLane.h"
#include "Utilities.h"

ChopTrackLane::ChopTrackLane(tracktion::engine::Edit& e, ZoomState& zs)
    : edit(e), zoomState(zs)
{
    // Register as a zoom state listener
    zoomState.addListener(this);
    chopTrack = getOrCreateChopTrack();
}

ChopTrackLane::~ChopTrackLane()
{
    // Remove zoom state listener
    zoomState.removeListener(this);
    clearClips();
}

tracktion::engine::AudioTrack::Ptr ChopTrackLane::getOrCreateChopTrack()
{
    auto track = EngineHelpers::getChopTrack(edit);
    if (track == nullptr)
    {
        DBG("No chop track found");
        return nullptr;
    }
    return track;
}

void ChopTrackLane::paint(juce::Graphics& g)
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
    
    DBG("Paint - Visible time range: " + juce::String(visibleTimeStart) + " to " + juce::String(visibleTimeEnd));
    DBG("Paint - Zoom level: " + juce::String(zoomState.getZoomLevel()) + ", Scroll position: " + juce::String(zoomState.getScrollPosition()));
    
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
    
    // Draw clips
    if (chopTrack != nullptr)
    {
        for (auto clip : chopTrack->getClips())
        {
            auto startTime = clip->getPosition().getStart().inSeconds();
            auto endTime = clip->getPosition().getEnd().inSeconds();

            DBG("ChopTrackLane::paint - Clip position: " + juce::String(startTime) + " - " + juce::String(endTime));
            
            // Skip clips that are completely outside the visible range
            if (endTime < visibleTimeStart || startTime > visibleTimeEnd)
                continue;
            
            // Clamp clip times to visible range
            double clampedStartTime = juce::jlimit(visibleTimeStart, visibleTimeEnd, startTime);
            double clampedEndTime = juce::jlimit(visibleTimeStart, visibleTimeEnd, endTime);
            
            // Convert clip times through beats for proper tempo handling
            auto startBeats = tempoSequence.toBeats(tracktion::TimePosition::fromSeconds(clampedStartTime));
            auto endBeats = tempoSequence.toBeats(tracktion::TimePosition::fromSeconds(clampedEndTime));
            
            auto startTimeSeconds = tempoSequence.toTime(startBeats).inSeconds();
            auto endTimeSeconds = tempoSequence.toTime(endBeats).inSeconds();
            
            auto startPoint = timeToXY(startTimeSeconds, 1.0);
            auto endPoint = timeToXY(endTimeSeconds, 1.0);
            
            // Calculate rectangle bounds for the clip
            juce::Rectangle<float> clipBounds(
                startPoint.x,
                bounds.getY(),
                endPoint.x - startPoint.x,
                bounds.getHeight()
            );
            
            // Draw clip with different color if selected
            g.setColour(clip == selectedClip ? juce::Colours::orangered : juce::Colours::orange);
            g.fillRect(clipBounds);
            
            // Draw border
            g.setColour(clip == selectedClip ? juce::Colours::white : juce::Colours::white.withAlpha(0.5f));
            g.drawRect(clipBounds, 1.0f);
        }
    }
}

void ChopTrackLane::mouseDown(const juce::MouseEvent& event)
{
    if (chopTrack == nullptr)
        return;

    // Use XYToTime instead of raw calculation to properly account for zoom and scroll
    auto [time, value] = XYToTime(event.position.x, event.position.y);
    
    DBG("MouseDown - Initial time from XYToTime: " + juce::String(time));
    DBG("MouseDown - Mouse X: " + juce::String(event.position.x));
    DBG("MouseDown - Zoom level: " + juce::String(zoomState.getZoomLevel()) + 
        ", Scroll position: " + juce::String(zoomState.getScrollPosition()));

    // First check if we clicked on a clip - use raw time before snapping
    selectedClip = nullptr;
    for (auto clip : chopTrack->getClips())
    {
        auto startTime = clip->getPosition().getStart().inSeconds();
        auto endTime = clip->getPosition().getEnd().inSeconds();
        
        if (time >= startTime && time <= endTime)
        {
            selectedClip = clip;
            isDragging = true;
            dragStartTime = time;
            dragOffsetTime = time - startTime;
            repaint();
            return;
        }
    }
    
    // If we didn't click on a clip, then we can snap the time for new clip creation
    if (snapEnabled)
        time = snapTimeToGrid(time);
        
    // Convert 1 beat to seconds using the tempo sequence
    auto startBeat = edit.tempoSequence.toBeats(tracktion::TimePosition::fromSeconds(time));
    auto endBeat = tracktion::BeatPosition::fromBeats(startBeat.inBeats() + 1.0);
    auto endTime = edit.tempoSequence.toTime(endBeat).inSeconds();
    
    DBG("MouseDown - Adding clip - Start time: " + juce::String(time) + 
        ", Start beat: " + juce::String(startBeat.inBeats()) + 
        ", End beat: " + juce::String(endBeat.inBeats()) + 
        ", End time: " + juce::String(endTime));
    
    addClip(time, endTime);

    isDragging = true;
    dragStartTime = time;
    dragOffsetTime = 0.0;
    repaint();
}

void ChopTrackLane::mouseDrag(const juce::MouseEvent& event)
{
    if (!isDragging || selectedClip == nullptr || chopTrack == nullptr)
        return;
        
    auto [time, value] = XYToTime(event.position.x, event.position.y);
    
    // Calculate the raw new position without snapping first
    double newStartTime = time - dragOffsetTime;
    
    // If snapping is enabled, snap based on the clip's position
    if (snapEnabled)
    {
        // Get the original clip start position
        auto originalStartTime = selectedClip->getPosition().getStart().inSeconds();
        
        // Calculate how much we're trying to move the clip
        double moveAmount = newStartTime - originalStartTime;
        
        // Snap the move amount to the grid
        auto& tempoSequence = edit.tempoSequence;
        auto moveInBeats = tempoSequence.toBeats(tracktion::TimePosition::fromSeconds(std::abs(moveAmount)));
        double gridSize = zoomState.getGridSize();
        
        // Find the previous and next grid lines
        double previousGridLine = std::floor(moveInBeats.inBeats() / gridSize) * gridSize;
        double nextGridLine = previousGridLine + gridSize;
        
        // Calculate how far we are between these grid lines
        double distanceFromPrevious = moveInBeats.inBeats() - previousGridLine;
        double percentageToNext = distanceFromPrevious / gridSize;
        
        // Choose which grid line to snap to
        double snappedBeats = (percentageToNext >= 0.9) ? nextGridLine : previousGridLine;
        
        // Convert back to time
        auto snappedMoveTime = tempoSequence.toTime(tracktion::BeatPosition::fromBeats(snappedBeats)).inSeconds();
        
        // Apply the direction of movement
        if (moveAmount < 0)
            snappedMoveTime = -snappedMoveTime;
            
        // Calculate final position
        newStartTime = originalStartTime + snappedMoveTime;
    }
    
    // Ensure we don't drag before 0
    if (newStartTime < 0)
        newStartTime = 0;
    
    // Move the clip
    auto duration = selectedClip->getPosition().getLength().inSeconds();
    auto newTimeRange = tracktion::TimeRange::between(
        tracktion::TimePosition::fromSeconds(newStartTime),
        tracktion::TimePosition::fromSeconds(newStartTime + duration)
    );
    
    selectedClip->setPosition({ newTimeRange, tracktion::TimeDuration() });
    repaint();
}

void ChopTrackLane::mouseUp(const juce::MouseEvent&)
{
    isDragging = false;
    selectedClip = nullptr;
    repaint();
}

void ChopTrackLane::mouseDoubleClick(const juce::MouseEvent& event)
{
    if (chopTrack == nullptr)
        return;

    auto [time, value] = XYToTime(event.position.x, event.position.y);
    
    // Check if we double-clicked on a clip
    for (auto clip : chopTrack->getClips())
    {
        auto startTime = clip->getPosition().getStart().inSeconds();
        auto endTime = clip->getPosition().getEnd().inSeconds();
        
        if (time >= startTime && time <= endTime)
        {
            clip->removeFromParent();
            selectedClip = nullptr;
            repaint();
            return;
        }
    }
}

void ChopTrackLane::mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel)
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

void ChopTrackLane::setSnapToGrid(bool shouldSnap)
{
    snapEnabled = shouldSnap;
}

double ChopTrackLane::snapTimeToGrid(double time) const
{
    if (!snapEnabled)
        return time;
        
    auto& tempoSequence = edit.tempoSequence;
    auto timeInBeats = tempoSequence.toBeats(tracktion::TimePosition::fromSeconds(time));
    double gridSize = zoomState.getGridSize();
    
    DBG("Snap - Time: " + juce::String(time) + 
        ", In beats: " + juce::String(timeInBeats.inBeats()) + 
        ", Grid size: " + juce::String(gridSize));
    
    // Find the previous and next grid lines relative to the click position
    double previousGridLine = std::floor(timeInBeats.inBeats() / gridSize) * gridSize;
    double nextGridLine = previousGridLine + gridSize;
    
    // Calculate how far we are between these grid lines
    double distanceFromPrevious = timeInBeats.inBeats() - previousGridLine;
    double percentageToNext = distanceFromPrevious / gridSize;
    
    // Choose which grid line to snap to
    double snappedBeats = (percentageToNext >= 0.9) ? nextGridLine : previousGridLine;
    
    auto snappedTime = tempoSequence.toTime(tracktion::BeatPosition::fromBeats(snappedBeats)).inSeconds();
    
    DBG("Snap - Distance to next: " + juce::String(percentageToNext * 100) + "%, Previous: " + 
        juce::String(previousGridLine) + ", Next: " + juce::String(nextGridLine) + 
        ", Snapped to: " + juce::String(snappedBeats));
    
    return snappedTime;
}

void ChopTrackLane::clearClips()
{
    if (chopTrack != nullptr)
    {
        for (auto clip : chopTrack->getClips())
            clip->removeFromParent();
    }
    repaint();
}

void ChopTrackLane::deleteSelectedClip()
{
    if (selectedClip != nullptr)
    {
        selectedClip->removeFromParent();
        selectedClip = nullptr;
        repaint();
    }
}

void ChopTrackLane::addClip(double startTime, double endTime)
{
    if (chopTrack == nullptr)
        return;

    DBG("AddClip - Creating clip from " + juce::String(startTime) + " to " + juce::String(endTime));

    auto timeRange = tracktion::TimeRange::between(
        tracktion::TimePosition::fromSeconds(startTime),
        tracktion::TimePosition::fromSeconds(endTime)
    );

    auto newClip = chopTrack->insertNewClip(tracktion::engine::TrackItem::Type::arranger, timeRange, nullptr);
    if (newClip != nullptr)
    {
        newClip->setName("Chop " + juce::String(chopTrack->getClips().size()));
        selectedClip = newClip;
        DBG("AddClip - Clip created successfully with range: " + 
            juce::String(newClip->getPosition().getStart().inSeconds()) + " to " + 
            juce::String(newClip->getPosition().getEnd().inSeconds()));
    }
    else
    {
        DBG("AddClip - Failed to create clip!");
    }
    repaint();
}

juce::Point<float> ChopTrackLane::timeToXY(double time, double value) const
{
    auto bounds = getLocalBounds().toFloat();
    auto sourceLength = getSourceLength();
    
    float x = bounds.getX() + bounds.getWidth() * 
        ((time - (sourceLength * zoomState.getScrollPosition())) / 
         (sourceLength / zoomState.getZoomLevel()));
         
    float y = bounds.getBottom() - (value * bounds.getHeight());
    
    return { x, y };
}

std::pair<double, double> ChopTrackLane::XYToTime(float x, float y) const
{
    auto bounds = getLocalBounds().toFloat();
    auto sourceLength = getSourceLength();
    
    double time = ((x - bounds.getX()) / bounds.getWidth()) * 
                  (sourceLength / zoomState.getZoomLevel()) + 
                  (sourceLength * zoomState.getScrollPosition());
                  
    double value = 1.0 - ((y - bounds.getY()) / bounds.getHeight());
    
    return { time, value };
}

double ChopTrackLane::getSourceLength() const
{
    // For now, return a fixed length of 60 seconds
    // This should be updated based on your actual track length
        auto clip = EngineHelpers::getCurrentClip(edit);
    if (clip != nullptr)
        return clip->getPosition().getLength().inSeconds();

    DBG("No clip found, returning default length");
    return 60.0;
} 
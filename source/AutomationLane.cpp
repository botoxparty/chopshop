#include "AutomationLane.h"

AutomationLane::AutomationLane(tracktion::engine::Edit& e, tracktion::engine::AutomatableParameter* param)
    : edit(e), parameter(param)
{
    if (parameter != nullptr)
        parameter->addListener(this);
    updatePoints();
}

AutomationLane::~AutomationLane()
{
    if (parameter != nullptr)
        parameter->removeListener(this);
}

void AutomationLane::setZoomLevel(double newZoomLevel)
{
    if (std::abs(zoomLevel - newZoomLevel) > 0.0001)
    {
        zoomLevel = newZoomLevel;
        repaint();
    }
}

void AutomationLane::setScrollPosition(double newScrollPosition)
{
    if (std::abs(scrollPosition - newScrollPosition) > 0.0001)
    {
        scrollPosition = newScrollPosition;
        repaint();
    }
}

void AutomationLane::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    
    // Draw background
    g.setColour(juce::Colours::darkgrey);
    g.fillRect(bounds);
    
    // Draw grid lines
    g.setColour(juce::Colours::grey);
    for (float y = 0; y <= 1.0f; y += 0.25f)
    {
        float yPos = bounds.getY() + bounds.getHeight() * (1.0f - y);
        g.drawHorizontalLine(static_cast<int>(yPos), bounds.getX(), bounds.getRight());
    }
    
    // Draw automation points and lines
    if (automationPoints.size() > 0 && parameter != nullptr)
    {
        g.setColour(juce::Colours::orange);
        
        for (size_t i = 0; i < automationPoints.size(); ++i)
        {
            auto point = timeToXY(automationPoints[i].first, automationPoints[i].second);
            
            // Make points more visible
            const float pointSize = 8.0f;
            g.fillEllipse(point.x - pointSize/2, point.y - pointSize/2, pointSize, pointSize);
            
            // Highlight dragged point
            if (static_cast<int>(i) == draggedPointIndex)
            {
                g.setColour(juce::Colours::yellow);
                g.drawEllipse(point.x - pointSize/2 - 2, point.y - pointSize/2 - 2, 
                            pointSize + 4, pointSize + 4, 2.0f);
                g.setColour(juce::Colours::orange);
            }
            
            // Draw line to next point
            if (i < automationPoints.size() - 1)
            {
                auto nextPoint = timeToXY(automationPoints[i + 1].first, automationPoints[i + 1].second);
                g.drawLine(point.x, point.y, nextPoint.x, nextPoint.y, 2.0f);
            }
        }
    }
}

void AutomationLane::resized()
{
}

int AutomationLane::findPointNear(float x, float y) const
{
    if (automationPoints.empty())
        return -1;

    // Increase hit radius for easier selection
    const float hitRadius = 10.0f;
    
    for (size_t i = 0; i < automationPoints.size(); ++i)
    {
        auto point = timeToXY(automationPoints[i].first, automationPoints[i].second);
        float distance = std::hypot(point.x - x, point.y - y);
        
        DBG("Point " << i << " at (" << point.x << "," << point.y << ") distance: " << distance 
            << " from click (" << x << "," << y << ")");
            
        if (distance <= hitRadius)
            return static_cast<int>(i);
    }
    
    return -1;
}

void AutomationLane::mouseDown(const juce::MouseEvent& event)
{
    if (parameter != nullptr)
    {
        // Check if we clicked on an existing point
        draggedPointIndex = findPointNear(event.position.x, event.position.y);
        
        if (draggedPointIndex == -1)
        {
            // If we didn't click on a point, add a new one
            auto [time, value] = XYToTime(event.position.x, event.position.y);
            addPoint(time, value);
        }
        else
        {
            // Start a change gesture if we're dragging an existing point
            parameterChangeGestureBegin(*parameter);
        }
    }
}

void AutomationLane::mouseDrag(const juce::MouseEvent& event)
{
    if (parameter != nullptr)
    {
        auto [time, value] = XYToTime(event.position.x, event.position.y);
        
        if (draggedPointIndex != -1 && draggedPointIndex < static_cast<int>(automationPoints.size()))
        {
            // Update the existing point's position
            auto& curve = parameter->getCurve();
            curve.removePoint(draggedPointIndex); // Remove old point
            curve.addPoint(tracktion::TimePosition::fromSeconds(time), value, 0.0f); // Add updated point
            updatePoints(); // Refresh our point cache
        }
        else
        {
            // Update or add a new point at the current position
            updateValueAtTime(time, value);
        }
    }
}

void AutomationLane::mouseUp(const juce::MouseEvent&)
{
    if (parameter != nullptr && draggedPointIndex != -1)
    {
        // End the change gesture if we were dragging a point
        parameterChangeGestureEnd(*parameter);
        draggedPointIndex = -1;
    }
}

void AutomationLane::mouseDoubleClick(const juce::MouseEvent& event)
{
    if (parameter != nullptr)
    {
        int pointIndex = findPointNear(event.position.x, event.position.y);
        if (pointIndex != -1)
        {
            auto& curve = parameter->getCurve();
            curve.removePoint(pointIndex);
            updatePoints();
            repaint();
        }
    }
}

void AutomationLane::setParameter(tracktion::engine::AutomatableParameter* param)
{
    if (parameter != nullptr)
        parameter->removeListener(this);
        
    parameter = param;
    
    if (parameter != nullptr)
        parameter->addListener(this);
        
    updatePoints();
    repaint();
}

void AutomationLane::updatePoints()
{
    automationPoints.clear();
    
    if (parameter != nullptr)
    {
        auto& curve = parameter->getCurve();
        // Get all automation points from the curve
        for (int i = 0; i < curve.getNumPoints(); ++i)
        {
            auto point = curve.getPoint(i);
            automationPoints.emplace_back(point.time.inSeconds(), point.value);
        }
    }
    
    repaint();
}

void AutomationLane::curveHasChanged(tracktion::engine::AutomatableParameter&)
{
    if (parameter != nullptr)
    {
        auto& curve = parameter->getCurve();
        std::vector<std::pair<double, double>> newPoints;
        
        // Get current points
        for (int i = 0; i < curve.getNumPoints(); ++i)
        {
            auto point = curve.getPoint(i);
            newPoints.emplace_back(point.time.inSeconds(), point.value);
        }
        
        // Compare with previous points to show changes
        if (newPoints.size() > automationPoints.size())
        {
            DBG("Points added: Previous count=" << automationPoints.size() << ", New count=" << newPoints.size());
        }
        else if (newPoints.size() < automationPoints.size())
        {
            DBG("Points removed: Previous count=" << automationPoints.size() << ", New count=" << newPoints.size());
        }
        
        // Check for value changes in existing points
        size_t minSize = std::min(newPoints.size(), automationPoints.size());
        for (size_t i = 0; i < minSize; ++i)
        {
            if (std::abs(newPoints[i].second - automationPoints[i].second) > 0.0001)
            {
                DBG("Point " << i << " value changed from " << automationPoints[i].second 
                    << " to " << newPoints[i].second 
                    << " at time " << newPoints[i].first << "s");
            }
        }
    }
    
    updatePoints();
}

void AutomationLane::currentValueChanged(tracktion::engine::AutomatableParameter&)
{
    // No need to handle this as we're only interested in curve point changes
}

void AutomationLane::parameterChanged(tracktion::engine::AutomatableParameter&, float)
{
    // No need to handle this as we're only interested in curve point changes
}

void AutomationLane::parameterChangeGestureBegin(tracktion::engine::AutomatableParameter&)
{
    // Called when a parameter change gesture begins (e.g., when the user starts dragging)
    // We don't need to do anything here as we're only interested in curve point changes
}

void AutomationLane::parameterChangeGestureEnd(tracktion::engine::AutomatableParameter&)
{
    // Called when a parameter change gesture ends (e.g., when the user stops dragging)
    // We don't need to do anything here as we're only interested in curve point changes
}

juce::Point<float> AutomationLane::timeToXY(double timeInSeconds, double value) const
{
    auto bounds = getLocalBounds().toFloat();
    
    // Calculate visible time range
    auto visibleTimeStart = getSourceLength() * scrollPosition;
    auto visibleTimeEnd = visibleTimeStart + (getSourceLength() / zoomLevel);
    
    // Normalize time to width using the visible time range
    float normalizedTime = (timeInSeconds - visibleTimeStart) / (visibleTimeEnd - visibleTimeStart);
    float x = bounds.getX() + (normalizedTime * bounds.getWidth());
    
    // Normalize value to height
    float normalizedValue = 0.0f;
    if (parameter != nullptr)
    {
        auto range = parameter->getValueRange();
        normalizedValue = static_cast<float>((value - range.getStart()) / (range.getEnd() - range.getStart()));
    }
    float y = bounds.getBottom() - (normalizedValue * bounds.getHeight());
    
    return {x, y};
}

std::pair<double, double> AutomationLane::XYToTime(float x, float y) const
{
    auto bounds = getLocalBounds().toFloat();
    
    // Calculate visible time range
    auto visibleTimeStart = getSourceLength() * scrollPosition;
    auto visibleTimeEnd = visibleTimeStart + (getSourceLength() / zoomLevel);
    
    // Convert x to time using the visible time range
    double normalizedX = (x - bounds.getX()) / bounds.getWidth();
    double timeInSeconds = visibleTimeStart + (normalizedX * (visibleTimeEnd - visibleTimeStart));
    
    // Convert y to parameter value
    float normalizedValue = 1.0f - ((y - bounds.getY()) / bounds.getHeight());
    double value = 0.0;
    
    if (parameter != nullptr)
    {
        auto range = parameter->getValueRange();
        value = range.getStart() + normalizedValue * (range.getEnd() - range.getStart());
        value = static_cast<double>(parameter->snapToState(static_cast<float>(value)));
    }
    
    // Clamp values
    timeInSeconds = juce::jlimit(0.0, getSourceLength(), timeInSeconds);
    
    return {timeInSeconds, value};
}

void AutomationLane::addPoint(double timeInSeconds, double value)
{
    if (parameter != nullptr)
    {
        auto& curve = parameter->getCurve();
        curve.addPoint(tracktion::TimePosition::fromSeconds(timeInSeconds), value, 0.0f);
        // No need to call updatePoints here as curveHasChanged will be called
    }
}

void AutomationLane::updateValueAtTime(double timeInSeconds, double value)
{
    if (parameter != nullptr)
    {
        auto& curve = parameter->getCurve();
        auto time = tracktion::TimePosition::fromSeconds(timeInSeconds);
        float val = static_cast<float>(value);
        int pointIndex = curve.getNearestPoint(time, val, 1.0);
        if (pointIndex >= 0)
        {
            auto point = curve.getPoint(pointIndex);
            point.value = value;
        }
    }
    updatePoints();
}
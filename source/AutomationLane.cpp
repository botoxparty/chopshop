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
            
            // Draw point
            g.fillEllipse(point.x - 4, point.y - 4, 8, 8);
            
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

void AutomationLane::mouseDown(const juce::MouseEvent& event)
{
    if (parameter != nullptr)
    {
        auto [time, value] = XYToTime(event.position.x, event.position.y);
        addPoint(time, value);
    }
}

void AutomationLane::mouseDrag(const juce::MouseEvent& event)
{
    if (parameter != nullptr)
    {
        auto [time, value] = XYToTime(event.position.x, event.position.y);
        updateValueAtTime(time, value);
    }
}

void AutomationLane::mouseUp(const juce::MouseEvent&)
{
    // No need to call updatePoints here as curveHasChanged will be called
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
    DBG("Curve has changed");
    updatePoints();
}

void AutomationLane::currentValueChanged(tracktion::engine::AutomatableParameter& param)
{
    // No need to handle this as we're only interested in curve point changes
}

void AutomationLane::parameterChanged(tracktion::engine::AutomatableParameter& param, float)
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
    
    // Normalize time to width
    float x = bounds.getX() + (timeInSeconds / 60.0f) * bounds.getWidth(); // Assuming 60 seconds view range
    
    // Normalize value to height
    float normalizedValue = 0.0f;
    if (parameter != nullptr)
    {
        auto range = parameter->getValueRange();
        normalizedValue = (value - range.getStart()) / (range.getEnd() - range.getStart());
    }
    float y = bounds.getBottom() - (normalizedValue * bounds.getHeight());
    
    return {x, y};
}

std::pair<double, double> AutomationLane::XYToTime(float x, float y) const
{
    auto bounds = getLocalBounds().toFloat();
    
    // Convert x to time
    double timeInSeconds = (x - bounds.getX()) * 60.0 / bounds.getWidth();
    
    // Convert y to parameter value
    float normalizedValue = 1.0f - ((y - bounds.getY()) / bounds.getHeight());
    double value = 0.0;
    
    if (parameter != nullptr)
    {
        auto range = parameter->getValueRange();
        value = range.getStart() + normalizedValue * (range.getEnd() - range.getStart());
        value = parameter->snapToState(value);
    }
    
    // Clamp values
    timeInSeconds = juce::jlimit(0.0, 60.0, timeInSeconds);
    
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
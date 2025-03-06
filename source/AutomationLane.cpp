#include "AutomationLane.h"

AutomationLane::AutomationLane(tracktion::engine::Edit& e)
    : edit(e)
{
    updatePoints();
}

AutomationLane::~AutomationLane()
{
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
    if (automationPoints.size() > 0)
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
    auto [time, bpm] = XYToTime(event.position.x, event.position.y);
    addPoint(time, bpm);
    repaint();
}

void AutomationLane::mouseDrag(const juce::MouseEvent& event)
{
    auto [time, bpm] = XYToTime(event.position.x, event.position.y);
    updateTempoAtTime(time, bpm);
    repaint();
}

void AutomationLane::mouseUp(const juce::MouseEvent&)
{
    updatePoints();
}

void AutomationLane::updatePoints()
{
    automationPoints.clear();
    
    // Get all tempo settings from the sequence
    auto& tempoSequence = edit.tempoSequence;
    
    for (int i = 0; i < tempoSequence.getNumTempos(); ++i)
    {
        auto* tempo = tempoSequence.getTempo(i);
        automationPoints.emplace_back(tempo->getStartTime().inSeconds(), tempo->getBpm());
    }
    
    repaint();
}

juce::Point<float> AutomationLane::timeToXY(double timeInSeconds, double value) const
{
    auto bounds = getLocalBounds().toFloat();
    
    // Normalize time to width
    float x = bounds.getX() + (timeInSeconds / 60.0f) * bounds.getWidth(); // Assuming 60 seconds view range
    
    // Normalize value (BPM) to height (assuming BPM range 60-180)
    float normalizedValue = (value - 60.0) / (180.0 - 60.0);
    float y = bounds.getBottom() - (normalizedValue * bounds.getHeight());
    
    return {x, y};
}

std::pair<double, double> AutomationLane::XYToTime(float x, float y) const
{
    auto bounds = getLocalBounds().toFloat();
    
    // Convert x to time
    double timeInSeconds = (x - bounds.getX()) * 60.0 / bounds.getWidth();
    
    // Convert y to BPM (60-180 range)
    float normalizedValue = 1.0f - ((y - bounds.getY()) / bounds.getHeight());
    double bpm = 60.0 + normalizedValue * (180.0 - 60.0);
    
    // Clamp values
    timeInSeconds = juce::jlimit(0.0, 60.0, timeInSeconds);
    bpm = juce::jlimit(60.0, 180.0, bpm);
    
    return {timeInSeconds, bpm};
}

void AutomationLane::addPoint(double timeInSeconds, double bpm)
{
    auto& tempoSequence = edit.tempoSequence;
    auto newTempo = tempoSequence.insertTempo(tracktion::TimePosition::fromSeconds(timeInSeconds));
    if (newTempo != nullptr)
    {
        newTempo->setBpm(bpm);
    }
    updatePoints();
}

void AutomationLane::updateTempoAtTime(double timeInSeconds, double bpm)
{
    auto& tempoSequence = edit.tempoSequence;
    if (auto* tempo = tempoSequence.getTempo(timeInSeconds))
    {
        tempo->setBpm(bpm);
    }
    updatePoints();
} 
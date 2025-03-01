#include "Thumbnail.h"

//==============================================================================
Thumbnail::Thumbnail(tracktion::engine::TransportControl& transportControl)
    : transport(transportControl),
      smartThumbnail(transport.engine, tracktion::engine::AudioFile(transport.engine), *this, nullptr)
{
    // Set up cursor
    cursor.setFill(cursorColor);
    addAndMakeVisible(cursor);
    
    // Set up pending cursor
    pendingCursor.setFill(juce::Colours::cyan.withAlpha(0.7f));
    addChildComponent(pendingCursor);
}

Thumbnail::~Thumbnail()
{
    stopTimer();
}

//==============================================================================
void Thumbnail::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();
    
    // Draw background
    g.setColour(backgroundColor);
    g.fillRoundedRectangle(bounds.toFloat(), 4.0f);
    
    // Draw border
    g.setColour(juce::Colours::grey.withAlpha(0.5f));
    g.drawRoundedRectangle(bounds.toFloat().reduced(0.5f), 4.0f, 1.0f);
    
    // Draw waveform
    if (smartThumbnail.isGeneratingProxy())
    {
        // Show loading progress
        g.setColour(juce::Colours::white);
        g.setFont(16.0f);
        g.drawText("Creating waveform: " + juce::String(juce::roundToInt(smartThumbnail.getProxyProgress() * 100.0f)) + "%",
                   bounds, juce::Justification::centred);
    }
    else if (smartThumbnail.getTotalLength() > 0.0)
    {
        // Draw time markers
        drawTimeMarkers(g, bounds);
        
        // Draw the waveform
        const float brightness = smartThumbnail.isOutOfDate() ? 0.4f : 1.0f;
        g.setColour(waveformColor.withMultipliedBrightness(brightness));
        
        auto totalLength = tracktion::TimePosition::fromSeconds(smartThumbnail.getTotalLength());
        auto waveformBounds = bounds.reduced(4);
        
        smartThumbnail.drawChannels(g, waveformBounds, { tracktion::TimePosition::fromSeconds(0), totalLength }, 1.0f);
    }
    else
    {
        // No file loaded
        g.setColour(juce::Colours::white.withAlpha(0.5f));
        g.setFont(16.0f);
        g.drawText("No audio file loaded", bounds, juce::Justification::centred);
    }
}

void Thumbnail::resized()
{
    updateCursorPosition();
}

//==============================================================================
void Thumbnail::mouseDown(const juce::MouseEvent& e)
{
    positionToJumpAt = {};
    
    transport.setUserDragging(true);
    mouseDrag(e);
}

void Thumbnail::mouseDrag(const juce::MouseEvent& e)
{
    if (!e.mouseWasDraggedSinceMouseDown())
        return;
    
    const auto bounds = getLocalBounds().reduced(4);
    
    if (bounds.getWidth() <= 0)
        return;
    
    const float proportion = (e.position.x - bounds.getX()) / bounds.getWidth();
    const float clampedProportion = juce::jlimit(0.0f, 1.0f, proportion);
    
    auto loopRange = transport.getLoopRange();
    auto loopStart = loopRange.getStart();
    
    // Calculate position directly
    transport.setPosition(tracktion::TimePosition::fromSeconds(
        loopStart.inSeconds() + loopRange.getLength().inSeconds() * clampedProportion));
}

void Thumbnail::mouseUp(const juce::MouseEvent& e)
{
    transport.setUserDragging(false);
    
    if (e.mouseWasDraggedSinceMouseDown())
        return;
    
    const auto bounds = getLocalBounds().reduced(4);
    
    if (bounds.getWidth() <= 0)
        return;
    
    if (auto epc = transport.edit.getCurrentPlaybackContext())
    {
        auto& ts = transport.edit.tempoSequence;
        
        const float proportion = (e.position.x - bounds.getX()) / bounds.getWidth();
        const float clampedProportion = juce::jlimit(0.0f, 1.0f, proportion);
        
        auto loopRange = transport.getLoopRange();
        auto loopStart = loopRange.getStart();
        
        // Calculate position directly
        auto positionToJumpTo = tracktion::TimePosition::fromSeconds(
            loopStart.inSeconds() + loopRange.getLength().inSeconds() * clampedProportion);
        
        if (quantisationNumBars)
        {
            positionToJumpTo = roundToNearest(positionToJumpTo, ts, *quantisationNumBars);
            positionToJumpAt = roundUp(epc->getPosition(), ts, *quantisationNumBars);
        }
        else
        {
            positionToJumpAt = {};
        }
        
        epc->postPosition(positionToJumpTo, positionToJumpAt);
    }
}

//==============================================================================
void Thumbnail::start()
{
    startTimerHz(30); // Update at 30fps for smooth cursor movement
}

void Thumbnail::setFile(const tracktion::engine::AudioFile& file)
{
    smartThumbnail.setNewFile(file);
    repaint();
}

void Thumbnail::setSpeedRatio(double ratio)
{
    currentSpeedRatio = ratio;
    repaint();
}

void Thumbnail::setQuantisation(std::optional<int> numBars)
{
    quantisationNumBars = numBars;
}

void Thumbnail::setWaveformColor(juce::Colour color)
{
    waveformColor = color;
    repaint();
}

void Thumbnail::setCursorColor(juce::Colour color)
{
    cursorColor = color;
    cursor.setFill(cursorColor);
    repaint();
}

void Thumbnail::setBackgroundColor(juce::Colour color)
{
    backgroundColor = color;
    repaint();
}

//==============================================================================
void Thumbnail::timerCallback()
{
    updateCursorPosition();
    
    if (smartThumbnail.isGeneratingProxy() || smartThumbnail.isOutOfDate())
        repaint();
}

void Thumbnail::updateCursorPosition()
{
    if (getWidth() <= 0)
        return;
    
    const auto bounds = getLocalBounds().reduced(4);
    
    if (auto epc = transport.edit.getCurrentPlaybackContext())
    {
        auto loopRange = transport.getLoopRange();
        auto loopLength = loopRange.getLength();
        
        if (loopLength > tracktion::TimeDuration::fromSeconds(0))
        {
            const auto currentPos = epc->getPosition();
            const auto proportion = (currentPos - loopRange.getStart()) / loopLength;
            const auto clampedProportion = juce::jlimit(0.0, 1.0, proportion);
            
            const int x = bounds.getX() + juce::roundToInt(clampedProportion * bounds.getWidth());
            cursor.setRectangle(juce::Rectangle<float>(x - 1.0f, bounds.getY(), 2.0f, bounds.getHeight()));
            cursor.setVisible(true);
            
            // Show pending cursor if needed
            if (positionToJumpAt.has_value())
            {
                const auto pendingProportion = (*positionToJumpAt - loopRange.getStart()) / loopLength;
                
                if (pendingProportion >= 0.0 && pendingProportion <= 1.0)
                {
                    const int pendingX = bounds.getX() + juce::roundToInt(pendingProportion * bounds.getWidth());
                    pendingCursor.setRectangle(juce::Rectangle<float>(pendingX - 1.0f, bounds.getY(), 2.0f, bounds.getHeight()));
                    pendingCursor.setVisible(true);
                }
                else
                {
                    pendingCursor.setVisible(false);
                }
            }
            else
            {
                pendingCursor.setVisible(false);
            }
        }
        else
        {
            cursor.setVisible(false);
            pendingCursor.setVisible(false);
        }
    }
}

void Thumbnail::drawTimeMarkers(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    if (smartThumbnail.getTotalLength() <= 0.0)
        return;
    
    const int numMarkers = 10;
    const float totalSeconds = static_cast<float>(smartThumbnail.getTotalLength());
    
    g.setColour(juce::Colours::white.withAlpha(0.4f));
    g.setFont(12.0f);
    
    for (int i = 0; i <= numMarkers; ++i)
    {
        const float proportion = static_cast<float>(i) / numMarkers;
        const int x = bounds.getX() + juce::roundToInt(proportion * bounds.getWidth());
        
        // Draw marker line
        g.setColour(juce::Colours::white.withAlpha(0.2f));
        g.drawLine(x, bounds.getY() + 2, x, bounds.getBottom() - 2, 1.0f);
        
        // Draw time label
        const float timeInSeconds = proportion * totalSeconds;
        const int minutes = static_cast<int>(timeInSeconds) / 60;
        const int seconds = static_cast<int>(timeInSeconds) % 60;
        
        juce::String timeString;
        timeString << minutes << ":" << juce::String(seconds).paddedLeft('0', 2);
        
        g.setColour(juce::Colours::white.withAlpha(0.6f));
        g.drawText(timeString, x - 20, bounds.getBottom() - 16, 40, 14, juce::Justification::centred, false);
    }
}

tracktion::TimePosition Thumbnail::roundToNearest(tracktion::TimePosition pos, const tracktion::engine::TempoSequence& ts, int quantisationNumBars)
{
    // Convert time to beats
    auto beatPos = ts.toBeats(pos);
    
    // Calculate quantized beat position
    const auto quantisation = static_cast<double>(quantisationNumBars) * 4.0;
    const auto quantisedBeats = std::round(beatPos.inBeats() / quantisation) * quantisation;
    
    // Convert back to time
    return ts.toTime(tracktion::BeatPosition::fromBeats(quantisedBeats));
}

tracktion::TimePosition Thumbnail::roundUp(tracktion::TimePosition pos, const tracktion::engine::TempoSequence& ts, int quantisationNumBars)
{
    // Convert time to beats
    auto beatPos = ts.toBeats(pos);
    
    // Calculate quantized beat position
    const auto quantisation = static_cast<double>(quantisationNumBars) * 4.0;
    const auto quantisedBeats = std::ceil(beatPos.inBeats() / quantisation) * quantisation;
    
    // Convert back to time
    return ts.toTime(tracktion::BeatPosition::fromBeats(quantisedBeats));
}

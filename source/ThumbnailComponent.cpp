#include "ThumbnailComponent.h"
#include "Utilities.h"

ThumbnailComponent::ThumbnailComponent(tracktion::engine::Edit& e, ZoomState& zs)
    : edit(e),
      transport(e.getTransport()),
      thumbnail(e.engine, tracktion::AudioFile(e.engine), *this, &e),
      currentClip(nullptr),
      zoomState(zs)
{
    // Initialize thumbnail
    thumbnail.audioFileChanged();

    // Setup playhead
    playhead = std::make_unique<juce::DrawableRectangle>();
    playhead->setFill(juce::FillType(juce::Colours::red));
    addAndMakeVisible(*playhead);

    // Register as listener for zoom state changes
    zoomState.addListener(this);

    startTimerHz(30);
}

ThumbnailComponent::~ThumbnailComponent()
{
    stopTimer();
    zoomState.removeListener(this);
}

void ThumbnailComponent::timerCallback()
{
    updatePlayheadPosition();
    
    // Force redraw during playback
    if (transport.isPlaying())
        repaint();
}

void ThumbnailComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    // Draw background
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));

    // Draw waveform area
    g.setColour(juce::Colours::darkgrey.darker(0.7f));
    g.fillRect(bounds);

    // Create drawing bounds
    auto drawBounds = bounds.reduced(2);

    // Draw waveform if we have a clip
    if (currentClip != nullptr)
    {
        auto clipLength = currentClip->getPosition().getLength().inSeconds();
        auto sourceLength = currentClip->getSourceLength().inSeconds();
        auto timeStretchRatio = clipLength / sourceLength;

        // Calculate visible time range in source time domain
        auto visibleTimeStart = (sourceLength * zoomState.getScrollPosition()) * timeStretchRatio;
        auto visibleTimeEnd = visibleTimeStart + ((sourceLength / zoomState.getZoomLevel()) * timeStretchRatio);

        auto timeRange = tracktion::TimeRange(
            tracktion::TimePosition::fromSeconds(visibleTimeStart),
            tracktion::TimePosition::fromSeconds(visibleTimeEnd));

        if (sourceLength > 0.0)
        {
            // Enable anti-aliasing
            g.setImageResamplingQuality(juce::Graphics::highResamplingQuality);

            // Calculate gains from clip properties
            const auto gain = currentClip->getGain();
            const auto pan = thumbnail.getNumChannels() == 1 ? 0.0f : currentClip->getPan();
            const float pv = pan * gain;
            const float leftGain = (gain - pv);
            const float rightGain = (gain + pv);

            // Draw waveform
            g.setColour(juce::Colours::lime.withAlpha(0.7f));

            if (thumbnail.getTotalLength() > 0.0)
            {
                // Draw base waveform
                g.setGradientFill(juce::ColourGradient(
                    juce::Colours::lime.withAlpha(0.8f),
                    drawBounds.getTopLeft().toFloat(),
                    juce::Colours::lime.withAlpha(0.3f),
                    drawBounds.getBottomLeft().toFloat(),
                    false));

                auto sourceTimeRange = tracktion::TimeRange(
                    tracktion::TimePosition::fromSeconds(visibleTimeStart / timeStretchRatio),
                    tracktion::TimePosition::fromSeconds(visibleTimeEnd / timeStretchRatio));

                float maxGain = juce::jmax(leftGain, rightGain);
                thumbnail.drawChannels(g, drawBounds, sourceTimeRange, maxGain);

                // Get chop track and overlay its clips
                if (auto chopTrack = EngineHelpers::getChopTrack(edit))
                {
                    for (auto clip : chopTrack->getClips())
                    {
                        auto startTime = clip->getPosition().getStart().inSeconds();
                        auto endTime = clip->getPosition().getEnd().inSeconds();

                        // Skip clips that are completely outside the visible range
                        if (endTime < visibleTimeStart || startTime > visibleTimeEnd)
                            continue;

                        // Clamp clip times to visible range
                        auto clampedStartTime = juce::jlimit(visibleTimeStart, visibleTimeEnd, startTime);
                        auto clampedEndTime = juce::jlimit(visibleTimeStart, visibleTimeEnd, endTime);

                        // Calculate clip bounds in view coordinates
                        auto startX = drawBounds.getX() + ((clampedStartTime - visibleTimeStart) / (visibleTimeEnd - visibleTimeStart)) * drawBounds.getWidth();
                        auto endX = drawBounds.getX() + ((clampedEndTime - visibleTimeStart) / (visibleTimeEnd - visibleTimeStart)) * drawBounds.getWidth();

                        // Draw clip overlay
                        auto clipBounds = juce::Rectangle<float>(
                            startX, drawBounds.getY(),
                            endX - startX, drawBounds.getHeight());

                        g.setColour(juce::Colours::purple.withAlpha(0.3f));
                        g.fillRect(clipBounds);

                        // Draw clip border
                        g.setColour(juce::Colours::purple.withAlpha(0.5f));
                        g.drawRect(clipBounds, 1.0f);
                    }
                }
            }
        }

        // Draw center line
        g.setColour(juce::Colours::grey.withAlpha(0.3f));
        g.drawHorizontalLine(drawBounds.getCentreY(),
            drawBounds.getX(),
            drawBounds.getRight());

        // Draw beat markers
        auto& tempoSequence = edit.tempoSequence;
        auto position = createPosition(tempoSequence);

        // Find the first beat before our visible range
        position.set(tracktion::TimePosition::fromSeconds(timeRange.getStart().inSeconds()));
        auto currentBarsBeats = position.getBarsBeats();
        auto beatTime = timeRange.getStart().inSeconds() - (currentBarsBeats.beats.inBeats() * 60.0 / position.getTempo());

        // Draw all beat markers
        for (double time = beatTime; time <= timeRange.getEnd().inSeconds(); time += 60.0 / position.getTempo())
        {
            if (time >= timeRange.getStart().inSeconds() && time <= timeRange.getEnd().inSeconds())
            {
                auto beatX = drawBounds.getX() + ((time - timeRange.getStart().inSeconds()) / (timeRange.getEnd().inSeconds() - timeRange.getStart().inSeconds())) * drawBounds.getWidth();

                position.set(tracktion::TimePosition::fromSeconds(time));
                auto barsBeats = position.getBarsBeats();

                // Bar starts get a brighter color
                g.setColour(barsBeats.beats.inBeats() == 0
                    ? juce::Colours::white.withAlpha(0.4f)
                    : juce::Colours::white.withAlpha(0.2f));

                g.drawVerticalLine(static_cast<int>(beatX),
                    drawBounds.getY(),
                    drawBounds.getBottom());
            }
        }
    }
}

void ThumbnailComponent::resized()
{
    auto bounds = getLocalBounds();
    
    // Update playhead if it exists
    if (playhead != nullptr)
    {
        playhead->setRectangle(juce::Rectangle<float>(2.0f, (float)bounds.getY(), 2.0f, (float)bounds.getHeight()));
        updatePlayheadPosition();
    }
}

void ThumbnailComponent::changeListenerCallback(juce::ChangeBroadcaster*)
{
    repaint();
}

void ThumbnailComponent::updateThumbnail()
{
    auto audioTracks = te::getAudioTracks(edit);
    currentClip = nullptr; // Reset current clip reference

    for (auto track : audioTracks)
    {
        auto clips = track->getClips();

        for (auto clip : clips)
        {
            if (auto* waveClip = dynamic_cast<tracktion::engine::WaveAudioClip*>(clip))
            {
                auto audioFile = waveClip->getAudioFile();

                if (audioFile.isValid())
                {
                    currentClip = waveClip;
                    thumbnail.setNewFile(audioFile);
                    repaint();
                    break;
                }
            }
        }
    }
}

void ThumbnailComponent::mouseDown(const juce::MouseEvent& event)
{
    auto bounds = getLocalBounds();

    if (bounds.contains(event.getPosition()))
    {
        if (currentClip != nullptr)
        {
            // Calculate normalized position from click
            auto clickX = event.position.x - bounds.getX();
            auto normalizedPosition = (clickX / bounds.getWidth()) / zoomState.getZoomLevel() + zoomState.getScrollPosition();
            normalizedPosition = juce::jlimit(0.0, 1.0, normalizedPosition);

            auto sourceLength = currentClip->getPosition().getLength().inSeconds();
            auto newPosition = (normalizedPosition * sourceLength);

            transport.setPosition(tracktion::TimePosition::fromSeconds(newPosition));
        }
    }
}

void ThumbnailComponent::mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel)
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

void ThumbnailComponent::updatePlayheadPosition()
{
    if (playhead != nullptr)
    {
        auto bounds = getLocalBounds();
        auto drawBounds = bounds.reduced(2);

        // Get current position from transport (this is already in the correct time domain)
        auto currentPosition = transport.getPosition().inSeconds();

        if (currentClip != nullptr)
        {
            auto sourceLength = currentClip->getPosition().getLength().inSeconds();

            // Get current tempo information
            auto& tempoSequence = edit.tempoSequence;
            auto position = createPosition(tempoSequence);
            position.set(transport.getPosition());
            auto currentTempo = position.getTempo();

            // If playing, adjust scroll position to keep playhead centered
            if (transport.isPlaying())
            {
                // Calculate the normalized position that would put the playhead in the center
                auto normalizedCenter = currentPosition / sourceLength;
                
                // Calculate how much visible time we can see based on zoom level
                auto visibleTimeWidth = sourceLength / zoomState.getZoomLevel();
                
                // Calculate the scroll position that would center the playhead
                auto newScrollPos = normalizedCenter - (0.5 / zoomState.getZoomLevel());
                
                // Clamp the scroll position to valid range
                newScrollPos = juce::jlimit(0.0, 1.0 - (1.0 / zoomState.getZoomLevel()), newScrollPos);
                
                // Update scroll position
                zoomState.setScrollPosition(newScrollPos);
            }

            // Calculate visible time range in source time domain first
            auto visibleTimeStart = sourceLength * zoomState.getScrollPosition();
            auto visibleTimeEnd = visibleTimeStart + (sourceLength / zoomState.getZoomLevel());

            // Calculate normalized position directly in source time domain
            auto normalizedPosition = (currentPosition - visibleTimeStart) / (visibleTimeEnd - visibleTimeStart);

            // Only show playhead if it's in the visible range
            if (currentPosition >= visibleTimeStart && currentPosition <= visibleTimeEnd)
            {
                auto playheadX = drawBounds.getX() + (normalizedPosition * drawBounds.getWidth());
                playhead->setVisible(true);
                playhead->setTopLeftPosition(static_cast<int>(playheadX), drawBounds.getY());
            }
            else
            {
                playhead->setVisible(false);
            }
        }
        else
        {
            playhead->setVisible(false);
        }
    }
}

void ThumbnailComponent::zoomLevelChanged(double newZoomLevel)
{
    repaint();
}

void ThumbnailComponent::scrollPositionChanged(double newScrollPosition)
{
    repaint();
} 
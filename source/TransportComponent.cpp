#include "TransportComponent.h"

TransportComponent::TransportComponent (tracktion::engine::Edit& e)
    : edit (e),
      transport (e.getTransport()),
      thumbnail (e.engine, tracktion::AudioFile (e.engine), *this, &e),
      zoomLevel(1.0),
      scrollPosition(0.0)
{
    // Add and make visible all buttons
    addAndMakeVisible (playButton);
    addAndMakeVisible (stopButton);
    addAndMakeVisible (loopButton);
    addAndMakeVisible (timeDisplay);
    addAndMakeVisible (zoomInButton);
    addAndMakeVisible (zoomOutButton);
    addAndMakeVisible (automationReadButton);
    addAndMakeVisible (automationWriteButton);

    // Create and add automation lane for tempo
    automationLane = std::make_unique<AutomationLane> (edit);
    addAndMakeVisible (*automationLane);

    // Create and add crossfader automation lane
    crossfaderAutomationLane = std::make_unique<AutomationLane> (edit);
    
    // Find the crossfader parameter
    tracktion::engine::AutomatableParameter* crossfaderParam = nullptr;
    if (auto chopPlugin = EngineHelpers::getPluginFromMasterTrack(edit, ChopPlugin::xmlTypeName))
    {
        if (auto* plugin = chopPlugin.get())
        {
            crossfaderParam = plugin->getAutomatableParameterByID("crossfader");
        }
    }
    
    if (crossfaderParam == nullptr)
    {
        DBG("No crossfader parameter found");
    }
    
    if (crossfaderParam != nullptr)
    {
        crossfaderAutomationLane->setParameter(crossfaderParam);
    }
    
    addAndMakeVisible (*crossfaderAutomationLane);

    // Initialize thumbnail
    thumbnail.audioFileChanged();
    startTimerHz(30);

    // Setup button callbacks
    playButton.onClick = [this] {
        auto& tempoSequence = edit.tempoSequence;

        if (transport.isPlaying())
            transport.stop (false, false);
        else
        {
            // Ensure we're synced to tempo before playing
            auto position = createPosition (tempoSequence);
            position.set (transport.getPosition());

            // Update transport to sync with tempo
            transport.setPosition (position.getTime());
            transport.play (false);
        }
        updateTransportState();
    };

    stopButton.onClick = [this] {
        transport.stop (false, false);
        transport.setPosition (tracktion::TimePosition::fromSeconds (0.0));
        updateTransportState();
    };

    loopButton.setClickingTogglesState (true);
    loopButton.onClick = [this] {
        transport.looping = loopButton.getToggleState();
        updateTransportState();
    };

    // Add zoom button callbacks
    zoomInButton.onClick = [this] {
        setZoomLevel (zoomLevel * 1.5);
    };

    zoomOutButton.onClick = [this] {
        setZoomLevel (zoomLevel / 1.5);
    };

    // Initialize time display
    timeDisplay.setJustificationType (juce::Justification::centred);
    timeDisplay.setFont(CustomLookAndFeel::getMonospaceFont());
    updateTimeDisplay();

    // Setup playhead
    playhead = std::make_unique<juce::DrawableRectangle>();
    playhead->setFill (juce::FillType (juce::Colours::red));
    addAndMakeVisible (*playhead);

    // Listen to transport changes
    edit.getTransport().addChangeListener (this);

    updateThumbnail();

    // Set up automation buttons
    automationReadButton.setClickingTogglesState(true);
    automationWriteButton.setClickingTogglesState(true);
    
    // Set up colors for automation buttons
    automationReadButton.setColour(juce::TextButton::buttonOnColourId, juce::Colours::green.darker());
    automationWriteButton.setColour(juce::TextButton::buttonOnColourId, juce::Colours::red.darker());
    
    automationReadButton.setToggleState(edit.getAutomationRecordManager().isReadingAutomation(), juce::dontSendNotification);
    automationWriteButton.setToggleState(edit.getAutomationRecordManager().isWritingAutomation(), juce::dontSendNotification);
    
    automationReadButton.onClick = [this] {
        edit.getAutomationRecordManager().setReadingAutomation(automationReadButton.getToggleState());
    };
    
    automationWriteButton.onClick = [this] {
        edit.getAutomationRecordManager().setWritingAutomation(automationWriteButton.getToggleState());
    };
    
    // Register as automation listener
    edit.getAutomationRecordManager().addListener(this);
}

TransportComponent::~TransportComponent()
{
    // First stop the timer to prevent any callbacks
    stopTimer();
    
    // Remove ourselves as a change listener if transport is still valid
    if (&edit != nullptr && &(edit.getTransport()) != nullptr)
    {
        edit.getTransport().removeChangeListener (this);
    }

    // Remove automation listener
    edit.getAutomationRecordManager().removeListener(this);
}

void TransportComponent::timerCallback()
{
    updateTimeDisplay();
    updatePlayheadPosition();

    // Force thumbnail redraw during playback
    if (transport.isPlaying())
        repaint();
}

void TransportComponent::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    // Draw background
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    // Draw waveform area
    auto waveformBounds = bounds.removeFromTop (static_cast<int> (bounds.getHeight() * 0.5));
    g.setColour (juce::Colours::darkgrey.darker (0.7f));
    g.fillRect (waveformBounds);

    // Create drawing bounds
    auto drawBounds = waveformBounds.reduced (2);
    
    // DBG("Paint called - waveformBounds: " + juce::String(waveformBounds.toString()));
    // DBG("drawBounds: " + juce::String(drawBounds.toString()));

    // Draw waveform if we have a clip
    if (currentClip != nullptr)
    {
        // DBG("Using stored clip reference");
        auto sourceLength = currentClip->getSourceLength().inSeconds();
        // DBG("Source length: " + juce::String(sourceLength));
        
        auto timeRange = tracktion::TimeRange (
            tracktion::TimePosition::fromSeconds (sourceLength * scrollPosition),
            tracktion::TimePosition::fromSeconds (sourceLength * (scrollPosition + 1.0 / zoomLevel)));
        
        if (sourceLength > 0.0)
        {
            // DBG("sourceLength: " + juce::String(sourceLength));
            // DBG("scrollPosition: " + juce::String(scrollPosition));
            // DBG("zoomLevel: " + juce::String(zoomLevel));
            // DBG("timeRange: " + juce::String(timeRange.getStart().inSeconds()) + " to " + juce::String(timeRange.getEnd().inSeconds()));
            // DBG("Thumbnail total length: " + juce::String(thumbnail.getTotalLength()));

            // Enable anti-aliasing
            g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);

            // Draw waveform first (so beat markers appear on top)
            g.setColour (juce::Colours::lightblue.withAlpha (0.7f));
            
            if (thumbnail.getTotalLength() > 0.0)
            {
                thumbnail.drawChannels (g, drawBounds, timeRange, 0.8f);
                // DBG("Drew thumbnail channels");
            }
            else
            {
                DBG("Error: Thumbnail has no length");
            }
        }
        else
        {
            DBG("Error: Source has no length");
        }

        // Draw center line
        g.setColour (juce::Colours::grey.withAlpha (0.3f));
        g.drawHorizontalLine (drawBounds.getCentreY(),
            drawBounds.getX(),
            drawBounds.getRight());

        // Draw beat markers
        auto& tempoSequence = edit.tempoSequence;
        auto position = createPosition (tempoSequence);

        // Find the first beat before our visible range
        position.set (tracktion::TimePosition::fromSeconds (timeRange.getStart().inSeconds()));
        auto currentBarsBeats = position.getBarsBeats();
        auto beatTime = timeRange.getStart().inSeconds() - (currentBarsBeats.beats.inBeats() * 60.0 / position.getTempo());

        // Draw all beat markers
        for (double time = beatTime; time <= timeRange.getEnd().inSeconds(); time += 60.0 / position.getTempo())
        {
            if (time >= timeRange.getStart().inSeconds() && time <= timeRange.getEnd().inSeconds()) // Only draw if in visible range
            {
                auto beatX = drawBounds.getX() + ((time - timeRange.getStart().inSeconds()) / (timeRange.getEnd().inSeconds() - timeRange.getStart().inSeconds())) * drawBounds.getWidth();

                position.set (tracktion::TimePosition::fromSeconds (time));
                auto barsBeats = position.getBarsBeats();

                // Bar starts get a brighter color
                g.setColour (barsBeats.beats.inBeats() == 0
                                 ? juce::Colours::white.withAlpha (0.4f)
                                 : juce::Colours::white.withAlpha (0.2f));

                g.drawVerticalLine (static_cast<int> (beatX),
                    drawBounds.getY(),
                    drawBounds.getBottom());
            }
        }
    }
    else
    {
        DBG("No clip reference available");
    }
}

void TransportComponent::resized()
{
    auto bounds = getLocalBounds();

    // Reserve space for waveform display (50% of height)
    auto waveformBounds = bounds.removeFromTop(static_cast<int>(bounds.getHeight() * 0.5));

    // Reserve space for automation lanes (30% of original height, split between the two lanes)
    auto automationBounds = bounds.removeFromTop(static_cast<int>(getHeight() * 0.3));
    auto firstLaneBounds = automationBounds.removeFromTop(automationBounds.getHeight() / 2);
    automationLane->setBounds(firstLaneBounds);
    crossfaderAutomationLane->setBounds(automationBounds);

    // Layout transport controls in remaining space
    auto controlsBounds = bounds.reduced(5);
    auto buttonWidth = controlsBounds.getWidth() / 7; // 7 buttons now

    playButton.setBounds(controlsBounds.removeFromLeft(buttonWidth).reduced(2));
    stopButton.setBounds(controlsBounds.removeFromLeft(buttonWidth).reduced(2));
    loopButton.setBounds(controlsBounds.removeFromLeft(buttonWidth).reduced(2));
    automationReadButton.setBounds(controlsBounds.removeFromLeft(buttonWidth).reduced(2));
    automationWriteButton.setBounds(controlsBounds.removeFromLeft(buttonWidth).reduced(2));
    zoomInButton.setBounds(controlsBounds.removeFromLeft(buttonWidth).reduced(2));
    zoomOutButton.setBounds(controlsBounds.removeFromLeft(buttonWidth).reduced(2));
    timeDisplay.setBounds(controlsBounds.reduced(2));

    if (playhead != nullptr)
    {
        playhead->setRectangle(juce::Rectangle<float>(2.0f, (float)waveformBounds.getY(), 2.0f, (float)waveformBounds.getHeight()));
        updatePlayheadPosition();
    }
}

void TransportComponent::changeListenerCallback (juce::ChangeBroadcaster*)
{
    updateTransportState();
    repaint();
}

void TransportComponent::updateTimeDisplay()
{
    auto& tempoSequence = edit.tempoSequence;
    auto position = createPosition (tempoSequence);
    position.set (transport.getPosition());

    auto barsBeats = position.getBarsBeats();
    auto tempo = position.getTempo();
    auto timeSignature = position.getTimeSignature();

    auto seconds = transport.getPosition().inSeconds();
    auto minutes = (int) (seconds / 60.0);
    auto millis = (int) (seconds * 1000) % 1000;

    timeDisplay.setText (juce::String::formatted ("%02d:%02d:%03d | %d/%d | Bar %d | %.1f BPM",
                             minutes,
                             (int) seconds % 60,
                             millis,
                             timeSignature.numerator,
                             timeSignature.denominator,
                             barsBeats.bars + 1,
                             tempo),
        juce::dontSendNotification);
}

void TransportComponent::updatePlayheadPosition()
{
    if (playhead != nullptr)
    {
        // DBG("Updating playhead position");
        auto bounds = getLocalBounds();
        auto waveformBounds = bounds.removeFromTop (static_cast<int> (bounds.getHeight() * 0.5)).reduced (2);

        auto currentPosition = transport.getPosition().inSeconds();

        if (currentClip != nullptr)
        {
            auto sourceLength = currentClip->getSourceLength().inSeconds();

            // Calculate visible time range
            auto visibleTimeStart = sourceLength * scrollPosition;
            auto visibleTimeEnd = visibleTimeStart + (sourceLength / zoomLevel);

            // Calculate normalized position within visible range
            auto normalizedPosition = (currentPosition - visibleTimeStart) / (visibleTimeEnd - visibleTimeStart);

            // DBG("Current position: " + juce::String(currentPosition) + "s");
            // DBG("Visible range: " + juce::String(visibleTimeStart) + "s to " + juce::String(visibleTimeEnd) + "s");
            // DBG("Normalized position: " + juce::String(normalizedPosition));

            // Only show playhead if it's in the visible range
            if (currentPosition >= visibleTimeStart && currentPosition <= visibleTimeEnd)
            {
                auto playheadX = waveformBounds.getX() + (normalizedPosition * waveformBounds.getWidth());
                playhead->setVisible (true);
                playhead->setTopLeftPosition (static_cast<int> (playheadX), waveformBounds.getY());
                // DBG("Playhead visible at x: " + juce::String(playheadX));
            }
            else
            {
                playhead->setVisible (false);
                DBG("Playhead hidden - position outside visible range");
            }
        }
        else
        {
            playhead->setVisible (false);
            DBG("Playhead hidden - no current clip");
        }
    }
}

void TransportComponent::updateTransportState()
{
    playButton.setToggleState (transport.isPlaying(), juce::dontSendNotification);
    loopButton.setToggleState (transport.looping, juce::dontSendNotification);
}

void TransportComponent::updateThumbnail()
{
    auto audioTracks = te::getAudioTracks (edit);
    currentClip = nullptr; // Reset current clip reference

    DBG("Updating thumbnail");

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
                else
                {
                    DBG ("    Error: Invalid audio file");
                }
            }
        }
    }
}

void TransportComponent::mouseDown (juce::MouseEvent const& event)
{
    auto bounds = getLocalBounds();
    auto waveformBounds = bounds.removeFromTop (static_cast<int> (bounds.getHeight() * 0.7));

    if (waveformBounds.contains (event.getPosition()))
    {
        DBG("Mouse down in waveform bounds");

        if (currentClip != nullptr)
        {
            // Calculate normalized position from click
            auto clickX = event.position.x - waveformBounds.getX();
            auto normalizedPosition = (clickX / waveformBounds.getWidth()) / zoomLevel + scrollPosition;
            normalizedPosition = juce::jlimit (0.0, 1.0, normalizedPosition);

            auto sourceLength = currentClip->getSourceLength().inSeconds();
            // Get current tempo for adjustment
            auto newPosition = (normalizedPosition * sourceLength);

            transport.setPosition (tracktion::TimePosition::fromSeconds (newPosition));
            updateTransportState();
        }
        else
        {
            DBG("No clip available for position change");
        }
    }
}

void TransportComponent::mouseWheelMove (const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel)
{
    auto bounds = getLocalBounds();
    auto waveformBounds = bounds.removeFromTop (static_cast<int> (bounds.getHeight() * 0.7));

    if (waveformBounds.contains (event.getPosition()))
    {
        // Handle horizontal scrolling with shift key or horizontal wheel
        if (event.mods.isShiftDown() || wheel.deltaX != 0.0f)
        {
            auto delta = wheel.deltaX != 0.0f ? wheel.deltaX : wheel.deltaY;
            auto newScrollPos = scrollPosition - (delta * 0.1);
            setScrollPosition (newScrollPos);
        }
        // Handle zooming
        else if (wheel.deltaY != 0.0f)
        {
            auto zoomFactor = wheel.deltaY > 0 ? 1.1 : 0.9;

            // Calculate the position under the mouse as a fraction of the visible width
            auto mouseXProportion = (event.position.x - waveformBounds.getX()) / (float) waveformBounds.getWidth();

            // Get the time position under the mouse
            auto oldTimePosition = (mouseXProportion + scrollPosition) / zoomLevel;

            // Apply the new zoom level
            setZoomLevel (zoomLevel * zoomFactor);

            // Calculate and set the new scroll position to keep the mouse point steady
            auto newScrollPos = oldTimePosition * zoomLevel - mouseXProportion;
            setScrollPosition (newScrollPos);
        }
    }
}

void TransportComponent::setZoomLevel (double newLevel)
{
    zoomLevel = juce::jlimit (minZoom, maxZoom, newLevel);
    repaint();
}

void TransportComponent::setScrollPosition (double newPosition)
{
    // Limit scrolling based on zoom level
    auto maxScroll = getMaxScrollPosition();
    scrollPosition = juce::jlimit (0.0, maxScroll, newPosition);
    repaint();
}

double TransportComponent::getMaxScrollPosition() const
{
    return zoomLevel > 1.0 ? 1.0 - (1.0 / zoomLevel) : 0.0;
}

void TransportComponent::automationModeChanged()
{
    bool isReading = edit.getAutomationRecordManager().isReadingAutomation();
    bool isWriting = edit.getAutomationRecordManager().isWritingAutomation();
    
    automationReadButton.setToggleState(isReading, juce::dontSendNotification);
    automationWriteButton.setToggleState(isWriting, juce::dontSendNotification);
    
    automationReadButton.setColour(juce::TextButton::textColourOnId, isReading ? juce::Colours::white : juce::Colours::grey);
    automationWriteButton.setColour(juce::TextButton::textColourOnId, isWriting ? juce::Colours::white : juce::Colours::grey);
}

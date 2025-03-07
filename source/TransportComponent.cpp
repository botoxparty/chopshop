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
    addAndMakeVisible (gridSizeComboBox);

    // Set up play and stop button shapes
    playButton.setShape(getPlayPath(), false, true, false);
    playButton.setOutline(juce::Colours::white, 1.0f);
    playButton.setColours(juce::Colours::white, juce::Colours::lightgrey.darker(0.2f), juce::Colours::white.darker(0.2f));
    playButton.setClickingTogglesState(true);

    stopButton.setShape(getStopPath(), false, true, false);
    stopButton.setOutline(juce::Colours::white, 1.0f);
    stopButton.setColours(juce::Colours::white, juce::Colours::lightgrey.darker(0.2f), juce::Colours::white.darker(0.2f));

    // Setup grid size combo box
    gridSizeComboBox.addItem("1/16", 1);  // 0.0625
    gridSizeComboBox.addItem("1/8", 2);   // 0.125
    gridSizeComboBox.addItem("1/4", 3);   // 0.25
    gridSizeComboBox.addItem("1/2", 4);   // 0.5
    gridSizeComboBox.addItem("1", 5);     // 1.0
    gridSizeComboBox.setSelectedId(3);    // Default to 1/4
    
    gridSizeComboBox.onChange = [this] {
        if (crossfaderAutomationLane != nullptr) {
            float division = 0.25f; // Default
            switch (gridSizeComboBox.getSelectedId()) {
                case 1: division = 0.0625f; break; // 1/16
                case 2: division = 0.125f; break;  // 1/8
                case 3: division = 0.25f; break;   // 1/4
                case 4: division = 0.5f; break;    // 1/2
                case 5: division = 1.0f; break;    // 1
            }
            crossfaderAutomationLane->setGridDivision(division);
        }
    };

    // Create and add crossfader automation lane
    crossfaderAutomationLane = std::make_unique<CrossfaderAutomationLane> (edit);
    
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

    // Create and add automation lane for tempo
    automationLane = std::make_unique<AutomationLane> (edit);
    addAndMakeVisible (*automationLane);
    
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
    timeDisplay.setFont(CustomLookAndFeel::getMonospaceFont().withHeight(18.0f));
    timeDisplay.setColour(juce::Label::textColourId, juce::Colours::white);
    timeDisplay.setColour(juce::Label::backgroundColourId, juce::Colours::darkgrey.darker(0.7f));
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

            // Calculate gains from clip properties
            const auto gain = currentClip->getGain();
            const auto pan = thumbnail.getNumChannels() == 1 ? 0.0f : currentClip->getPan();
            const float pv = pan * gain;
            const float leftGain = (gain - pv);
            const float rightGain = (gain + pv);
            
            // Draw waveform first (so beat markers appear on top)
            g.setColour (juce::Colours::lime.withAlpha (0.7f));
            
            if (thumbnail.getTotalLength() > 0.0)
            {
                // Get crossfader parameter
                tracktion::engine::AutomatableParameter* crossfaderParam = nullptr;
                if (auto chopPlugin = EngineHelpers::getPluginFromMasterTrack(edit, ChopPlugin::xmlTypeName))
                {
                    if (auto* plugin = chopPlugin.get())
                    {
                        crossfaderParam = plugin->getAutomatableParameterByID("crossfader");
                    }
                }

                // Instead of drawing channels separately, draw a combined view
                float maxGain = jmax(leftGain, rightGain);

                if (crossfaderParam != nullptr)
                {
                    // Calculate visible time range
                    auto visibleTimeStart = timeRange.getStart().inSeconds();
                    auto visibleTimeEnd = timeRange.getEnd().inSeconds();
                    
                    // Draw in small segments to capture crossfader changes
                    const int numSegments = drawBounds.getWidth();
                    const double timePerSegment = (visibleTimeEnd - visibleTimeStart) / numSegments;
                    
                    for (int i = 0; i < numSegments; i++)
                    {
                        auto segmentTime = visibleTimeStart + (i * timePerSegment);
                        auto segmentEndTime = segmentTime + timePerSegment;
                        
                        // Get crossfader value at this time
                        auto crossfaderValue = crossfaderParam->getCurve().getValueAt(tracktion::TimePosition::fromSeconds(segmentTime));
                        
                        // Create segment bounds
                        auto segmentBounds = drawBounds.withWidth(1).withX(drawBounds.getX() + i);
                        
                        // Set color based on crossfader value
                        juce::Colour waveformColor = crossfaderValue >= 0.5f ? juce::Colours::purple : juce::Colours::lime;
                        
                        // Create gradient for this segment
                        g.setGradientFill(juce::ColourGradient(
                            waveformColor.withAlpha(0.8f),
                            segmentBounds.getTopLeft().toFloat(),
                            waveformColor.withAlpha(0.3f),
                            segmentBounds.getBottomLeft().toFloat(),
                            false));
                        
                        // Draw this segment
                        thumbnail.drawChannels(g, 
                                            segmentBounds,
                                            tracktion::TimeRange(tracktion::TimePosition::fromSeconds(segmentTime),
                                                               tracktion::TimePosition::fromSeconds(segmentEndTime)),
                                            maxGain);
                    }
                    
                    // Add a subtle outline
                    g.setColour(juce::Colours::grey.withAlpha(0.4f));
                    g.drawRect(drawBounds.toFloat(), 1.0f);
                }
                else
                {
                    // Fallback if no crossfader parameter
                    g.setGradientFill(juce::ColourGradient(
                        juce::Colours::lime.withAlpha(0.8f),
                        drawBounds.getTopLeft().toFloat(),
                        juce::Colours::lime.withAlpha(0.3f),
                        drawBounds.getBottomLeft().toFloat(),
                        false));
                        
                    thumbnail.drawChannels(g, drawBounds, timeRange, maxGain);
                }
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
    automationLane->setBounds(automationBounds);
    crossfaderAutomationLane->setBounds(firstLaneBounds);

    // Layout transport controls in remaining space
    auto controlsBounds = bounds.reduced(5);
    
    // Give more space to the time display and grid control
    auto timeDisplayWidth = controlsBounds.getWidth() * 0.3; // 30% of the width
    auto gridControlWidth = controlsBounds.getWidth() * 0.1; // 10% of the width
    auto buttonsWidth = controlsBounds.getWidth() - timeDisplayWidth - gridControlWidth;
    auto buttonWidth = buttonsWidth / 7; // 7 buttons
    
    auto buttonBounds = controlsBounds.removeFromLeft(buttonsWidth);
    
    playButton.setBounds(buttonBounds.removeFromLeft(buttonWidth).reduced(2));
    stopButton.setBounds(buttonBounds.removeFromLeft(buttonWidth).reduced(2));
    loopButton.setBounds(buttonBounds.removeFromLeft(buttonWidth).reduced(2));
    automationReadButton.setBounds(buttonBounds.removeFromLeft(buttonWidth).reduced(2));
    automationWriteButton.setBounds(buttonBounds.removeFromLeft(buttonWidth).reduced(2));
    zoomInButton.setBounds(buttonBounds.removeFromLeft(buttonWidth).reduced(2));
    zoomOutButton.setBounds(buttonBounds.removeFromLeft(buttonWidth).reduced(2));
    
    gridSizeComboBox.setBounds(controlsBounds.removeFromLeft(gridControlWidth).reduced(2));
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
    bool isPlaying = transport.isPlaying();
    playButton.setToggleState(isPlaying, juce::dontSendNotification);
    playButton.setShape(isPlaying ? getPausePath() : getPlayPath(), false, true, false);
    loopButton.setToggleState(transport.looping, juce::dontSendNotification);
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
                    
                    // Update automation lanes with the new source length
                    auto sourceLength = waveClip->getSourceLength().inSeconds();
                    if (automationLane != nullptr)
                        automationLane->setSourceLength(sourceLength);
                    if (crossfaderAutomationLane != nullptr)
                        crossfaderAutomationLane->setSourceLength(sourceLength);
                        
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
    
    // Update automation lanes
    if (automationLane != nullptr)
        automationLane->setZoomLevel(zoomLevel);
    if (crossfaderAutomationLane != nullptr)
        crossfaderAutomationLane->setZoomLevel(zoomLevel);
        
    repaint();
}

void TransportComponent::setScrollPosition (double newPosition)
{
    // Limit scrolling based on zoom level
    auto maxScroll = getMaxScrollPosition();
    scrollPosition = juce::jlimit (0.0, maxScroll, newPosition);
    
    // Update automation lanes
    if (automationLane != nullptr)
        automationLane->setScrollPosition(scrollPosition);
    if (crossfaderAutomationLane != nullptr)
        crossfaderAutomationLane->setScrollPosition(scrollPosition);
        
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

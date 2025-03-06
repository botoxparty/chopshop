#include "TransportComponent.h"

TransportComponent::TransportComponent(tracktion::engine::Edit& e)
    : edit(e),
      transport(e.getTransport()),
      thumbnail(e.engine, tracktion::AudioFile(e.engine), *this, &e)
{
    // Add and make visible all buttons
    addAndMakeVisible(playButton);
    addAndMakeVisible(stopButton);
    addAndMakeVisible(recordButton);
    addAndMakeVisible(loopButton);
    addAndMakeVisible(timeDisplay);
    addAndMakeVisible(zoomInButton);
    addAndMakeVisible(zoomOutButton);
    
    // Create and add automation lane
    automationLane = std::make_unique<AutomationLane>(edit);
    addAndMakeVisible(*automationLane);
    
    // Setup button callbacks
    playButton.onClick = [this] { 
        auto& transport = edit.getTransport();
        auto& tempoSequence = edit.tempoSequence;
        
        if (transport.isPlaying())
            transport.stop(false, false);
        else {
            // Ensure we're synced to tempo before playing
            auto position = createPosition(tempoSequence);
            position.set(transport.getPosition());
            
            // Get current tempo and time signature
            auto tempo = position.getTempo();
            auto timeSignature = position.getTimeSignature();
            
            // Update transport to sync with tempo
            transport.setPosition(position.getTime());
            transport.play(false);
        }
        updateTransportState();
    };
    
    stopButton.onClick = [this] {
        auto& transport = edit.getTransport();
        transport.stop(false, false);
        transport.setPosition(tracktion::TimePosition::fromSeconds(0.0));
        updateTransportState();
    };
    
    recordButton.onClick = [this] {
        auto& transport = edit.getTransport();
        if (transport.isRecording())
            transport.stop(false, false);
        else
            transport.record(false);
        updateTransportState();
    };
    
    loopButton.setClickingTogglesState(true);
    loopButton.onClick = [this] {
        auto& transport = edit.getTransport();
        transport.looping = loopButton.getToggleState();
        updateTransportState();
    };
    
    // Add zoom button callbacks
    zoomInButton.onClick = [this] {
        setZoomLevel(zoomLevel * 1.5);
    };
    
    zoomOutButton.onClick = [this] {
        setZoomLevel(zoomLevel / 1.5);
    };
    
    // Initialize time display
    timeDisplay.setJustificationType(juce::Justification::centred);
    updateTimeDisplay();
    
    // Setup playhead
    playhead = std::make_unique<juce::DrawableRectangle>();
    playhead->setFill(juce::FillType(juce::Colours::red));
    addAndMakeVisible(*playhead);
    
    // Start timer for updates
    startTimerHz(30);
    
    // Listen to transport changes
    edit.getTransport().addChangeListener(this);
}

TransportComponent::~TransportComponent()
{
    edit.getTransport().removeChangeListener(this);
    stopTimer();
}

void TransportComponent::timerCallback()
{
    updateTimeDisplay();
    updatePlayheadPosition();
    repaint();
}

void TransportComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();
    
    // Draw background
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
    
    // Draw waveform area
    auto waveformBounds = bounds.removeFromTop(static_cast<int>(bounds.getHeight() * 0.5));
    g.setColour(juce::Colours::darkgrey.darker(0.7f));
    g.fillRect(waveformBounds);
    
    // Create drawing bounds
    auto drawBounds = waveformBounds.reduced(2);
    
    // Draw waveform if we have one
    if (auto track = EngineHelpers::getOrInsertAudioTrackAt(edit, 0))
    {
        if (!track->getClips().isEmpty())
        {
            if (auto* clip = dynamic_cast<tracktion::engine::WaveAudioClip*>(track->getClips().getFirst()))
            {
                auto sourceLength = clip->getSourceLength().inSeconds();
                auto timeRange = tracktion::TimeRange(
                    tracktion::TimePosition::fromSeconds(sourceLength * scrollPosition),
                    tracktion::TimePosition::fromSeconds(sourceLength * (scrollPosition + 1.0 / zoomLevel)));
                
                // Enable anti-aliasing
                g.setImageResamplingQuality(juce::Graphics::highResamplingQuality);
                
                // Draw waveform first (so beat markers appear on top)
                g.setColour(juce::Colours::lightblue.withAlpha(0.7f));
                thumbnail.drawChannels(g, drawBounds, timeRange, 0.8f);
                
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
                    if (time >= timeRange.getStart().inSeconds() && time <= timeRange.getEnd().inSeconds()) // Only draw if in visible range
                    {
                        auto beatX = drawBounds.getX() + 
                            ((time - timeRange.getStart().inSeconds()) / (timeRange.getEnd().inSeconds() - timeRange.getStart().inSeconds())) * drawBounds.getWidth();
                        
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
    }
}

void TransportComponent::resized()
{
    auto bounds = getLocalBounds();
    
    // Reserve space for waveform display (50% of height)
    auto waveformBounds = bounds.removeFromTop(static_cast<int>(bounds.getHeight() * 0.5));
    
    // Reserve space for automation lane (30% of original height)
    auto automationBounds = bounds.removeFromTop(static_cast<int>(getHeight() * 0.3));
    automationLane->setBounds(automationBounds);
    
    // Layout transport controls in remaining space
    auto controlsBounds = bounds.reduced(5);
    auto buttonWidth = controlsBounds.getWidth() / 7; // Updated to accommodate zoom buttons
    
    playButton.setBounds(controlsBounds.removeFromLeft(buttonWidth).reduced(2));
    stopButton.setBounds(controlsBounds.removeFromLeft(buttonWidth).reduced(2));
    recordButton.setBounds(controlsBounds.removeFromLeft(buttonWidth).reduced(2));
    loopButton.setBounds(controlsBounds.removeFromLeft(buttonWidth).reduced(2));
    zoomOutButton.setBounds(controlsBounds.removeFromLeft(buttonWidth).reduced(2));
    zoomInButton.setBounds(controlsBounds.removeFromLeft(buttonWidth).reduced(2));
    timeDisplay.setBounds(controlsBounds.reduced(2));
    
    if (playhead != nullptr)
    {
        playhead->setRectangle(juce::Rectangle<float>(2.0f, (float)waveformBounds.getY(),
                                                   2.0f, (float)waveformBounds.getHeight()));
        updatePlayheadPosition();
    }
}

void TransportComponent::changeListenerCallback(juce::ChangeBroadcaster*)
{
    updateTransportState();
}

void TransportComponent::updateTimeDisplay()
{
    auto& transport = edit.getTransport();
    auto& tempoSequence = edit.tempoSequence;
    auto position = createPosition(tempoSequence);
    position.set(transport.getPosition());
    
    auto barsBeats = position.getBarsBeats();
    auto tempo = position.getTempo();
    auto timeSignature = position.getTimeSignature();
    
    auto seconds = transport.getPosition().inSeconds();
    auto minutes = (int)(seconds / 60.0);
    auto millis = (int)(seconds * 1000) % 1000;
    
    timeDisplay.setText(juce::String::formatted("%02d:%02d:%03d | %d/%d | Bar %d | %.1f BPM", 
                                              minutes, (int)seconds % 60, millis,
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
        auto bounds = getLocalBounds();
        auto waveformBounds = bounds.removeFromTop(bounds.getHeight() * 0.7);
        
        auto& transport = edit.getTransport();
        auto normalizedPosition = 0.0;
        
        if (auto track = EngineHelpers::getOrInsertAudioTrackAt(edit, 0))
        {
            if (!track->getClips().isEmpty())
            {
                if (auto* clip = dynamic_cast<tracktion::engine::WaveAudioClip*>(track->getClips().getFirst()))
                {
                    auto sourceLength = clip->getSourceLength().inSeconds();
                    auto currentPosition = transport.getPosition().inSeconds();
                    
                    // Get current tempo for adjustment
                    auto& tempoSequence = edit.tempoSequence;
                    auto position = createPosition(tempoSequence);
                    position.set(transport.getPosition());
                    auto currentTempo = position.getTempo();
                    auto tempoRatio = currentTempo / clip->getLoopInfo().getBpm(clip->getAudioFile().getInfo());
                    
                    // Calculate normalized position accounting for tempo
                    auto adjustedPosition = currentPosition * tempoRatio;
                    normalizedPosition = adjustedPosition / sourceLength;
                    
                    // Adjust for zoom and scroll
                    normalizedPosition = (normalizedPosition - scrollPosition) * zoomLevel;
                }
            }
        }
        
        // Only show playhead if it's in the visible range
        if (normalizedPosition >= 0.0 && normalizedPosition <= 1.0)
        {
            auto playheadX = waveformBounds.getX() + waveformBounds.getWidth() * normalizedPosition;
            playhead->setVisible(true);
            playhead->setTopLeftPosition((int)playheadX, waveformBounds.getY());
        }
        else
        {
            playhead->setVisible(false);
        }
    }
}

void TransportComponent::updateTransportState()
{
    auto& transport = edit.getTransport();
    
    playButton.setToggleState(transport.isPlaying(), juce::dontSendNotification);
    recordButton.setToggleState(transport.isRecording(), juce::dontSendNotification);
    loopButton.setToggleState(transport.looping, juce::dontSendNotification);
    
    updateThumbnail();
}

void TransportComponent::updateThumbnail()
{
    if (auto track = EngineHelpers::getOrInsertAudioTrackAt(edit, 0))
    {
        if (!track->getClips().isEmpty())
        {
            if (auto* clip = dynamic_cast<tracktion::engine::WaveAudioClip*>(track->getClips().getFirst()))
            {
                auto audioFile = clip->getPlaybackFile();
                if (audioFile.getFile().existsAsFile())
                {
                    thumbnail.setSource(new juce::FileInputSource(audioFile.getFile()));
                }
            }
        }
    }
}

void TransportComponent::mouseDown(juce::MouseEvent const& event)
{
    auto bounds = getLocalBounds();
    auto waveformBounds = bounds.removeFromTop(static_cast<int>(bounds.getHeight() * 0.7));
    
    if (waveformBounds.contains(event.getPosition()))
    {
        auto& transport = edit.getTransport();
        
        if (auto track = EngineHelpers::getOrInsertAudioTrackAt(edit, 0))
        {
            if (!track->getClips().isEmpty())
            {
                if (auto* clip = dynamic_cast<tracktion::engine::WaveAudioClip*>(track->getClips().getFirst()))
                {
                    // Calculate normalized position from click
                    auto clickX = event.position.x - waveformBounds.getX();
                    auto normalizedPosition = (clickX / waveformBounds.getWidth()) / zoomLevel + scrollPosition;
                    normalizedPosition = juce::jlimit(0.0, 1.0, normalizedPosition);
                    
                    auto sourceLength = clip->getSourceLength().inSeconds();
                    
                    // Get current tempo for adjustment
                    auto& tempoSequence = edit.tempoSequence;
                    auto position = createPosition(tempoSequence);
                    position.set(transport.getPosition());
                    auto currentTempo = position.getTempo();
                    auto tempoRatio = currentTempo / clip->getLoopInfo().getBpm(clip->getAudioFile().getInfo());
                    
                    // Calculate the actual position accounting for tempo
                    auto newPosition = (normalizedPosition * sourceLength) / tempoRatio;
                    
                    transport.setPosition(tracktion::TimePosition::fromSeconds(newPosition));
                    updateTransportState();
                }
            }
        }
    }
}

void TransportComponent::mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel)
{
    auto bounds = getLocalBounds();
    auto waveformBounds = bounds.removeFromTop(static_cast<int>(bounds.getHeight() * 0.7));
    
    if (waveformBounds.contains(event.getPosition()))
    {
        // Handle horizontal scrolling with shift key or horizontal wheel
        if (event.mods.isShiftDown() || wheel.deltaX != 0.0f)
        {
            auto delta = wheel.deltaX != 0.0f ? wheel.deltaX : wheel.deltaY;
            auto newScrollPos = scrollPosition - (delta * 0.1);
            setScrollPosition(newScrollPos);
        }
        // Handle zooming
        else if (wheel.deltaY != 0.0f)
        {
            auto zoomFactor = wheel.deltaY > 0 ? 1.1 : 0.9;
            
            // Calculate the position under the mouse as a fraction of the visible width
            auto mouseXProportion = (event.position.x - waveformBounds.getX()) / (float)waveformBounds.getWidth();
            
            // Get the time position under the mouse
            auto oldTimePosition = (mouseXProportion + scrollPosition) / zoomLevel;
            
            // Apply the new zoom level
            setZoomLevel(zoomLevel * zoomFactor);
            
            // Calculate and set the new scroll position to keep the mouse point steady
            auto newScrollPos = oldTimePosition * zoomLevel - mouseXProportion;
            setScrollPosition(newScrollPos);
        }
    }
}

void TransportComponent::setZoomLevel(double newLevel)
{
    zoomLevel = juce::jlimit(minZoom, maxZoom, newLevel);
    repaint();
}

void TransportComponent::setScrollPosition(double newPosition)
{
    // Limit scrolling based on zoom level
    auto maxScroll = getMaxScrollPosition();
    scrollPosition = juce::jlimit(0.0, maxScroll, newPosition);
    repaint();
}

double TransportComponent::getMaxScrollPosition() const
{
    return zoomLevel > 1.0 ? 1.0 - (1.0 / zoomLevel) : 0.0;
}

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
    auto waveformBounds = bounds.removeFromTop(static_cast<int>(bounds.getHeight() * 0.7));
    g.setColour(juce::Colours::darkgrey);
    g.fillRect(waveformBounds);
    
    // Draw timeline
    g.setColour(juce::Colours::white);
    auto timelineBounds = waveformBounds.removeFromBottom(20);
    g.drawRect(timelineBounds);
    
    // Draw waveform if we have one
    if (auto track = EngineHelpers::getOrInsertAudioTrackAt(edit, 0))
    {
        if (!track->getClips().isEmpty())
        {
            if (auto* clip = dynamic_cast<tracktion::engine::WaveAudioClip*>(track->getClips().getFirst()))
            {
                g.setColour(juce::Colours::lightblue);
                auto timeRange = tracktion::TimeRange(
                    tracktion::TimePosition::fromSeconds(0.0),
                    tracktion::TimePosition::fromSeconds(clip->getSourceLength().inSeconds()));
                thumbnail.drawChannels(g, waveformBounds.reduced(2), timeRange, 1.0f);
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
    auto buttonWidth = controlsBounds.getWidth() / 5;
    
    playButton.setBounds(controlsBounds.removeFromLeft(buttonWidth).reduced(2));
    stopButton.setBounds(controlsBounds.removeFromLeft(buttonWidth).reduced(2));
    recordButton.setBounds(controlsBounds.removeFromLeft(buttonWidth).reduced(2));
    loopButton.setBounds(controlsBounds.removeFromLeft(buttonWidth).reduced(2));
    timeDisplay.setBounds(controlsBounds.reduced(2));
    
    // Only set the playhead rectangle dimensions here, not its position
    if (playhead != nullptr)
    {
        playhead->setRectangle(juce::Rectangle<float>(2.0f, (float)waveformBounds.getY(),
                                                    2.0f, (float)waveformBounds.getHeight()));
        updatePlayheadPosition(); // Let the dedicated method handle positioning
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
                    normalizedPosition = juce::jlimit(0.0, 1.0, normalizedPosition);
                }
            }
        }
        
        auto playheadX = waveformBounds.getX() + waveformBounds.getWidth() * normalizedPosition;
        playhead->setTopLeftPosition((int)playheadX, waveformBounds.getY());
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
    
    // Check if click is within waveform area
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
                    auto normalizedPosition = clickX / waveformBounds.getWidth();
                    normalizedPosition = juce::jlimit(0.0f, 1.0f, normalizedPosition);
                    
                    // Get the source length and calculate the actual position
                    auto sourceLength = clip->getSourceLength().inSeconds();
                    
                    // Get current tempo for adjustment
                    auto& tempoSequence = edit.tempoSequence;
                    auto position = createPosition(tempoSequence);
                    position.set(transport.getPosition());
                    auto currentTempo = position.getTempo();
                    auto tempoRatio = currentTempo / clip->getLoopInfo().getBpm(clip->getAudioFile().getInfo());
                    
                    // Calculate the actual position accounting for tempo
                    auto newPosition = (normalizedPosition * sourceLength) / tempoRatio;

                    // debug the event.
                    DBG("Mouse down event at position: " + juce::String(event.position.x) + " " + juce::String(event.position.y));
                    DBG("Waveform bounds: " + juce::String(waveformBounds.getX()) + " " + juce::String(waveformBounds.getY()) + " " + juce::String(waveformBounds.getWidth()) + " " + juce::String(waveformBounds.getHeight()));
                    DBG("New position: " + juce::String(newPosition));
                    DBG("Current tempo: " + juce::String(currentTempo));
                    DBG("Tempo ratio: " + juce::String(tempoRatio));
                    DBG("Source length: " + juce::String(sourceLength));
                    DBG("Normalized position: " + juce::String(normalizedPosition));
                    DBG("Transport position: " + juce::String(transport.getPosition().inSeconds()));

                    
                    // Set the transport position
                    transport.setPosition(tracktion::TimePosition::fromSeconds(newPosition));
                    updateTransportState();
                }
            }
        }
    }
} 
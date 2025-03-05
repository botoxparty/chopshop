#include "TransportComponent.h"

TransportComponent::TransportComponent(tracktion::engine::Edit& e)
    : edit(e),
      thumbnail(e.engine, tracktion::AudioFile(e.engine), *this, &e)
{
    // Add and make visible all buttons
    addAndMakeVisible(playButton);
    addAndMakeVisible(stopButton);
    addAndMakeVisible(recordButton);
    addAndMakeVisible(loopButton);
    addAndMakeVisible(timeDisplay);
    
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
    
    // Reserve space for waveform display
    auto waveformBounds = bounds.removeFromTop(static_cast<int>(bounds.getHeight() * 0.7));
    
    // Layout transport controls
    auto controlsBounds = bounds.reduced(5);
    auto buttonWidth = controlsBounds.getWidth() / 5;
    
    playButton.setBounds(controlsBounds.removeFromLeft(buttonWidth).reduced(2));
    stopButton.setBounds(controlsBounds.removeFromLeft(buttonWidth).reduced(2));
    recordButton.setBounds(controlsBounds.removeFromLeft(buttonWidth).reduced(2));
    loopButton.setBounds(controlsBounds.removeFromLeft(buttonWidth).reduced(2));
    timeDisplay.setBounds(controlsBounds.reduced(2));
    
    // Position playhead
    if (playhead != nullptr)
    {
        auto playheadX = (float)waveformBounds.getX() + 
                        (float)waveformBounds.getWidth() * 
                        (float)(edit.getTransport().getPosition() / edit.getLength());
        playhead->setRectangle(juce::Rectangle<float>(2.0f, (float)waveformBounds.getY(),
                                                    2.0f, (float)waveformBounds.getHeight()));
        playhead->setTopLeftPosition((int)playheadX, waveformBounds.getY());
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
        auto& tempoSequence = edit.tempoSequence;
        auto position = createPosition(tempoSequence);
        position.set(transport.getPosition());
        
        // Get current tempo
        auto currentTempo = position.getTempo();
        
        if (auto track = EngineHelpers::getOrInsertAudioTrackAt(edit, 0))
        {
            if (!track->getClips().isEmpty())
            {
                if (auto* clip = dynamic_cast<tracktion::engine::WaveAudioClip*>(track->getClips().getFirst()))
                {
                    // Get the source length and current position
                    auto sourceLength = clip->getSourceLength().inSeconds();
                    auto currentPosition = transport.getPosition().inSeconds();
                    
                    // Calculate tempo ratio (1.0 = normal speed, 0.5 = half speed, etc)
                    auto tempoRatio = currentTempo / clip->getLoopInfo().getBpm(clip->getAudioFile().getInfo());
                    
                    // Adjust the position based on tempo
                    auto adjustedPosition = currentPosition * tempoRatio;
                    auto adjustedLength = sourceLength;
                    
                    if (adjustedLength > 0.0)
                    {
                        // Calculate normalized position
                        auto normalizedPosition = adjustedPosition / adjustedLength;
                        normalizedPosition = juce::jlimit(0.0, 1.0, normalizedPosition);
                        
                        auto playheadX = waveformBounds.getX() + waveformBounds.getWidth() * normalizedPosition;
                        playhead->setTopLeftPosition((int)playheadX, waveformBounds.getY());
                    }
                }
            }
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
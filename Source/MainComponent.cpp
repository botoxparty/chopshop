#include "MainComponent.h"


//==============================================================================
MainComponent::MainComponent()
{
    addAndMakeVisible(openButton);
    addAndMakeVisible(saveButton);
    addAndMakeVisible(playButton);
    addAndMakeVisible(stopButton);
    addAndMakeVisible(tempoSlider);
    addAndMakeVisible(crossfaderSlider);

    // Create and set up thumbnail
    thumbnail = std::make_unique<Thumbnail>(edit.getTransport());
    addAndMakeVisible(*thumbnail);
    thumbnail->start();

    setSize(600, 400);
    
    playButton.setToggleState(false, juce::NotificationType::dontSendNotification);
    stopButton.setToggleState(true, juce::NotificationType::dontSendNotification);

    playButton.onClick = [this] { play(); };
    stopButton.onClick = [this] { stop(); };
    openButton.onClick = [this] { loadAudioFile(); };

    tempoSlider.setRange(30.0, 220.0, 0.1);
    tempoSlider.onDragEnd = [this] { updateTempo(); };

    // Setup crossfader
    crossfaderSlider.setRange(0.0, 1.0, 0.01);
    crossfaderSlider.setValue(0.0, juce::dontSendNotification);
    crossfaderSlider.onValueChange = [this] { updateCrossfader(); };
}

MainComponent::~MainComponent()
{
}

//==============================================================================
void MainComponent::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    g.setFont (juce::FontOptions (16.0f));
    g.setColour (juce::Colours::white);
    // g.drawText ("Hello World!", getLocalBounds(), juce::Justification::centred, true);
}

void MainComponent::resized()
{
    auto bounds = getLocalBounds();
    auto controlsArea = bounds.removeFromLeft(220);  // Space for controls

    // Layout controls
    openButton.setBounds(controlsArea.removeFromTop(30).reduced(10, 5));
    saveButton.setBounds(controlsArea.removeFromTop(30).reduced(10, 5));
    playButton.setBounds(controlsArea.removeFromTop(30).reduced(10, 5));
    stopButton.setBounds(controlsArea.removeFromTop(30).reduced(10, 5));
    tempoSlider.setBounds(controlsArea.removeFromTop(30).reduced(10, 5));
    crossfaderSlider.setBounds(controlsArea.removeFromTop(30).reduced(10, 5));

    // Position thumbnail in remaining space
    bounds.reduce(10, 10);  // Add some padding
    thumbnail->setBounds(bounds);
}

void MainComponent::play()
{
    if(playState == PlayState::Playing) {
        return;
    }

    edit.getTransport().play(false);

    if(playState == PlayState::Stopped) 
    {
        playState = PlayState::Playing;
        // TODO: Implement play functionality 
        stopButton.setToggleState(false, juce::NotificationType::dontSendNotification);
        playButton.setToggleState(true, juce::NotificationType::dontSendNotification);
    }
    else if(playState == PlayState::Paused)
    {
        playState = PlayState::Playing;
        // TODO: Implement play functionality 
        stopButton.setToggleState(false, juce::NotificationType::dontSendNotification);
        playButton.setToggleState(true, juce::NotificationType::dontSendNotification);
    }
}

void MainComponent::stop()
{
    if(playState == PlayState::Stopped) {
        return;
    }

    edit.getTransport().stop(true, false);

    if(playState == PlayState::Playing) {
        playState = PlayState::Stopped;
        // TODO: Implement stop functionality
        stopButton.setToggleState(true, juce::NotificationType::dontSendNotification);
        playButton.setToggleState(false, juce::NotificationType::dontSendNotification);
    }
    else if(playState == PlayState::Paused) {
        playState = PlayState::Stopped;
        // TODO: Implement stop functionality
        stopButton.setToggleState(true, juce::NotificationType::dontSendNotification);
        playButton.setToggleState(false, juce::NotificationType::dontSendNotification);
    }
}

void MainComponent::loadAudioFile()
{
    EngineHelpers::browseForAudioFile(engine, [this](const juce::File& file)
    {
        handleFileSelection(file);
    });
}

void MainComponent::handleFileSelection(const juce::File& file)
{
    if (!file.existsAsFile())
        return;

    // Load clip into first track
    auto clip1 = EngineHelpers::loadAudioFileAsClip(edit, file);
    if (clip1)
    {
        // Disable auto tempo and pitch for first clip
        clip1->setAutoTempo(false);
        clip1->setAutoPitch(false);
        clip1->setTimeStretchMode(te::TimeStretcher::defaultMode);

        // Create second track and load the same file
        if (auto track2 = EngineHelpers::getOrInsertAudioTrackAt(edit, 1))
        {
            EngineHelpers::removeAllClips(*track2);
            
            // Add clip to second track
            if (auto clip2 = track2->insertWaveClip(file.getFileNameWithoutExtension(), file,
                { { tracktion::TimePosition::fromSeconds(1.0),
                    tracktion::TimeDuration::fromSeconds(clip1->getSourceLength().inSeconds()) }, {} }, 
                false))
            {
                // Configure second clip
                clip2->setAutoTempo(false);
                clip2->setAutoPitch(false);
                clip2->setTimeStretchMode(te::TimeStretcher::defaultMode);
                clip2->setGainDB(0.0f);
            }
        }

        baseTempo = 104.0;
        // Set initial tempo
        tempoSlider.setValue(baseTempo, juce::dontSendNotification);
        updateTempo();

        // Reset crossfader to first track
        crossfaderSlider.setValue(0.0, juce::dontSendNotification);
        updateCrossfader();

        // Update thumbnail with new audio file
        thumbnail->setFile(te::AudioFile(engine, file));

        play();
    }
}

void MainComponent::updateTempo()
{
    const double ratio = tempoSlider.getValue() / baseTempo;

    // Update first clip
    if (auto clip1 = getClip(0))
    {
        clip1->setSpeedRatio(ratio);
        clip1->setLength(tracktion::TimeDuration::fromSeconds(clip1->getSourceLength().inSeconds()) / clip1->getSpeedRatio(), true);
    }

    // Update second clip
    if (auto clip2 = getClip(1))
    {
        clip2->setSpeedRatio(ratio);
        clip2->setLength(tracktion::TimeDuration::fromSeconds(clip2->getSourceLength().inSeconds()) / clip2->getSpeedRatio(), true);
    }
}

te::WaveAudioClip::Ptr MainComponent::getClip(int trackIndex)
{
    if (auto track = EngineHelpers::getOrInsertAudioTrackAt(edit, trackIndex))
        if (auto clip = dynamic_cast<te::WaveAudioClip*>(track->getClips()[0]))
            return *clip;

    return {};
}

void MainComponent::updateCrossfader()
{
    const float position = crossfaderSlider.getValue();
    // Convert linear position (0.0 to 1.0) to decibels (-20dB to 0dB)
    // Track 1 volume goes from 0dB to -20dB
    setTrackVolume(0, position <= 0.0f ? 0.0f : -20.0f * position);
    // Track 2 volume goes from -20dB to 0dB
    setTrackVolume(1, position >= 1.0f ? 0.0f : -20.0f * (1.0f - position));
}

void MainComponent::setTrackVolume(int trackIndex, float gainDB)
{
    if (auto clip = getClip(trackIndex))
    {
        clip->setGainDB(gainDB);
    }
}


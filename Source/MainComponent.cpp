#include "MainComponent.h"
#include "Utilities.h"


//==============================================================================
MainComponent::MainComponent()
{
    addAndMakeVisible(openButton);
    addAndMakeVisible(saveButton);
    addAndMakeVisible(playButton);
    addAndMakeVisible(stopButton);
    addAndMakeVisible(tempoSlider);
    setSize (600, 400);
    
    playButton.setToggleState(false, juce::NotificationType::dontSendNotification);
    stopButton.setToggleState(true, juce::NotificationType::dontSendNotification);

    playButton.onClick = [this] { play(); };
    stopButton.onClick = [this] { stop(); };
    openButton.onClick = [this] { loadAudioFile(); };

    tempoSlider.setRange(30.0, 220.0, 0.1);
    tempoSlider.onDragEnd = [this] { updateTempo(); };
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
    g.drawText ("Hello World!", getLocalBounds(), juce::Justification::centred, true);
}

void MainComponent::resized()
{
    // This is called when the MainComponent is resized.
    // If you add any child components, this is where you should
    // update their positions.
    openButton.setBounds(10, 10, 100, 20);
    saveButton.setBounds(10, 40, 100, 20);
    playButton.setBounds(10, 70, 100, 20);
    stopButton.setBounds(10, 100, 100, 20);
    tempoSlider.setBounds(10, 130, 100, 20);
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
                { { tracktion::TimePosition::fromSeconds(1.0), // Start at 1 second
                    tracktion::TimeDuration::fromSeconds(clip1->getSourceLength().inSeconds()) }, {} }, 
                false))
            {
                // Configure second clip
                clip2->setAutoTempo(false);
                clip2->setAutoPitch(false);
                clip2->setTimeStretchMode(te::TimeStretcher::defaultMode);
            }
        }

        // Set initial tempo
        tempoSlider.setValue(120.0, juce::dontSendNotification);
        updateTempo();

        play();
    }
}

void MainComponent::updateTempo()
{
    const double baseTempo = 120.0; // Assume a base tempo
    const double ratio = tempoSlider.getValue() / baseTempo;

    // Update first clip
    if (auto clip1 = getClip(0))
    {
        clip1->setSpeedRatio(ratio);
        clip1->setLength(tracktion::TimeDuration::fromSeconds(120) / clip1->getSpeedRatio(), true);
    }

    // Update second clip
    if (auto clip2 = getClip(1))
    {
        clip2->setSpeedRatio(ratio);
        clip2->setLength(tracktion::TimeDuration::fromSeconds(120) / clip2->getSpeedRatio(), true);
    }
}

te::WaveAudioClip::Ptr MainComponent::getClip(int trackIndex)
{
    if (auto track = EngineHelpers::getOrInsertAudioTrackAt(edit, trackIndex))
        if (auto clip = dynamic_cast<te::WaveAudioClip*>(track->getClips()[0]))
            return *clip;

    return {};
}


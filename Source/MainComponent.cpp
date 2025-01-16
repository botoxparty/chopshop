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

    if (auto clip = EngineHelpers::loadAudioFileAsClip(edit, file))
    {
        // Disable auto tempo and pitch, we'll handle these manually
        clip->setAutoTempo(false);
        clip->setAutoPitch(false);
        clip->setTimeStretchMode(te::TimeStretcher::defaultMode);

        // Set initial tempo
        tempoSlider.setValue(120.0, juce::dontSendNotification);
        updateTempo();

        play();
    }
}

void MainComponent::updateTempo()
{
    if (auto clip = getClip())
    {
        const double baseTempo = 120.0; // Assume a base tempo
        const double ratio = tempoSlider.getValue() / baseTempo;
        clip->setSpeedRatio(ratio);
        clip->setLength(tracktion::TimeDuration::fromSeconds(120) / clip->getSpeedRatio(), true);
    }
}

te::WaveAudioClip::Ptr MainComponent::getClip()
{
    if (auto track = EngineHelpers::getOrInsertAudioTrackAt(edit, 0))
        if (auto clip = dynamic_cast<te::WaveAudioClip*>(track->getClips()[0]))
            return *clip;

    return {};
}


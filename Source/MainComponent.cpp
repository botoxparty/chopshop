#include "MainComponent.h"
#include "Utilities.h"


//==============================================================================
MainComponent::MainComponent()
{
    addAndMakeVisible(openButton);
    addAndMakeVisible(saveButton);
    addAndMakeVisible(playButton);
    addAndMakeVisible(stopButton);
    setSize (600, 400);
    
    playButton.setToggleState(false, juce::NotificationType::dontSendNotification);
    stopButton.setToggleState(true, juce::NotificationType::dontSendNotification);

    playButton.onClick = [this] { play(); };
    stopButton.onClick = [this] { stop(); };
    openButton.onClick = [this] { loadAudioFile(); };
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
        edit.getTransport().play(false);
    }
}


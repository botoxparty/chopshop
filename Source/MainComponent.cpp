#include "MainComponent.h"
#include "ChopComponent.h"
#include <algorithm>

#define JUCE_USE_DIRECTWRITE 0 // Fix drawing of Monospace fonts in Code Editor!

//==============================================================================
MainComponent::MainComponent()
{
    // Create a global command manager
    commandManager = std::make_unique<juce::ApplicationCommandManager>();
    
    // Add this near the start of the constructor, before other component setup
    controlBarComponent = std::make_unique<ControlBarComponent>(edit);
    addAndMakeVisible(*controlBarComponent);

    // Set up callbacks for the control bar
    controlBarComponent->onPlayButtonClicked = [this] { play(); };
    controlBarComponent->onStopButtonClicked = [this] { stop(); };

    // Register our custom plugins with the engine
    engine.getPluginManager().createBuiltInType<tracktion_engine::OscilloscopePlugin>();
    engine.getPluginManager().createBuiltInType<FlangerPlugin>();
    engine.getPluginManager().createBuiltInType<AutoDelayPlugin>();
    engine.getPluginManager().createBuiltInType<AutoPhaserPlugin>();

    addAndMakeVisible(saveButton);
    addAndMakeVisible(recordButton);
    addAndMakeVisible(audioSettingsButton);

    customLookAndFeel = std::make_unique<CustomLookAndFeel>();
    // // setLookAndFeel(customLookAndFeel.get());
    LookAndFeel::setDefaultLookAndFeel(customLookAndFeel.get());

    // Create and set up thumbnail
    thumbnail = std::make_unique<Thumbnail>(edit.getTransport());
    addAndMakeVisible(*thumbnail);
    thumbnail->start();

    // Set initial position
    edit.getTransport().setPosition(tracktion::TimePosition::fromSeconds(0.0));

    setSize(1024, 900);

    // Setup reverb control
    reverbComponent = std::make_unique<ReverbComponent>(edit);
    addAndMakeVisible(*reverbComponent);

    // setup effects
    recordButton.setToggleState(false, juce::NotificationType::dontSendNotification);
    recordButton.onClick = [this]
    {
        if (edit.getTransport().isRecording())
            stopRecording();
        else
            startRecording();
    };

    // Create ChopComponent and pass the command manager
    chopComponent = std::make_unique<ChopComponent>(edit);
    addAndMakeVisible(*chopComponent);
    
    // Set the command manager for the ChopComponent
    chopComponent->setCommandManager(commandManager.get());

    // Register all commands with the command manager
    commandManager->registerAllCommandsForTarget(this);
    
    // Add key mappings to the top level component
    addKeyListener(commandManager->getKeyMappings());

    // Restore the mouse handlers
    chopComponent->onChopButtonPressed = [this]()
    {
        chopStartTime = juce::Time::getMillisecondCounterHiRes();
        float currentPosition = chopComponent->getCrossfaderValue();
        chopComponent->setCrossfaderValue(currentPosition <= 0.5f ? 1.0f : 0.0f);
    };

    chopComponent->onChopButtonReleased = [this]()
    {
        double elapsedTime = juce::Time::getMillisecondCounterHiRes() - chopStartTime;
        double minimumTime = chopComponent->getChopDurationInMs(screwComponent->getTempo());

        if (elapsedTime >= minimumTime)
        {
            float currentPosition = chopComponent->getCrossfaderValue();
            chopComponent->setCrossfaderValue(currentPosition <= 0.5f ? 1.0f : 0.0f);
        }
        else
        {
            chopReleaseDelay = minimumTime - elapsedTime;
            startTimer(static_cast<int>(chopReleaseDelay));
        }
    };

    getLookAndFeel().setDefaultSansSerifTypefaceName("Arial");

    // Add the button callback
    audioSettingsButton.onClick = [this]
    {
        EngineHelpers::showAudioDeviceSettings(engine);
    };

    gamepadManager = std::make_unique<GamepadManager>();
    gamepadManager->addListener(this);

    // Add after reverbComponent initialization
    flangerComponent = std::make_unique<FlangerComponent>(edit);
    addAndMakeVisible(*flangerComponent);

    delayComponent = std::make_unique<DelayComponent>(edit);
    addAndMakeVisible(*delayComponent);

    phaserComponent = std::make_unique<PhaserComponent>(edit);
    addAndMakeVisible(*phaserComponent);

    updateButtonStates();

    libraryComponent = std::make_unique<LibraryComponent>();
    addAndMakeVisible(*libraryComponent);

    // Set up the callback for when a file is selected
    libraryComponent->onFileSelected = [this](const juce::File &file)
    {
        handleFileSelection(file);
    };

    // Initialize two tracks
    if (auto track1 = EngineHelpers::getOrInsertAudioTrackAt(edit, 0))
    {
        EngineHelpers::removeAllClips(*track1);
        volumeAndPan1 = dynamic_cast<te::VolumeAndPanPlugin *>(track1->pluginList.insertPlugin(te::VolumeAndPanPlugin::create(), 0).get());

        // Add oscilloscope plugin to track 1
        track1->pluginList.insertPlugin(te::OscilloscopePlugin::create(), -1);
    }

    if (auto track2 = EngineHelpers::getOrInsertAudioTrackAt(edit, 1))
    {
        EngineHelpers::removeAllClips(*track2);
        volumeAndPan2 = dynamic_cast<te::VolumeAndPanPlugin *>(track2->pluginList.insertPlugin(te::VolumeAndPanPlugin::create(), 0).get());
    }

    createVinylBrakeComponent();

    startTimerHz(30); // Update 30 times per second

    // Add oscilloscope to master track
    if (auto masterTrack = edit.getMasterTrack())
    {
        DBG("Found master track");

        // Register our custom plugins with the engine
        engine.getPluginManager().createBuiltInType<tracktion_engine::OscilloscopePlugin>();

        oscilloscopePlugin = masterTrack->pluginList.insertPlugin(tracktion_engine::OscilloscopePlugin::create(), -1);
        if (oscilloscopePlugin != nullptr)
        {
            DBG("Created oscilloscope plugin");
            if (auto *oscPlugin = dynamic_cast<tracktion_engine::OscilloscopePlugin *>(oscilloscopePlugin.get()))
            {
                DBG("Cast to oscilloscope plugin successful");
                oscPlugin->addListener(this);
            }
        }
    }

    // Add oscilloscope to visualizer box
    if (oscilloscopeComponent != nullptr)
        DBG("Oscilloscope component added to visualizer box");
    else
        DBG("Oscilloscope component not added to visualizer box");

    // Add after other component setup
    chopComponent->onCrossfaderValueChanged = [this](float value)
    {
        updateCrossfader();
    };

    screwComponent = std::make_unique<ScrewComponent>(edit);
    addAndMakeVisible(*screwComponent);

    // Initialize the screw component with the current tempo
    screwComponent->setTempo(baseTempo, juce::dontSendNotification);

    screwComponent->onTempoChanged = [this](double tempo)
    {
        updateTempo();
        if (delayComponent)
            delayComponent->setTempo(tempo);
    };

    // Create plugin rack after all effects are initialized
    createPluginRack();

    controllerMappingComponent = std::make_unique<ControllerMappingComponent>();
    addAndMakeVisible(*controllerMappingComponent);
    resized();
}

MainComponent::~MainComponent()
{
    // Remove key listener before destroying command manager
    removeKeyListener(commandManager->getKeyMappings());
    
    // Call releaseResources first to ensure proper cleanup
    releaseResources();
    
    // Additional cleanup if needed
    LookAndFeel::setDefaultLookAndFeel(nullptr);
    customLookAndFeel = nullptr;
    
    // Clear command manager last
    commandManager = nullptr;
}

//==============================================================================
void MainComponent::paint(juce::Graphics &g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));

    g.setFont(juce::FontOptions(16.0f));
    g.setColour(juce::Colours::white);
    // g.drawText ("Hello World!", getLocalBounds(), juce::Justification::centred, true);
}

void MainComponent::resized()
{
    auto bounds = getLocalBounds();
    bounds.reduce(10, 10); // Add some padding

    // Create main column FlexBox
    juce::FlexBox mainColumn;
    mainColumn.flexDirection = juce::FlexBox::Direction::column;
    mainColumn.justifyContent = juce::FlexBox::JustifyContent::spaceBetween;

    // Row 1: Thumbnail and Oscilloscope (about 1/3 of height)
    juce::FlexBox visualizerBox;
    visualizerBox.flexDirection = juce::FlexBox::Direction::column;
    if (oscilloscopeComponent != nullptr)
        visualizerBox.items.add(juce::FlexItem(*oscilloscopeComponent).withFlex(0.7f).withMargin(5));

    visualizerBox.items.add(juce::FlexItem(*thumbnail).withFlex(0.3f).withMargin(5));
    mainColumn.items.add(juce::FlexItem(visualizerBox).withFlex(1.0f));

    // Row 2: Control Bar - now just add the component directly
    mainColumn.items.add(juce::FlexItem(*controlBarComponent).withHeight(50).withMargin(5));

    // Row 3: Main Box (remaining space)
    juce::FlexBox mainBox;
    mainBox.flexDirection = juce::FlexBox::Direction::row;
    mainBox.flexWrap = juce::FlexBox::Wrap::noWrap;
    mainBox.justifyContent = juce::FlexBox::JustifyContent::spaceAround;

    // Column 1 (Transport controls)
    juce::FlexBox column1;
    column1.flexDirection = juce::FlexBox::Direction::column;
    column1.items.add(juce::FlexItem(*libraryComponent).withFlex(1.0f).withHeight(300).withMargin(5));
    column1.items.add(juce::FlexItem(audioSettingsButton).withHeight(30).withMargin(5));
    column1.items.add(juce::FlexItem(*controllerMappingComponent).withHeight(30).withMargin(5));

    // Column 2 (Tempo and crossfader)
    juce::FlexBox column2;
    column2.flexDirection = juce::FlexBox::Direction::column;
    column2.items.add(juce::FlexItem(*screwComponent).withFlex(0.25f).withMinHeight(100).withMargin(5));
    column2.items.add(juce::FlexItem(*chopComponent).withFlex(0.5f).withMinHeight(200).withMargin(5));
    column2.items.add(juce::FlexItem(*vinylBrakeComponent).withFlex(0.25f).withMinHeight(100).withMargin(5));

    // Column 3 (Effects)
    juce::FlexBox column3;
    column3.flexDirection = juce::FlexBox::Direction::column;
    column3.items.add(juce::FlexItem(*reverbComponent).withFlex(1.0f).withMinHeight(120).withMargin(5));
    column3.items.add(juce::FlexItem(*delayComponent).withFlex(1.0f).withMinHeight(120).withMargin(5));
    column3.items.add(juce::FlexItem(*flangerComponent).withFlex(1.0f).withMinHeight(120).withMargin(5));
    column3.items.add(juce::FlexItem(*phaserComponent).withFlex(1.0f).withMinHeight(120).withMargin(5));

    // Add columns to main box
    mainBox.items.add(juce::FlexItem(column1).withFlex(1.0f));
    mainBox.items.add(juce::FlexItem(column2).withFlex(1.0f));
    mainBox.items.add(juce::FlexItem(column3).withFlex(1.0f));

    // Add main box to main column
    mainColumn.items.add(juce::FlexItem(mainBox).withFlex(2.0f).withMargin(5));

    // Perform the layout
    mainColumn.performLayout(bounds);
}

void MainComponent::play()
{
    EngineHelpers::togglePlay(edit);

    // Update button states based on transport state
    const bool isPlaying = edit.getTransport().isPlaying();
    controlBarComponent->setPlayButtonState(isPlaying);
    controlBarComponent->setStopButtonState(!isPlaying);
    playState = isPlaying ? PlayState::Playing : PlayState::Stopped;
}

void MainComponent::stop()
{
    EngineHelpers::togglePlay(edit, EngineHelpers::ReturnToStart::yes);

    // Stop transport and reset position
    edit.getTransport().stop(true, false);
    edit.getTransport().setPosition(tracktion::TimePosition::fromSeconds(0.0));

    playState = PlayState::Stopped;
    controlBarComponent->setPlayButtonState(false);
    controlBarComponent->setStopButtonState(true);

    updateButtonStates();
}

void MainComponent::loadAudioFile()
{
    EngineHelpers::browseForAudioFile(engine, [this](const juce::File &file)
                                      { handleFileSelection(file); });
}

void MainComponent::handleFileSelection(const juce::File &file)
{
    if (!file.existsAsFile())
        return;

    // Load clip into first track
    auto clip1 = EngineHelpers::loadAudioFileAsClip(edit, file);
    if (clip1)
    {
        // Stop playback and reset transport
        edit.getTransport().stop(false, false);
        edit.getTransport().setPosition(tracktion::TimePosition::fromSeconds(0.0));

        // Reset play/pause/stop button states
        playState = PlayState::Stopped;
        controlBarComponent->setPlayButtonState(false);
        controlBarComponent->setStopButtonState(true);
        controlBarComponent->setTrackName(file.getFileNameWithoutExtension());

        // Load audio file and get samples
        juce::AudioFormatManager formatManager;
        formatManager.registerBasicFormats();

        std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file));
        if (reader)
        {
            // Store current tempo ratio before updating base tempo
            const double currentRatio = screwComponent->getTempo() / baseTempo;

            float detectedBPM = libraryComponent->getBPMForFile(file);
            baseTempo = detectedBPM;
            trackOffset = (60.0 / baseTempo) * 1000.0;
            screwComponent->setBaseTempo(baseTempo);

            // Apply the previous tempo ratio to the new base tempo
            const double newTempo = baseTempo * currentRatio;
            screwComponent->setTempo(newTempo, juce::sendNotification);

            // Calculate and set delay time to 1/4 note
            if (delayComponent)
            {
                // Calculate quarter note duration in milliseconds
                double quarterNoteMs = (60.0 / baseTempo) * 1000.0;
                delayComponent->setDelayTime(quarterNoteMs);
            }
        }

        // Disable auto tempo and pitch for first clip
        // clip1->setAutoTempo(false);
        clip1->setSyncType(te::Clip::syncBarsBeats);
        clip1->setAutoPitch(false);
        clip1->setTimeStretchMode(te::TimeStretcher::defaultMode);
        clip1->setUsesProxy(false);

        auto loopedClip = EngineHelpers::loopAroundClip(*clip1);
        edit.getTransport().stop(false, false); // Stop playback after loop setup
        thumbnail->setFile(loopedClip->getPlaybackFile());

        // Create second track and load the same file
        if (auto track2 = EngineHelpers::getOrInsertAudioTrackAt(edit, 1))
        {
            EngineHelpers::removeAllClips(*track2);

            // Add clip to second track with offset
            if (auto clip2 = track2->insertWaveClip(file.getFileNameWithoutExtension(), file,
                                                    {{tracktion::TimePosition::fromSeconds(-trackOffset / 1000.0), // Convert ms to seconds
                                                      tracktion::TimeDuration::fromSeconds(clip1->getSourceLength().inSeconds())},
                                                     {}},
                                                    false))
            {
                // Configure second clip
                // clip2->setAutoTempo(false);
                clip2->setSyncType(te::Clip::syncBarsBeats);
                clip2->setAutoPitch(false);
                clip2->setTimeStretchMode(te::TimeStretcher::defaultMode);
                clip2->setGainDB(0.0f);
                clip2->setUsesProxy(false);
            }
        }

        // Reset crossfader to first track
        chopComponent->setCrossfaderValue(0.0);
        updateCrossfader();
    }

    updateButtonStates();

    // Auto-play the newly loaded track
    if (playState != PlayState::Playing)
    {
        edit.getTransport().setPosition(tracktion::TimePosition::fromSeconds(0.0));
        play();
    }
}

void MainComponent::updateTempo()
{
    const double ratio = screwComponent->getTempo() / baseTempo;
    // Convert ratio to plus/minus proportion (e.g., 1.5 becomes 0.5, 0.5 becomes -0.5)
    const double plusOrMinusProportion = ratio - 1.0;

    edit.getTransport().getCurrentPlaybackContext()->setSpeedCompensation(plusOrMinusProportion * 100.0);
}

te::WaveAudioClip::Ptr MainComponent::getClip(int trackIndex)
{
    if (auto track = EngineHelpers::getOrInsertAudioTrackAt(edit, trackIndex))
        if (auto clip = dynamic_cast<te::WaveAudioClip *>(track->getClips()[0]))
            return *clip;

    return {};
}

void MainComponent::updateCrossfader()
{
    const float position = chopComponent->getCrossfaderValue();
    const float minDB = -60.0f; // Effectively silent

    // Calculate volume curves that give equal power at center position
    float gainTrack1 = std::cos(position * juce::MathConstants<float>::halfPi);
    float gainTrack2 = std::sin(position * juce::MathConstants<float>::halfPi);

    // Convert linear gains to dB
    float gainDB1 = gainTrack1 <= 0.0f ? minDB : juce::Decibels::gainToDecibels(gainTrack1);
    float gainDB2 = gainTrack2 <= 0.0f ? minDB : juce::Decibels::gainToDecibels(gainTrack2);

    // Apply volumes to tracks
    setTrackVolume(0, gainDB1);
    setTrackVolume(1, gainDB2);
}

void MainComponent::setTrackVolume(int trackIndex, float gainDB)
{
    if (trackIndex == 0 && volumeAndPan1)
        volumeAndPan1->setVolumeDb(gainDB);
    else if (trackIndex == 1 && volumeAndPan2)
        volumeAndPan2->setVolumeDb(gainDB);
}

void MainComponent::armTrack(int trackIndex, bool arm)
{
    if (auto track = EngineHelpers::getOrInsertAudioTrackAt(edit, trackIndex))
    {
        EngineHelpers::armTrack(*track, arm);
    }
}

void MainComponent::startRecording()
{
    // Arm the first track for recording
    armTrack(0, true);

    // Start transport recording
    edit.getTransport().record(false);

    recordButton.setToggleState(true, juce::NotificationType::dontSendNotification);
}

void MainComponent::stopRecording()
{
    // Stop recording
    edit.getTransport().stop(false, false);

    // Disarm track
    armTrack(0, false);

    recordButton.setToggleState(false, juce::NotificationType::dontSendNotification);
}

bool MainComponent::isTempoPercentageActive(double percentage) const
{
    // Compare current tempo with base tempo * percentage
    const double currentPercentage = screwComponent->getTempo() / baseTempo;
    // Allow for small floating point differences
    return std::abs(currentPercentage - percentage) < 0.001;
}

void MainComponent::gamepadButtonPressed(int buttonId)
{
    float currentPosition;
    switch (buttonId)
    {
    case SDL_GAMEPAD_BUTTON_SOUTH:
        chopStartTime = juce::Time::getMillisecondCounterHiRes();
        currentPosition = chopComponent->getCrossfaderValue();
        chopComponent->setCrossfaderValue(currentPosition <= 0.5f ? 1.0f : 0.0f);
        break;
    case SDL_GAMEPAD_BUTTON_DPAD_UP:
    {
        if (reverbComponent)
            reverbComponent->rampMixLevel(true);
        break;
    }
    case SDL_GAMEPAD_BUTTON_DPAD_RIGHT:
    {
        if (delayComponent)
            delayComponent->rampMixLevel(true);
        break;
    }
    case SDL_GAMEPAD_BUTTON_DPAD_DOWN:
    {
        if (flangerComponent)
            flangerComponent->rampMixLevel(true);
        break;
    }
    }
}

void MainComponent::gamepadButtonReleased(int buttonId)
{
    switch (buttonId)
    {
    case SDL_GAMEPAD_BUTTON_SOUTH: // Cross
    {
        double elapsedTime = juce::Time::getMillisecondCounterHiRes() - chopStartTime;
        double minimumTime = trackOffset;

        if (elapsedTime >= minimumTime)
        {
            float currentPosition = chopComponent->getCrossfaderValue();
            chopComponent->setCrossfaderValue(currentPosition <= 0.5f ? 1.0f : 0.0f);
        }
        else
        {
            chopReleaseDelay = minimumTime - elapsedTime;
            startTimer(static_cast<int>(chopReleaseDelay));
        }
        break;
    }
    case SDL_GAMEPAD_BUTTON_DPAD_UP:
    {
        if (reverbComponent)
            reverbComponent->rampMixLevel(false);
        break;
    }
    case SDL_GAMEPAD_BUTTON_DPAD_RIGHT:
    {
        if (delayComponent)
            delayComponent->rampMixLevel(false);
        break;
    }
    case SDL_GAMEPAD_BUTTON_DPAD_DOWN:
    {
        if (flangerComponent)
            flangerComponent->rampMixLevel(false);
        break;
    }
    }
}

void MainComponent::gamepadAxisMoved(int axisId, float value)
{
    static float rightX = 0.0f;
    static float rightY = 0.0f;
    static float leftX = 0.0f;
    static float leftY = 0.0f;

    switch (axisId)
    {
    case SDL_GAMEPAD_AXIS_RIGHT_TRIGGER:
        if (vinylBrakeComponent)
        {
            if (value < 0.01f && vinylBrakeComponent->getBrakeValue() > 0.0f)
                vinylBrakeComponent->startSpringAnimation();
            else
                vinylBrakeComponent->setBrakeValue(value);
        }
        break;

    case SDL_GAMEPAD_AXIS_LEFTX:
        leftX = value;
        if (flangerComponent)
        {
            flangerComponent->setSpeed(value * 10.0f);
            // Calculate width based on stick distance from center
            float distance = std::sqrt(leftX * leftX + leftY * leftY);
            float normalizedDistance = distance / std::sqrt(2.0f);
            float curvedWidth = normalizedDistance * normalizedDistance;
            flangerComponent->setWidth(juce::jlimit(0.0f, 0.99f, curvedWidth));
        }
        break;

    case SDL_GAMEPAD_AXIS_LEFTY:
        leftY = value;
        if (flangerComponent)
        {
            flangerComponent->setDepth(value * 10.0f);
            // Calculate width based on stick distance from center
            float distance = std::sqrt(leftX * leftX + leftY * leftY);
            float normalizedDistance = distance / std::sqrt(2.0f);
            float curvedWidth = normalizedDistance * normalizedDistance;
            flangerComponent->setWidth(juce::jlimit(0.0f, 0.99f, curvedWidth));
        }
        break;

    case SDL_GAMEPAD_AXIS_RIGHTX:
        rightX = value;
        if (phaserComponent)
        {
            phaserComponent->setRate(value * 10.0f);
            float distance = std::sqrt(rightX * rightX + rightY * rightY);
            phaserComponent->setFeedback(juce::jlimit(0.0f, 0.70f, distance));
        }
        break;

    case SDL_GAMEPAD_AXIS_RIGHTY:
        rightY = value;
        if (phaserComponent)
        {
            phaserComponent->setDepth(value * 10.0f);
            float distance = std::sqrt(rightX * rightX + rightY * rightY);
            phaserComponent->setFeedback(juce::jlimit(0.0f, 0.70f, distance));
        }
        break;
    }
}

void MainComponent::updatePositionLabel()
{
    if (controlBarComponent)
        controlBarComponent->updatePositionLabel();
}

void MainComponent::createVinylBrakeComponent()
{
    vinylBrakeComponent = std::make_unique<VinylBrakeComponent>(edit);

    // Set up the callback to get current tempo adjustment
    vinylBrakeComponent->getCurrentTempoAdjustment = [this]()
    {
        // Get the current tempo ratio from the screw component
        double ratio = screwComponent->getTempo() / baseTempo;
        // Convert to plus/minus proportion (same as updateTempo())
        return ratio - 1.0;
    };

    addAndMakeVisible(*vinylBrakeComponent);
}

void MainComponent::createPluginRack()
{
    if (auto masterTrack = edit.getMasterTrack())
    {
        tracktion_engine::Plugin::Array plugins;

        if (reverbComponent)
            plugins.add(reverbComponent->getPlugin());
        if (delayComponent)
            plugins.add(delayComponent->getPlugin());
        if (flangerComponent)
            plugins.add(flangerComponent->getPlugin());
        if (phaserComponent)
            plugins.add(phaserComponent->getPlugin());

        // Create the rack type with proper channel connections
        if (auto rack = tracktion_engine::RackType::createTypeToWrapPlugins(plugins, edit))
        {
            masterTrack->pluginList.insertPlugin(tracktion_engine::RackInstance::create(*rack), 0);
        }
    }
}

void MainComponent::releaseResources()
{
    // Stop any active timers
    stopTimer();
    
    // Stop playback if active
    if (edit.getTransport().isPlaying())
        edit.getTransport().stop(true, false);
    
    // Debug reference counting
    if (oscilloscopePlugin != nullptr)
    {
        DBG("Oscilloscope plugin reference count: " + 
            juce::String(oscilloscopePlugin->getReferenceCount()));
    }
    
    // Remove oscilloscope listener
    if (auto* oscPlugin = dynamic_cast<tracktion_engine::OscilloscopePlugin*>(oscilloscopePlugin.get()))
    {
        oscPlugin->removeListener(this);
        DBG("Removed oscilloscope listener");
    }
    
    // Clear all component pointers in a specific order
    oscilloscopeComponent = nullptr;
    thumbnail = nullptr;
    
    controllerMappingComponent = nullptr;
    libraryComponent = nullptr;
    
    phaserComponent = nullptr;
    delayComponent = nullptr;
    flangerComponent = nullptr;
    screwComponent = nullptr;
    chopComponent = nullptr;
    reverbComponent = nullptr;
    vinylBrakeComponent = nullptr;
    
    // Release plugin reference
    oscilloscopePlugin = nullptr;
    
    // Clear gamepad manager
    if (gamepadManager)
        gamepadManager->removeListener(this);
    gamepadManager = nullptr;

    controlBarComponent = nullptr;
}


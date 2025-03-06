#include "MainComponent.h"
#include "ChopComponent.h"
#include <algorithm>

#define JUCE_USE_DIRECTWRITE 0 // Fix drawing of Monospace fonts in Code Editor!

//==============================================================================
MainComponent::MainComponent()
{
    // Create a global command manager
    commandManager = std::make_unique<juce::ApplicationCommandManager>();

    customLookAndFeel = std::make_unique<CustomLookAndFeel>();
    LookAndFeel::setDefaultLookAndFeel (customLookAndFeel.get());
    getLookAndFeel().setDefaultSansSerifTypefaceName ("Arial");
    setSize (1024, 900);
    startTimerHz (30);

    // Register our custom plugins with the engine
    engine.getPluginManager().createBuiltInType<tracktion::engine::OscilloscopePlugin>();
    engine.getPluginManager().createBuiltInType<FlangerPlugin>();
    engine.getPluginManager().createBuiltInType<AutoDelayPlugin>();
    engine.getPluginManager().createBuiltInType<AutoPhaserPlugin>();
    engine.getPluginManager().createBuiltInType<ScratchPlugin>();

    addAndMakeVisible (audioSettingsButton);

    // Add the button callback
    audioSettingsButton.onClick = [this] {
        EngineHelpers::showAudioDeviceSettings (engine);
    };

    gamepadManager = GamepadManager::getInstance();
    gamepadManager->addListener (this);

    // Initialize library component first as it's not edit-dependent
    setupLibraryComponent();

    // Initialize controller mapping component (not edit-dependent)
    controllerMappingComponent = std::make_unique<ControllerMappingComponent>();
    addAndMakeVisible (*controllerMappingComponent);

    resized();
}

MainComponent::~MainComponent()
{
    // Remove key listener before destroying command manager
    removeKeyListener (commandManager->getKeyMappings());

    // Call releaseResources first to ensure proper cleanup
    releaseResources();

    // Additional cleanup if needed
    LookAndFeel::setDefaultLookAndFeel (nullptr);
    customLookAndFeel = nullptr;

    // Clear command manager last
    commandManager = nullptr;
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
    bounds.reduce (10, 10); // Add some padding

    // Create main column FlexBox
    juce::FlexBox mainColumn;
    mainColumn.flexDirection = juce::FlexBox::Direction::column;
    mainColumn.justifyContent = juce::FlexBox::JustifyContent::spaceBetween;

    // Row 1: Transport control (about 1/4 of height)
    if (transportComponent != nullptr)
        mainColumn.items.add (juce::FlexItem (*transportComponent).withFlex (1.0f).withMargin (5));

    // Row 2: Thumbnail and Oscilloscope (about 1/3 of height)
    juce::FlexBox visualizerBox;
    visualizerBox.flexDirection = juce::FlexBox::Direction::column;
    if (oscilloscopeComponent != nullptr)
        visualizerBox.items.add (juce::FlexItem (*oscilloscopeComponent).withFlex (0.6f).withMargin (5));

    // Give the thumbnail more space for visualization
    // mainColumn.items.add (juce::FlexItem (visualizerBox).withFlex (1.0f));

    // Row 3: Main Box (remaining space)
    juce::FlexBox mainBox;
    mainBox.flexDirection = juce::FlexBox::Direction::row;
    mainBox.flexWrap = juce::FlexBox::Wrap::noWrap;
    mainBox.justifyContent = juce::FlexBox::JustifyContent::spaceAround;

    // Column 1 (Transport controls)
    juce::FlexBox column1;
    column1.flexDirection = juce::FlexBox::Direction::column;
    column1.items.add (juce::FlexItem (*libraryComponent).withFlex (1.0f).withHeight (300).withMargin (5));
    column1.items.add (juce::FlexItem (audioSettingsButton).withHeight (30).withMargin (5));
    column1.items.add (juce::FlexItem (*controllerMappingComponent).withHeight (30).withMargin (5));

    // Column 2 (Tempo and crossfader)
    juce::FlexBox column2;
    column2.flexDirection = juce::FlexBox::Direction::column;
    column2.items.add (juce::FlexItem (*screwComponent).withFlex (0.25f).withMargin (5));
    column2.items.add (juce::FlexItem (*chopComponent).withFlex (0.5f).withMargin (5));
    column2.items.add (juce::FlexItem (*scratchComponent).withFlex (0.25f).withMargin (5));
    column2.items.add (juce::FlexItem (*vinylBrakeComponent).withFlex (0.25f).withMargin (5));

    // Column 3 (Effects)
    juce::FlexBox column3;
    column3.flexDirection = juce::FlexBox::Direction::column;
    column3.items.add (juce::FlexItem (*reverbComponent).withFlex (1.0f).withMargin (5));
    column3.items.add (juce::FlexItem (*delayComponent).withFlex (1.0f).withMargin (5));
    column3.items.add (juce::FlexItem (*flangerComponent).withFlex (1.0f).withMargin (5));
    column3.items.add (juce::FlexItem (*phaserComponent).withFlex (1.0f).withMargin (5));

    // Add columns to main box
    mainBox.items.add (juce::FlexItem (column1).withFlex (1.0f));
    mainBox.items.add (juce::FlexItem (column2).withFlex (1.0f));
    mainBox.items.add (juce::FlexItem (column3).withFlex (1.0f));

    // Add main box to main column
    mainColumn.items.add (juce::FlexItem (mainBox).withFlex (2.0f).withMargin (5));

    // Perform the layout
    mainColumn.performLayout (bounds);
}

void MainComponent::play()
{
    EngineHelpers::togglePlay (*edit);

    // Update button states based on transport state
    const bool isPlaying = edit->getTransport().isPlaying();
    playState = isPlaying ? PlayState::Playing : PlayState::Stopped;
}

void MainComponent::stop()
{
    EngineHelpers::togglePlay (*edit, EngineHelpers::ReturnToStart::yes);

    // Stop transport and reset position
    edit->getTransport().stop (true, false);
    edit->getTransport().setPosition (tracktion::TimePosition::fromSeconds (0.0));

    playState = PlayState::Stopped;
}

void MainComponent::updateTempo()
{
    // Calculate the new BPM based on the current tempo from the screw component
    double newBpm = screwComponent->getTempo();

    // Insert tempo change at the beginning of the track
    auto tempoSetting = edit->tempoSequence.insertTempo (tracktion::TimePosition::fromSeconds (0.0));
    if (tempoSetting != nullptr)
        tempoSetting->setBpm (newBpm);

    // Calculate ratio for thumbnail display
    const double ratio = baseTempo / newBpm;

    // Update the delay component if it exists
    if (delayComponent)
    {
        delayComponent->setTempo (newBpm);
    }
}

te::WaveAudioClip::Ptr MainComponent::getClip (int trackIndex)
{
    if (auto track = EngineHelpers::getOrInsertAudioTrackAt (*edit, trackIndex))
        if (auto clip = dynamic_cast<te::WaveAudioClip*> (track->getClips()[0]))
            return *clip;

    return {};
}

void MainComponent::updateCrossfader()
{
    const float position = chopComponent->getCrossfaderValue();
    const float minDB = -60.0f; // Effectively silent

    // Calculate volume curves that give equal power at center position
    float gainTrack1 = std::cos (position * juce::MathConstants<float>::halfPi);
    float gainTrack2 = std::sin (position * juce::MathConstants<float>::halfPi);

    // Convert linear gains to dB
    float gainDB1 = gainTrack1 <= 0.0f ? minDB : juce::Decibels::gainToDecibels (gainTrack1);
    float gainDB2 = gainTrack2 <= 0.0f ? minDB : juce::Decibels::gainToDecibels (gainTrack2);

    // Apply volumes to tracks
    setTrackVolume (0, gainDB1);
    setTrackVolume (1, gainDB2);
}

void MainComponent::setupChopComponent()
{
    // Create ChopComponent and pass the command manager
    chopComponent = std::make_unique<ChopComponent> (*edit);
    addAndMakeVisible (*chopComponent);

    // Set the command manager for the ChopComponent
    chopComponent->setCommandManager (commandManager.get());

    // Register all commands with the command manager
    commandManager->registerAllCommandsForTarget (this);

    // Add key mappings to the top level component
    addKeyListener (commandManager->getKeyMappings());

    // Restore the mouse handlers
    chopComponent->onChopButtonPressed = [this]() {
        chopStartTime = juce::Time::getMillisecondCounterHiRes();
        float currentPosition = chopComponent->getCrossfaderValue();
        chopComponent->setCrossfaderValue (currentPosition <= 0.5f ? 1.0f : 0.0f);
    };

    chopComponent->onChopButtonReleased = [this]() {
        double elapsedTime = juce::Time::getMillisecondCounterHiRes() - chopStartTime;
        double minimumTime = chopComponent->getChopDurationInMs (screwComponent->getTempo());

        if (elapsedTime >= minimumTime)
        {
            float currentPosition = chopComponent->getCrossfaderValue();
            chopComponent->setCrossfaderValue (currentPosition <= 0.5f ? 1.0f : 0.0f);
        }
        else
        {
            chopReleaseDelay = minimumTime - elapsedTime;
            startTimer (static_cast<int> (chopReleaseDelay));
        }
    };

    chopComponent->onCrossfaderValueChanged = [this] (float) {
        updateCrossfader();
    };
}

void MainComponent::setupLibraryComponent()
{
    libraryComponent = std::make_unique<LibraryComponent>(engine);
    addAndMakeVisible(libraryComponent.get());
    
    // Update to use Edit selection instead of File selection
    libraryComponent->onEditSelected = [this](std::unique_ptr<tracktion::engine::Edit> edit) {
        handleEditSelection(std::move(edit));
    };
}

void MainComponent::handleEditSelection(std::unique_ptr<tracktion::engine::Edit> newEdit)
{
    if (!newEdit)
        return;

    // Stop any current playback if we have an existing edit
    if (edit)
        edit->getTransport().stop(false, false);

    // Release current edit resources
    edit = std::move(newEdit);

    // Initialize tracks first
    initialiseTracks();

    // Get the stored BPM from the Edit
    float bpm = edit->state.getProperty("bpm", 120.0f);
    baseTempo = bpm;  // Update base tempo

    // Setup audio graph
    setupAudioGraph();

    // Initialize all edit-dependent components
    reverbComponent = std::make_unique<ReverbComponent>(*edit);
    addAndMakeVisible(*reverbComponent);

    setupChopComponent();

    flangerComponent = std::make_unique<FlangerComponent>(*edit);
    addAndMakeVisible(*flangerComponent);

    delayComponent = std::make_unique<DelayComponent>(*edit);
    addAndMakeVisible(*delayComponent);

    phaserComponent = std::make_unique<PhaserComponent>(*edit);
    addAndMakeVisible(*phaserComponent);

    setupVinylBrakeComponent();
    setupScrewComponent();
    setupScratchComponent();

    // Create transport component
    transportComponent = std::make_unique<TransportComponent>(*edit);
    addAndMakeVisible(*transportComponent);

    // Create plugin rack after all effects are initialized
    createPluginRack();

    // Reset component states
    if (screwComponent)
    {
        screwComponent->setBaseTempo(baseTempo);
        screwComponent->setTempo(baseTempo, juce::dontSendNotification);
    }

    // Initialize the tempo sequence with the base tempo
    auto tempoSetting = edit->tempoSequence.insertTempo(tracktion::TimePosition::fromSeconds(0.0));
    if (tempoSetting != nullptr)
        tempoSetting->setBpm(baseTempo);

    // Reset transport position and state
    edit->getTransport().setPosition(tracktion::TimePosition::fromSeconds(0.0));
    playState = PlayState::Stopped;

    // Reset delay time to 1/4 note if component exists
    if (delayComponent)
    {
        // Calculate quarter note duration in milliseconds
        double quarterNoteMs = (60.0 / baseTempo) * 1000.0;
        delayComponent->setDelayTime(quarterNoteMs);
        delayComponent->setTempo(baseTempo);
    }

    // Reset crossfader to first track
    if (chopComponent)
    {
        chopComponent->setCrossfaderValue(0.0f);
        updateCrossfader();
    }

    // Update plugin components
    if (oscilloscopePlugin)
        oscilloscopePlugin->setEnabled(true);

    // Apply the current tempo to the clips
    updateTempo();

    // Trigger a layout update
    resized();
}

void MainComponent::setTrackVolume (int trackIndex, float gainDB)
{
    if (trackIndex == 0 && volumeAndPan1)
        volumeAndPan1->setVolumeDb (gainDB);
    else if (trackIndex == 1 && volumeAndPan2)
        volumeAndPan2->setVolumeDb (gainDB);
}

void MainComponent::armTrack (int trackIndex, bool arm)
{
    if (auto track = EngineHelpers::getOrInsertAudioTrackAt (*edit, trackIndex))
    {
        EngineHelpers::armTrack (*track, arm);
    }
}

void MainComponent::startRecording()
{
    // Arm the first track for recording
    armTrack (0, true);

    // Start transport recording
    edit->getTransport().record (true);
}

void MainComponent::stopRecording()
{
    // Stop recording
    edit->getTransport().stop (false, false);

    // Disarm track
    armTrack (0, false);
}

void MainComponent::gamepadButtonPressed (int buttonId)
{
    float currentPosition;
    switch (buttonId)
    {
        case SDL_GAMEPAD_BUTTON_SOUTH:
            chopStartTime = juce::Time::getMillisecondCounterHiRes();
            currentPosition = chopComponent->getCrossfaderValue();
            chopComponent->setCrossfaderValue (currentPosition <= 0.5f ? 1.0f : 0.0f);
            break;
        case SDL_GAMEPAD_BUTTON_DPAD_UP:
        {
            if (reverbComponent)
                reverbComponent->rampMixLevel (true);
            break;
        }
        case SDL_GAMEPAD_BUTTON_DPAD_RIGHT:
        {
            if (delayComponent)
                delayComponent->rampMixLevel (true);
            break;
        }
        case SDL_GAMEPAD_BUTTON_DPAD_DOWN:
        {
            if (flangerComponent)
                flangerComponent->rampMixLevel (true);
            break;
        }
    }
}

void MainComponent::gamepadButtonReleased (int buttonId)
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
                chopComponent->setCrossfaderValue (currentPosition <= 0.5f ? 1.0f : 0.0f);
            }
            else
            {
                chopReleaseDelay = minimumTime - elapsedTime;
                startTimer (static_cast<int> (chopReleaseDelay));
            }
            break;
        }
        case SDL_GAMEPAD_BUTTON_DPAD_UP:
        {
            if (reverbComponent)
                reverbComponent->rampMixLevel (false);
            break;
        }
        case SDL_GAMEPAD_BUTTON_DPAD_RIGHT:
        {
            if (delayComponent)
                delayComponent->rampMixLevel (false);
            break;
        }
        case SDL_GAMEPAD_BUTTON_DPAD_DOWN:
        {
            if (flangerComponent)
                flangerComponent->rampMixLevel (false);
            break;
        }
    }
}

void MainComponent::gamepadAxisMoved (int axisId, float value)
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
                    vinylBrakeComponent->setBrakeValue (value);
            }
            break;

        case SDL_GAMEPAD_AXIS_LEFT_TRIGGER:
            // Use left trigger for scratch effect
            // if (scratchComponent)
            // {
            //     // Apply scratch effect based on trigger value
            //     // Trigger values are 0 to 1, we'll map to -1 to 1 for scratch speed
            //     float scratchSpeed = (value > 0.1f) ? (value * 2.0f - 1.0f) : 0.0f;
            //     scratchComponent->applyScratchEffect(scratchSpeed);

            //     // If trigger is released, start returning to original tempo
            //     if (value < 0.1f)
            //     {
            //         scratchComponent->startReturnToOriginalTempo();
            //     }
            // }
            break;

        case SDL_GAMEPAD_AXIS_LEFTX:
            leftX = value;
            if (flangerComponent)
            {
                flangerComponent->setSpeed (value * 10.0f);
                // Calculate width based on stick distance from center
                float distance = std::sqrt (leftX * leftX + leftY * leftY);
                float normalizedDistance = distance / std::sqrt (2.0f);
                float curvedWidth = normalizedDistance * normalizedDistance;
                flangerComponent->setWidth (juce::jlimit (0.0f, 0.99f, curvedWidth));
            }
            break;

        case SDL_GAMEPAD_AXIS_LEFTY:
            leftY = value;
            if (flangerComponent)
            {
                flangerComponent->setDepth (value * 10.0f);
                // Calculate width based on stick distance from center
                float distance = std::sqrt (leftX * leftX + leftY * leftY);
                float normalizedDistance = distance / std::sqrt (2.0f);
                float curvedWidth = normalizedDistance * normalizedDistance;
                flangerComponent->setWidth (juce::jlimit (0.0f, 0.99f, curvedWidth));
            }
            break;

        case SDL_GAMEPAD_AXIS_RIGHTX:
            rightX = value;
            if (phaserComponent)
            {
                phaserComponent->setRate (value * 10.0f);
                float distance = std::sqrt (rightX * rightX + rightY * rightY);
                phaserComponent->setFeedback (juce::jlimit (0.0f, 0.70f, distance));
            }
            break;

        case SDL_GAMEPAD_AXIS_RIGHTY:
            rightY = value;
            if (phaserComponent)
            {
                phaserComponent->setDepth (value * 10.0f);
                float distance = std::sqrt (rightX * rightX + rightY * rightY);
                phaserComponent->setFeedback (juce::jlimit (0.0f, 0.70f, distance));
            }
            break;
    }
}

void MainComponent::setupVinylBrakeComponent()
{
    vinylBrakeComponent = std::make_unique<VinylBrakeComponent> (*edit);

    // Set up the callback to get current tempo adjustment
    vinylBrakeComponent->getCurrentTempoAdjustment = [this]() {
        // Get the current tempo ratio from the screw component
        double ratio = screwComponent->getTempo() / baseTempo;
        // For time stretching, we just return the ratio - 1.0 as before
        // This represents the adjustment from the base tempo
        return ratio - 1.0;
    };

    vinylBrakeComponent->getEffectiveTempo = [this]() {
        return screwComponent->getTempo();
    };

    addAndMakeVisible (*vinylBrakeComponent);
}

void MainComponent::createPluginRack()
{
    if (auto masterTrack = edit->getMasterTrack())
    {
        tracktion::engine::Plugin::Array plugins;

        if (reverbComponent)
            plugins.add (reverbComponent->getPlugin());
        if (delayComponent)
            plugins.add (delayComponent->getPlugin());
        if (flangerComponent)
            plugins.add (flangerComponent->getPlugin());
        if (phaserComponent)
            plugins.add (phaserComponent->getPlugin());
        // if (scratchComponent)
        // plugins.add (scratchComponent->getPlugin());

        // Create the rack type with proper channel connections
        if (auto rack = tracktion::engine::RackType::createTypeToWrapPlugins (plugins, *edit))
        {
            masterTrack->pluginList.insertPlugin (tracktion::engine::RackInstance::create (*rack), 0);
        }
    }
}

void MainComponent::setupOscilloscopeComponent()
{
    // Add oscilloscope to master track
    if (auto masterTrack = edit->getMasterTrack())
    {
        // Register our custom plugins with the engine
        engine.getPluginManager().createBuiltInType<tracktion::engine::OscilloscopePlugin>();

        oscilloscopePlugin = masterTrack->pluginList.insertPlugin (tracktion::engine::OscilloscopePlugin::create(), -1);
        if (oscilloscopePlugin != nullptr)
        {
            if (auto* oscPlugin = dynamic_cast<tracktion::engine::OscilloscopePlugin*> (oscilloscopePlugin.get()))
            {
                oscPlugin->addListener (this);
            }
        }
    }
}

void MainComponent::setupScrewComponent()
{
    screwComponent = std::make_unique<ScrewComponent> (*edit);
    addAndMakeVisible (*screwComponent);

    // Initialize the screw component with the current tempo
    screwComponent->setTempo (baseTempo, juce::dontSendNotification);

    screwComponent->onTempoChanged = [this] (double tempo) {
        updateTempo();
        if (delayComponent)
            delayComponent->setTempo (tempo);
    };
}

void MainComponent::setupScratchComponent()
{
    // Initialize the scratch component
    DBG ("MainComponent: Creating ScratchComponent");
    scratchComponent = std::make_unique<ScratchComponent> (*edit);
    DBG ("MainComponent: ScratchComponent created, now making visible");

    // Set up callbacks to get current tempo and effective tempo
    scratchComponent->getCurrentTempoAdjustment = [this]() {
        // Get the current tempo ratio from the screw component
        if (screwComponent)
        {
            double ratio = screwComponent->getTempo() / baseTempo;
            // For time stretching, we just return the ratio - 1.0
            // This represents the adjustment from the base tempo
            return ratio - 1.0;
        }
        return 0.0;
    };

    scratchComponent->getEffectiveTempo = [this]() {
        return screwComponent ? screwComponent->getTempo() : 120.0;
    };

    // addAndMakeVisible (*scratchComponent);
}

void MainComponent::initialiseTracks()
{
    // Initialize two tracks
    if (auto track1 = EngineHelpers::getOrInsertAudioTrackAt (*edit, 0))
    {
        EngineHelpers::removeAllClips (*track1);
        volumeAndPan1 = dynamic_cast<te::VolumeAndPanPlugin*> (track1->pluginList.insertPlugin (te::VolumeAndPanPlugin::create(), 0).get());

        // Add oscilloscope plugin to track 1
        track1->pluginList.insertPlugin (te::OscilloscopePlugin::create(), -1);
    }

    if (auto track2 = EngineHelpers::getOrInsertAudioTrackAt (*edit, 1))
    {
        EngineHelpers::removeAllClips (*track2);
        volumeAndPan2 = dynamic_cast<te::VolumeAndPanPlugin*> (track2->pluginList.insertPlugin (te::VolumeAndPanPlugin::create(), 0).get());
    }
}

void MainComponent::setupAudioGraph()
{
    // Ensure transport is stopped
    edit->getTransport().stop(false, false);
    
    // Reset transport position
    edit->getTransport().setPosition(tracktion::TimePosition::fromSeconds(0.0));
    
    // Ensure playback context is allocated
    edit->getTransport().ensureContextAllocated();
}

void MainComponent::releaseResources()
{
    // Stop any active timers
    stopTimer();

    // Stop playback if active
    if (edit->getTransport().isPlaying())
        edit->getTransport().stop (true, false);

    // Debug reference counting
    if (oscilloscopePlugin != nullptr)
    {
        DBG ("Oscilloscope plugin reference count: " + juce::String (oscilloscopePlugin->getReferenceCount()));
    }

    // Remove oscilloscope listener
    if (auto* oscPlugin = dynamic_cast<tracktion::engine::OscilloscopePlugin*> (oscilloscopePlugin.get()))
    {
        oscPlugin->removeListener (this);
        DBG ("Removed oscilloscope listener");
    }

    // Clear all component pointers in a specific order
    oscilloscopeComponent = nullptr;
    controllerMappingComponent = nullptr;
    libraryComponent = nullptr;

    phaserComponent = nullptr;
    delayComponent = nullptr;
    flangerComponent = nullptr;
    screwComponent = nullptr;
    chopComponent = nullptr;
    scratchComponent = nullptr;
    reverbComponent = nullptr;
    vinylBrakeComponent = nullptr;

    // Release plugin reference
    oscilloscopePlugin = nullptr;

    // Clear gamepad manager
    if (gamepadManager)
        gamepadManager->removeListener (this);
    gamepadManager = nullptr;
}

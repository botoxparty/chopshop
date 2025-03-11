#include "MainComponent.h"
#include "ChopComponent.h"
#include "Plugins/ChopPlugin.h"
#include <algorithm>

#define JUCE_USE_DIRECTWRITE 0 // Fix drawing of Monospace fonts in Code Editor!

// Initialize settings icon path data
const uint8 MainComponent::settingsIconPathData[486] = { 110, 109, 202, 111, 210, 64, 243, 226, 61, 64, 108, 0, 0, 224, 64, 0, 0, 0, 0, 108, 0, 0, 48, 65, 0, 0, 0, 0, 108, 27, 200, 54, 65, 243, 226, 61, 64, 98, 91, 248, 63, 65, 174, 170, 76, 64, 95, 130, 72, 65, 231, 138, 96, 64, 46, 46, 80, 65, 180, 163, 120, 64, 108, 42, 181, 124, 65, 20, 38, 49, 64, 108, 149, 90, 142, 65, 246, 108, 199, 64, 108, 68, 249, 118, 65, 2, 85, 1, 65, 98, 112, 166, 119, 65, 201, 31, 6, 65, 0, 0, 120, 65, 111, 5, 11, 65, 0, 0, 120, 65, 0, 0, 16, 65, 98, 0, 0, 120, 65, 145, 250, 20, 65, 108, 166, 119, 65, 55, 224, 25, 65, 72, 249, 118, 65, 254, 170, 30, 65, 108, 151, 90, 142, 65, 133, 73, 60, 65, 108, 46, 181, 124, 65, 123, 182, 115, 65, 108, 50, 46, 80, 65, 18, 215, 97, 65, 98, 99, 130, 72, 65, 70, 221, 103, 65, 96, 248, 63, 65, 83, 213, 108, 65, 32, 200, 54, 65, 66, 135, 112, 65, 108, 0, 0, 48, 65, 0, 0, 144, 65, 108, 0, 0, 224, 64, 0, 0, 144, 65, 108, 202, 111, 210, 64, 67, 135, 112, 65, 98, 74, 15, 192, 64, 84, 213, 108, 65, 65, 251, 174, 64, 70, 221, 103, 65, 164, 163, 159, 64, 19, 215, 97, 65, 108, 92, 43, 13, 64, 123, 182, 115, 65, 108, 187, 181, 82, 62, 133, 73, 60, 65, 108, 244, 26, 36, 64, 254, 170, 30, 65, 98, 64, 102, 33, 64, 55, 224, 25, 65, 0, 0, 32, 64, 145, 250, 20, 65, 0, 0, 32, 64, 0, 0, 16, 65, 98, 0, 0, 32, 64, 111, 5, 11, 65, 64, 102, 33, 64, 201, 31, 6, 65, 244, 26, 36, 64, 2, 85, 1, 65, 108, 187, 181, 82, 62, 246, 108, 199, 64, 108, 92, 43, 13, 64, 20, 38, 49, 64, 108, 164, 163, 159, 64, 180, 163, 120, 64, 98, 65, 251, 174, 64, 231, 138, 96, 64, 74, 15, 192, 64, 175, 170, 76, 64, 202, 111, 210, 64, 243, 226, 61, 64, 99, 109, 0, 0, 16, 65, 0, 0, 64, 65, 98, 121, 130, 42, 65, 0, 0, 64, 65, 0, 0, 64, 65, 121, 130, 42, 65, 0, 0, 64, 65, 0, 0, 16, 65, 98, 0, 0, 64, 65, 13, 251, 234, 64, 121, 130, 42, 65, 0, 0, 192, 64, 0, 0, 16, 65, 0, 0, 192, 64, 98, 13, 251, 234, 64, 0, 0, 192, 64, 0, 0, 192, 64, 13, 251, 234, 64, 0, 0, 192, 64, 0, 0, 16, 65, 98, 0, 0, 192, 64, 121, 130, 42, 65, 13, 251, 234, 64, 0, 0, 64, 65, 0, 0, 16, 65, 0, 0, 64, 65, 99, 101, 0, 0 };

//==============================================================================
MainComponent::MainComponent()
{
    // Create a global command manager
    commandManager = std::make_unique<juce::ApplicationCommandManager>();

    // Register all commands with the command manager
    commandManager->registerAllCommandsForTarget(this);

    // Add key mappings to the top level component
    addKeyListener(commandManager->getKeyMappings());

    // Set this as the first command target
    commandManager->setFirstCommandTarget(this);

    // Set up menu bar (in-window for all platforms)
    setApplicationCommandManagerToWatch(commandManager.get());
    menuBar = std::make_unique<juce::MenuBarComponent>(this);
    addAndMakeVisible(*menuBar);

    customLookAndFeel = std::make_unique<CustomLookAndFeel>();
    LookAndFeel::setDefaultLookAndFeel(customLookAndFeel.get());
    getLookAndFeel().setDefaultSansSerifTypefaceName("Arial");
    setSize(924, 720);
    startTimerHz(30);

    // Initialize licensing UI
    addAndMakeVisible(showActivationUiButton);
    showActivationUiButton.onClick = [&]()
    {
        MOONBASE_SHOW_ACTIVATION_UI;
    };

    if (activationUI != nullptr)
    {
        activationUI->setWelcomePageText("ChopShop", "License Management");
        activationUI->setSpinnerLogo(juce::Drawable::createFromImageData(BinaryData::PoundingSystemsLogo_png, 
                                                                        BinaryData::PoundingSystemsLogo_pngSize));
        activationUI->setCompanyLogo(std::make_unique<CompanyLogo>());
        activationUI->setWelcomeButtonTextScale(0.3f);
    }

    // Register our custom plugins with the engine
    engine.getPluginManager().createBuiltInType<tracktion::engine::OscilloscopePlugin>();
    engine.getPluginManager().createBuiltInType<FlangerPlugin>();
    engine.getPluginManager().createBuiltInType<AutoDelayPlugin>();
    engine.getPluginManager().createBuiltInType<AutoPhaserPlugin>();
    engine.getPluginManager().createBuiltInType<ScratchPlugin>();
    engine.getPluginManager().createBuiltInType<ChopPlugin>();

    // Initialize audio settings button with icon
    audioSettingsButton = std::make_unique<juce::DrawableButton> ("Audio Settings", juce::DrawableButton::ImageOnButtonBackground);
    juce::Path settingsIconPath;
    settingsIconPath.loadPathFromData (settingsIconPathData, sizeof (settingsIconPathData));

    auto drawable = std::make_unique<juce::DrawablePath>();
    drawable->setPath (settingsIconPath);
    drawable->setFill (getLookAndFeel().findColour (juce::DrawableButton::textColourId));

    audioSettingsButton->setImages (drawable.get());
    addAndMakeVisible (*audioSettingsButton);

    // Add the button callback
    audioSettingsButton->onClick = [this] {
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

    if (juce::RuntimePermissions::isRequired(juce::RuntimePermissions::recordAudio)
        && !juce::RuntimePermissions::isGranted(juce::RuntimePermissions::recordAudio))
    {
        juce::RuntimePermissions::request(juce::RuntimePermissions::recordAudio,
                                        [&](bool granted) { setAudioChannels(granted ? 2 : 0, 2); });
    }
    else
    {
        setAudioChannels(2, 2);
    }
}

void MainComponent::prepareToPlay(int samplesPerBlockExpected, double sampleRate)
{
    MOONBASE_PREPARE_TO_PLAY(sampleRate, samplesPerBlockExpected);
}

void MainComponent::getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill)
{
    bufferToFill.clearActiveBufferRegion();

    // Your audio processing code here

    if (auto* buffer = bufferToFill.buffer)
    {
        MOONBASE_PROCESS(*buffer);
    }
}

MainComponent::~MainComponent()
{
    // Remove key listener before destroying command manager
    removeKeyListener (commandManager->getKeyMappings());

    // Call releaseResources first to ensure proper cleanup
    releaseResources();
    shutdownAudio();

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
    bounds.reduce(10, 10); // Add some padding

    // Position menu bar at the top for all platforms
    if (menuBar)
    {
        menuBar->setBounds(bounds.removeFromTop(25));
        bounds.removeFromTop(5); // Add a small gap after the menu bar
    }

    // Position activation UI button
    juce::Rectangle<int> activationUiButtonArea(250, 30);
    activationUiButtonArea.setCentre(bounds.getCentre().withY(bounds.getY() + 40));
    showActivationUiButton.setBounds(activationUiButtonArea);

    MOONBASE_RESIZE_ACTIVATION_UI;

    // Always position the library bar at the top
    if (libraryBar != nullptr)
    {
        auto libraryHeight = 40;
        auto libraryBounds = bounds.removeFromTop(libraryHeight);

        // Position settings button and controller mapping button on the right
        if (audioSettingsButton != nullptr && controllerMappingComponent != nullptr)
        {
            auto buttonSize = 30;
            auto buttonSpacing = 5;

            // Position controller mapping button first (rightmost)
            auto controllerBounds = libraryBounds.removeFromRight(buttonSize + buttonSpacing).withSizeKeepingCentre(buttonSize, buttonSize);
            controllerMappingComponent->setBounds(controllerBounds);

            // Then position settings button to its left
            auto settingsBounds = libraryBounds.removeFromRight(buttonSize + buttonSpacing).withSizeKeepingCentre(buttonSize, buttonSize);
            audioSettingsButton->setBounds(settingsBounds);
        }

        // Position library bar in remaining space
        libraryBar->setBounds(libraryBounds);
    }

    // Position library component in the remaining space if visible
    if (libraryComponent && libraryComponent->isVisible()) {
        libraryComponent->setBounds(bounds);
        libraryComponent->toFront(false); // Bring to front but don't focus
    }

    // Always layout other components even if library is visible
    // Row 1: Transport control
    if (transportComponent != nullptr)
    {
        auto transportHeight = bounds.getHeight() / 2; // 1/2 of total height
        transportComponent->setBounds(bounds.removeFromTop(transportHeight));
    }

    // Main layout using FlexBox
    juce::FlexBox mainBox;
    mainBox.flexDirection = juce::FlexBox::Direction::column;
    mainBox.flexWrap = juce::FlexBox::Wrap::noWrap;
    mainBox.justifyContent = juce::FlexBox::JustifyContent::spaceBetween;

    // Row 1: Screw and Chop components
    juce::FlexBox topRow;
    topRow.flexDirection = juce::FlexBox::Direction::row;
    topRow.justifyContent = juce::FlexBox::JustifyContent::spaceBetween;
    if (chopComponent)
        topRow.items.add(juce::FlexItem(*chopComponent).withFlex(1.0f).withMargin(5));
    if (screwComponent)
        topRow.items.add(juce::FlexItem(*screwComponent).withFlex(1.0f).withMargin(5));

    // Row 2: Effects Box with three columns
    juce::FlexBox effectsBox;
    effectsBox.flexDirection = juce::FlexBox::Direction::row;
    effectsBox.justifyContent = juce::FlexBox::JustifyContent::spaceBetween;

    // Column 1: Scratch and Vinyl Brake
    juce::FlexBox effectsColumn1;
    effectsColumn1.flexDirection = juce::FlexBox::Direction::column;
    if (scratchComponent)
        effectsColumn1.items.add(juce::FlexItem(*scratchComponent).withFlex(1.0f).withMargin(5));
    if (vinylBrakeComponent)
        effectsColumn1.items.add(juce::FlexItem(*vinylBrakeComponent).withFlex(1.0f).withMargin(5));

    // Column 2: Reverb and Delay
    juce::FlexBox effectsColumn2;
    effectsColumn2.flexDirection = juce::FlexBox::Direction::column;
    if (reverbComponent)
        effectsColumn2.items.add(juce::FlexItem(*reverbComponent).withFlex(1.0f).withMargin(5));
    if (delayComponent)
        effectsColumn2.items.add(juce::FlexItem(*delayComponent).withFlex(1.0f).withMargin(5));

    // Column 3: Flanger and Phaser
    juce::FlexBox effectsColumn3;
    effectsColumn3.flexDirection = juce::FlexBox::Direction::column;
    if (flangerComponent)
        effectsColumn3.items.add(juce::FlexItem(*flangerComponent).withFlex(1.0f).withMargin(5));
    if (phaserComponent)
        effectsColumn3.items.add(juce::FlexItem(*phaserComponent).withFlex(1.0f).withMargin(5));

    // Add columns to effects box
    effectsBox.items.add(juce::FlexItem(effectsColumn1).withFlex(1.0f));
    effectsBox.items.add(juce::FlexItem(effectsColumn2).withFlex(1.0f));
    effectsBox.items.add(juce::FlexItem(effectsColumn3).withFlex(1.0f));

    // Add rows to main box
    mainBox.items.add(juce::FlexItem(topRow).withFlex(0.3f));
    mainBox.items.add(juce::FlexItem(effectsBox).withFlex(0.7f));

    // Perform the layout
    mainBox.performLayout(bounds);
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

    // Set up the tempo callback
    chopComponent->getTempoCallback = [this]() {
        return screwComponent->getTempo();
    };
}

void MainComponent::setupLibraryComponent()
{
    // Create the library component directly
    libraryComponent = std::make_unique<LibraryComponent>(engine);
    addAndMakeVisible(*libraryComponent);

    // Create the library bar
    libraryBar = std::make_unique<LibraryBar>();
    addAndMakeVisible(libraryBar.get());

    // Set up the show library button callback to toggle visibility
    libraryBar->getShowLibraryButton().onClick = [this]() {
        if (libraryComponent && libraryBar) {
            bool showLibrary = !libraryComponent->isVisible();
            libraryComponent->setVisible(showLibrary);
            resized();
        }
    };

    // Update to use Edit selection instead of File selection
    libraryComponent->onEditSelected = [this](std::unique_ptr<tracktion::engine::Edit> newEdit) {
        handleEditSelection(std::move(newEdit));
    };

    // Show library by default
    if (libraryComponent) {
        libraryComponent->setVisible(true);
    }
}

void MainComponent::handleEditSelection (std::unique_ptr<tracktion::engine::Edit> newEdit)
{
    if (!newEdit)
        return;

    // Stop any current playback if we have an existing edit
    if (edit)
    {
        edit->getTransport().stop (false, false);

        // Clean up all edit-dependent components first
        transportComponent = nullptr;
        reverbComponent = nullptr;
        delayComponent = nullptr;
        flangerComponent = nullptr;
        phaserComponent = nullptr;
        chopComponent = nullptr;
        screwComponent = nullptr;
        scratchComponent = nullptr;
        vinylBrakeComponent = nullptr;
        oscilloscopeComponent = nullptr;
    }

    // Release current edit and assign new one
    edit = std::move (newEdit);

    // Update library bar with track name
    libraryBar->setCurrentTrackName (edit->getName());

    // Hide library window since we now have an edit loaded
    libraryComponent->setVisible (false);

    // Get the stored BPM from the Edit
    float bpm = edit->state.getProperty ("bpm", 120.0f);
    baseTempo = bpm; // Update base tempo

    // Setup audio graph
    setupAudioGraph();

    // Initialize all edit-dependent components
    reverbComponent = std::make_unique<ReverbComponent> (*edit);
    addAndMakeVisible (*reverbComponent);

    DBG ("Setup reverb component");

    setupChopComponent();

    DBG ("Setup chop component");

    flangerComponent = std::make_unique<FlangerComponent> (*edit);
    addAndMakeVisible (*flangerComponent);

    delayComponent = std::make_unique<DelayComponent> (*edit);
    addAndMakeVisible (*delayComponent);

    phaserComponent = std::make_unique<PhaserComponent> (*edit);
    addAndMakeVisible (*phaserComponent);

    DBG ("Setup phaser component");

    setupVinylBrakeComponent();
    setupScrewComponent();
    setupScratchComponent();

    // Create transport component
    transportComponent = std::make_unique<TransportComponent> (*edit);
    addAndMakeVisible (*transportComponent);

    // Reset component states
    if (screwComponent)
    {
        screwComponent->setBaseTempo (baseTempo);
        screwComponent->setTempo (baseTempo, juce::dontSendNotification);
    }

    // Initialize the tempo sequence with the base tempo
    auto tempoSetting = edit->tempoSequence.insertTempo (tracktion::TimePosition::fromSeconds (0.0));
    if (tempoSetting != nullptr)
        tempoSetting->setBpm (baseTempo);

    // Reset transport position and state
    edit->getTransport().setPosition (tracktion::TimePosition::fromSeconds (0.0));
    playState = PlayState::Stopped;

    // Reset delay time to 1/4 note if component exists
    if (delayComponent)
    {
        // Calculate quarter note duration in milliseconds
        double quarterNoteMs = (60.0 / baseTempo) * 1000.0;
        delayComponent->setDelayTime (quarterNoteMs);
        delayComponent->setTempo (baseTempo);
    }

    // Update plugin components
    if (oscilloscopePlugin)
        oscilloscopePlugin->setEnabled (true);

    // Apply the current tempo to the clips
    updateTempo();

    // Force transport component to update its thumbnail
    if (transportComponent)
        transportComponent->updateThumbnail();

    // Trigger a layout update
    resized();
}

void MainComponent::armTrack (int trackIndex, bool arm)
{
    if (auto track = EngineHelpers::getAudioTrack (*edit, trackIndex))
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
            if (chopComponent)
                chopComponent->handleChopButtonPressed();
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
            if (chopComponent)
                chopComponent->handleChopButtonReleased();
            break;
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

void MainComponent::setupAudioGraph()
{
    // Ensure transport is stopped
    edit->getTransport().stop (false, false);

    // Reset transport position
    edit->getTransport().setPosition (tracktion::TimePosition::fromSeconds (0.0));

    // Ensure playback context is allocated
    edit->getTransport().ensureContextAllocated();
}

void MainComponent::releaseResources()
{
    // Stop any active timers
    stopTimer();

    // Stop playback if active and edit is valid
    if (edit != nullptr)
    {
        if (edit->getTransport().isPlaying())
            edit->getTransport().stop(true, false);
    }

    // First, clean up transport component as it has automation listeners
    transportComponent = nullptr;

    // Debug reference counting
    if (oscilloscopePlugin != nullptr)
    {
        DBG("Oscilloscope plugin reference count: " + juce::String(oscilloscopePlugin->getReferenceCount()));
    }

    // Remove oscilloscope listener
    if (auto* oscPlugin = dynamic_cast<tracktion::engine::OscilloscopePlugin*>(oscilloscopePlugin.get()))
    {
        oscPlugin->removeListener(this);
        DBG("Removed oscilloscope listener");
    }

    // Clear all component pointers in a specific order
    oscilloscopeComponent = nullptr;
    controllerMappingComponent = nullptr;
    libraryBar = nullptr;
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
        gamepadManager->removeListener(this);
    gamepadManager = nullptr;

    // Clear edit pointer last
    edit = nullptr;
}

void MainComponent::getAllCommands (juce::Array<juce::CommandID>& commands)
{
    commands.add (CommandIDs::DeleteSelectedRegion);
    commands.add (CommandIDs::SaveProject);
    commands.add (CommandIDs::Undo);
    commands.add (CommandIDs::Redo);
}

void MainComponent::getCommandInfo (juce::CommandID commandID, juce::ApplicationCommandInfo& result)
{
    switch (commandID)
    {
        case CommandIDs::DeleteSelectedRegion:
            result.setInfo ("Delete Selected Region", "Deletes the currently selected region", "Edit", 0);
            result.addDefaultKeypress (juce::KeyPress::deleteKey, 0);
            break;

        case CommandIDs::SaveProject:
            result.setInfo ("Save", "Saves the current project", "File", 0);
            result.addDefaultKeypress ('s', juce::ModifierKeys::commandModifier);
            break;

        case CommandIDs::Undo:
            result.setInfo ("Undo", "Undo the last action", "Edit", 0);
            result.addDefaultKeypress ('z', juce::ModifierKeys::commandModifier);
            result.setActive (edit != nullptr && edit->getUndoManager().canUndo());
            break;

        case CommandIDs::Redo:
            result.setInfo ("Redo", "Redo the last undone action", "Edit", 0);
            result.addDefaultKeypress ('z', juce::ModifierKeys::commandModifier | juce::ModifierKeys::shiftModifier);
            result.setActive (edit != nullptr && edit->getUndoManager().canRedo());
            break;

        default:
            break;
    }
}

bool MainComponent::perform (const juce::ApplicationCommandTarget::InvocationInfo& info)
{
    switch (info.commandID)
    {
        case CommandIDs::DeleteSelectedRegion:
            if (transportComponent != nullptr)
                transportComponent->deleteSelectedChopRegion();
            break;

        case CommandIDs::SaveProject:
            if (edit != nullptr)
            {
                // 1. Flush the current state to the ValueTree
                edit->flushState();

                // 2. Create EditFileOperations
                EditFileOperations operations (*edit);

                // 3. Save with desired options
                // - warnOfFailure: Show error dialog if save fails
                // - forceSaveEvenIfNotModified: Save even if no changes
                // - offerToDiscardChanges: Show dialog asking to save changes
                bool success = operations.save (true, false, false);

                if (success == true)
                {
                    juce::AlertWindow::showMessageBoxAsync (juce::AlertWindow::InfoIcon,
                        "Save",
                        "Project saved successfully!");
                }
            }
            break;

        case CommandIDs::Undo:
            if (edit != nullptr)
            {
                edit->getUndoManager().undo();
                // Update UI state after undo
                commandManager->commandStatusChanged();
            }
            break;

        case CommandIDs::Redo:
            if (edit != nullptr)
            {
                edit->getUndoManager().redo();
                // Update UI state after redo
                commandManager->commandStatusChanged();
            }
            break;

        default:
            return false;
    }

    return true;
}

juce::StringArray MainComponent::getMenuBarNames()
{
    return { "File", "Edit" };
}

juce::PopupMenu MainComponent::getMenuForIndex (int topLevelMenuIndex, const juce::String& /*menuName*/)
{
    juce::PopupMenu menu;

    if (topLevelMenuIndex == 0) // File menu
    {
        menu.addCommandItem(commandManager.get(), CommandIDs::SaveProject);
        menu.addSeparator();
        menu.addItem(1, "Audio Settings", true, false);
        menu.addItem(3, "Game Controller Settings", true, false);
        menu.addSeparator();
        menu.addItem(2, "Quit", true, false);
    }
    else if (topLevelMenuIndex == 1) // Edit menu
    {
        menu.addCommandItem(commandManager.get(), CommandIDs::Undo);
        menu.addCommandItem(commandManager.get(), CommandIDs::Redo);
        menu.addSeparator();
        menu.addCommandItem(commandManager.get(), CommandIDs::DeleteSelectedRegion);
    }

    return menu;
}

void MainComponent::menuItemSelected (int menuItemID, int topLevelMenuIndex)
{
    if (topLevelMenuIndex == 0) // File menu
    {
        switch (menuItemID)
        {
            case 1: // Audio Settings
                EngineHelpers::showAudioDeviceSettings(engine);
                break;
            case 2: // Quit
                juce::JUCEApplication::getInstance()->systemRequestedQuit();
                break;
            case 3: // Game Controller Settings
                if (controllerMappingComponent)
                {
                    controllerMappingComponent->toFront(true);
                    controllerMappingComponent->setVisible(true);
                }
                break;
            default:
                break;
        }
    }
}

void MainComponent::gamepadTouchpadMoved(float x, float y, bool touched)
{
    // Implement touchpad handling here if needed
    // For now, we can leave it empty if you're not using the touchpad
}

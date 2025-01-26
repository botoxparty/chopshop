#include "MainComponent.h"
#include <algorithm>

//==============================================================================
MainComponent::MainComponent()
{
    // Add this near the start of the constructor, before other component setup
    currentTrackLabel.setJustificationType(juce::Justification::centred);
    currentTrackLabel.setText("No Track Loaded", juce::dontSendNotification);
    currentTrackLabel.setFont(juce::Font(16.0f));
    currentTrackLabel.setMinimumHorizontalScale(1.0f);
    addAndMakeVisible(currentTrackLabel);

    positionLabel.setJustificationType(juce::Justification::centred);
    positionLabel.setFont(juce::Font(16.0f));
    positionLabel.setMinimumHorizontalScale(1.0f);
    addAndMakeVisible(positionLabel);

    // Register our custom plugins with the engine
    engine.getPluginManager().createBuiltInType<tracktion_engine::OscilloscopePlugin>();

    addAndMakeVisible(openButton);
    addAndMakeVisible(saveButton);
    addAndMakeVisible(playButton);
    addAndMakeVisible(stopButton);
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

    setSize(1024, 768);

    playButton.setToggleState(false, juce::NotificationType::dontSendNotification);
    stopButton.setToggleState(true, juce::NotificationType::dontSendNotification);

    playButton.onClick = [this]
    { play(); };
    stopButton.onClick = [this]
    { stop(); };
    openButton.onClick = [this]
    { loadAudioFile(); };

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

    chopComponent = std::make_unique<ChopComponent>(edit);
    addAndMakeVisible(*chopComponent);
    
    chopComponent->onChopButtonPressed = [this]() {
        chopStartTime = juce::Time::getMillisecondCounterHiRes();
        float currentPosition = chopComponent->getCrossfaderValue();
        chopComponent->setCrossfaderValue(currentPosition <= 0.5f ? 1.0f : 0.0f);
    };
    
    chopComponent->onChopButtonReleased = [this]() {
        double elapsedTime = juce::Time::getMillisecondCounterHiRes() - chopStartTime;
        double minimumTime = chopComponent->getChopDurationInMs(screwComponent->getTempo());

        if (elapsedTime >= minimumTime) {
            float currentPosition = chopComponent->getCrossfaderValue();
            chopComponent->setCrossfaderValue(currentPosition <= 0.5f ? 1.0f : 0.0f);
        } else {
            chopReleaseDelay = minimumTime - elapsedTime;
            startTimer(static_cast<int>(chopReleaseDelay));
        }
    };

    getLookAndFeel().setDefaultSansSerifTypefaceName("Arial");

    addAndMakeVisible(currentTrackLabel);

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

    updateButtonStates();

    libraryComponent = std::make_unique<LibraryComponent>();
    addAndMakeVisible(*libraryComponent);

    // Set up the callback for when a file is selected
    libraryComponent->onFileSelected = [this](const juce::File& file) {
        handleFileSelection(file);
    };

    // Initialize two tracks
    if (auto track1 = EngineHelpers::getOrInsertAudioTrackAt(edit, 0))
    {
        EngineHelpers::removeAllClips(*track1);
        volumeAndPan1 = dynamic_cast<te::VolumeAndPanPlugin*>(track1->pluginList.insertPlugin(te::VolumeAndPanPlugin::create(), 0).get());
        
        // Add oscilloscope plugin to track 1
        track1->pluginList.insertPlugin(te::OscilloscopePlugin::create(), -1);
    }

    if (auto track2 = EngineHelpers::getOrInsertAudioTrackAt(edit, 1))
    {
        EngineHelpers::removeAllClips(*track2);
        volumeAndPan2 = dynamic_cast<te::VolumeAndPanPlugin*>(track2->pluginList.insertPlugin(te::VolumeAndPanPlugin::create(), 0).get());
    }

    vinylBrakeComponent = std::make_unique<VinylBrakeComponent>(edit);
    addAndMakeVisible(*vinylBrakeComponent);

    resized();

    startTimerHz(30); // Update 30 times per second

    // Add oscilloscope to master track
    if (auto masterTrack = edit.getMasterTrack())
    {
        DBG("Found master track");
        
        // Register our custom plugins with the engine
        engine.getPluginManager().createBuiltInType<tracktion_engine::OscilloscopePlugin>();
        
        oscilloscopePlugin = std::shared_ptr<tracktion_engine::Plugin>(masterTrack->pluginList.insertPlugin(tracktion_engine::OscilloscopePlugin::create(), -1).get());
        if (oscilloscopePlugin != nullptr)
        {
            DBG("Created oscilloscope plugin");
            if (auto* oscPlugin = dynamic_cast<tracktion_engine::OscilloscopePlugin*>(oscilloscopePlugin.get()))
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
    chopComponent->onCrossfaderValueChanged = [this](float value) { 
        updateCrossfader(); 
    };

    screwComponent = std::make_unique<ScrewComponent>(edit);
    addAndMakeVisible(*screwComponent);

    // Initialize the screw component with the current tempo
    screwComponent->setTempo(baseTempo, juce::dontSendNotification);

    screwComponent->onTempoChanged = [this](double tempo) {
        updateTempo();
    };
}

MainComponent::~MainComponent()
{
    if (auto* oscPlugin = dynamic_cast<tracktion_engine::OscilloscopePlugin*>(oscilloscopePlugin.get()))
        oscPlugin->removeListener(this);
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

    // Row 2: Control Bar
    juce::FlexBox controlBarBox;
    controlBarBox.flexDirection = juce::FlexBox::Direction::row;
    controlBarBox.justifyContent = juce::FlexBox::JustifyContent::flexStart;
    controlBarBox.alignItems = juce::FlexBox::AlignItems::center;

    // Track label with border - modified to take full width
    juce::FlexBox trackLabelBox;
    trackLabelBox.flexDirection = juce::FlexBox::Direction::row;
    trackLabelBox.items.add(juce::FlexItem(currentTrackLabel).withFlex(1.0f).withHeight(30).withMargin(2));

    juce::FlexBox positionLabelBox;
    positionLabelBox.flexDirection = juce::FlexBox::Direction::row;
    positionLabelBox.items.add(juce::FlexItem(positionLabel).withWidth(200).withHeight(30).withMargin(2));

    // Transport controls
    juce::FlexBox transportBox;
    transportBox.flexDirection = juce::FlexBox::Direction::row;
    transportBox.items.add(juce::FlexItem(playButton).withWidth(80).withHeight(30).withMargin(2));
    transportBox.items.add(juce::FlexItem(stopButton).withWidth(80).withHeight(30).withMargin(2));
    transportBox.items.add(juce::FlexItem(recordButton).withWidth(80).withHeight(30).withMargin(2));

    // Modified to make trackLabelBox take remaining space
    controlBarBox.items.add(juce::FlexItem(trackLabelBox).withFlex(1.0f));
    controlBarBox.items.add(juce::FlexItem(positionLabelBox).withWidth(200));
    controlBarBox.items.add(juce::FlexItem(transportBox).withWidth(260));

    // Add control bar to main column
    mainColumn.items.add(juce::FlexItem(controlBarBox).withHeight(40).withMargin(5));

    // Row 3: Main Box (remaining space)
    juce::FlexBox mainBox;
    mainBox.flexDirection = juce::FlexBox::Direction::row;
    mainBox.flexWrap = juce::FlexBox::Wrap::noWrap;
    mainBox.justifyContent = juce::FlexBox::JustifyContent::spaceAround;

    // Column 1 (Transport controls)
    juce::FlexBox column1;
    column1.flexDirection = juce::FlexBox::Direction::column;
    column1.items.add(juce::FlexItem(openButton).withHeight(30).withMargin(5));
    column1.items.add(juce::FlexItem(*libraryComponent).withFlex(1.0f).withHeight(300).withMargin(5));
    column1.items.add(juce::FlexItem(audioSettingsButton).withHeight(30).withMargin(5));

    // Column 2 (Tempo and crossfader)
    juce::FlexBox column2;
    column2.flexDirection = juce::FlexBox::Direction::column;
    column2.items.add(juce::FlexItem(*screwComponent).withMinHeight(120).withMargin(5));
    column2.items.add(juce::FlexItem(*chopComponent).withMinHeight(120).withMargin(5));
    column2.items.add(juce::FlexItem(*vinylBrakeComponent).withMinHeight(120).withMargin(5));

    // Column 3 (Effects)
    juce::FlexBox column3;
    column3.flexDirection = juce::FlexBox::Direction::column;
    column3.items.add(juce::FlexItem(*reverbComponent).withFlex(1.0f).withMinHeight(120).withMargin(5));
    column3.items.add(juce::FlexItem(*delayComponent).withFlex(1.0f).withMinHeight(120).withMargin(5));
    column3.items.add(juce::FlexItem(*flangerComponent).withFlex(1.0f).withMinHeight(120).withMargin(5));

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
    stopButton.setToggleState(!isPlaying, juce::NotificationType::dontSendNotification);
    playButton.setToggleState(isPlaying, juce::NotificationType::dontSendNotification);
    playButton.setButtonText(isPlaying ? "Pause" : "Play");
    playState = isPlaying ? PlayState::Playing : PlayState::Stopped;
}

void MainComponent::stop()
{
    EngineHelpers::togglePlay(edit, EngineHelpers::ReturnToStart::yes);

    // Stop transport and reset position
    edit.getTransport().stop(true, false);
    edit.getTransport().setPosition(tracktion::TimePosition::fromSeconds(0.0));

    playState = PlayState::Stopped;
    stopButton.setToggleState(true, juce::NotificationType::dontSendNotification);
    playButton.setToggleState(false, juce::NotificationType::dontSendNotification);
    playButton.setButtonText("Play");

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
        stopButton.setToggleState(true, juce::NotificationType::dontSendNotification);
        playButton.setToggleState(false, juce::NotificationType::dontSendNotification);
        playButton.setButtonText("Play");

        // Load audio file and get samples
        juce::AudioFormatManager formatManager;
        formatManager.registerBasicFormats();

        std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file));
        if (reader)
        {
            const float minTempo = 60.0f;
            const float maxTempo = 200.0f;
            const int minPeakDistance = static_cast<int>(reader->sampleRate * 0.01); // 10ms
            
            // Create MiniBPM detector
            breakfastquay::MiniBPM bpmDetector(reader->sampleRate);
            bpmDetector.setBPMRange(60, 180);  // typical range for music
            
            // Process audio in chunks
            const int blockSize = 1024;
            juce::AudioBuffer<float> buffer(1, blockSize);
            std::vector<float> samples(blockSize);
            
            for (int pos = 0; pos < reader->lengthInSamples; pos += blockSize) 
            {
                const int numSamples = std::min(blockSize, 
                    static_cast<int>(reader->lengthInSamples - pos));
                    
                reader->read(&buffer, 0, numSamples, pos, true, false);
                memcpy(samples.data(), buffer.getReadPointer(0), numSamples * sizeof(float));
                
                bpmDetector.process(samples.data(), numSamples);
            }
            
            float detectedBPM = bpmDetector.estimateTempo();
            
            if (detectedBPM > 0)
            {
                baseTempo = detectedBPM;
                trackOffset = (60.0 / baseTempo) * 1000.0;
                screwComponent->setTempo(baseTempo, juce::dontSendNotification);
            }
            else
            {
                baseTempo = 120.0;  // fallback value
                trackOffset = (60.0 / baseTempo) * 1000.0;
                screwComponent->setTempo(baseTempo, juce::dontSendNotification);
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
        currentTrackLabel.setText(file.getFileNameWithoutExtension(), juce::dontSendNotification);

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
}

void MainComponent::updateTempo()
{
    const double ratio = screwComponent->getTempo() / baseTempo;
    // Convert ratio to plus/minus proportion (e.g., 1.5 becomes 0.5, 0.5 becomes -0.5)
    const double plusOrMinusProportion = ratio - 1.0;

    edit.getTransport().getCurrentPlaybackContext()->setTempoAdjustment(plusOrMinusProportion);
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
    const float threshold = 0.02f; // 2% threshold for complete silence
    const float minDB = -60.0f;    // Effectively silent

    // Track 1 volume
    if (position >= 1.0f - threshold)
        setTrackVolume(0, minDB);  // Completely silent
    else if (position <= threshold)
        setTrackVolume(0, 0.0f);   // Full volume
    else
        setTrackVolume(0, minDB * position);

    // Track 2 volume
    if (position <= threshold)
        setTrackVolume(1, minDB);  // Completely silent
    else if (position >= 1.0f - threshold)
        setTrackVolume(1, 0.0f);   // Full volume
    else
        setTrackVolume(1, minDB * (1.0f - position));
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
    float currentPosition;  // Moved outside switch
    
    switch (buttonId)
    {
        case SDL_CONTROLLER_BUTTON_A:  // Cross
            chopStartTime = juce::Time::getMillisecondCounterHiRes();
            currentPosition = chopComponent->getCrossfaderValue();
            chopComponent->setCrossfaderValue(currentPosition <= 0.5f ? 1.0f : 0.0f);
            break;
        case SDL_CONTROLLER_BUTTON_B:  // Circle
            loadAudioFile();
            break;
        case SDL_CONTROLLER_BUTTON_X:  // Square
            stop();
            break;
        case SDL_CONTROLLER_BUTTON_Y:  // Triangle
            play();
            break;
    }
}

void MainComponent::gamepadButtonReleased(int buttonId)
{
    if (buttonId == SDL_CONTROLLER_BUTTON_A)  // Cross
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
    }
}

void MainComponent::gamepadAxisMoved(int axisId, float value)
{
    // Handle left stick for crossfader
    if (axisId == SDL_CONTROLLER_AXIS_LEFTX)
    {
        // Map -1.0 to 1.0 to 0.0 to 1.0
        float crossfaderValue = (value + 1.0f) * 0.5f;
        chopComponent->setCrossfaderValue(crossfaderValue);
    }
}

void MainComponent::updatePositionLabel()
{
    auto& transport = edit.getTransport();
    auto position = transport.getPosition();
    
    // Get time position
    auto timeString = PlayHeadHelpers::timeToTimecodeString(position.inSeconds());
    
    // Get beat position
    auto& tempoSequence = edit.tempoSequence;
    auto beatPosition = tempoSequence.toBarsAndBeats(position);
    auto quarterNotes = beatPosition.getTotalBars() * 4.0; // Convert bars to quarter notes
    auto beatString = PlayHeadHelpers::quarterNotePositionToBarsBeatsString(
        quarterNotes,
        tempoSequence.getTimeSigAt(position).numerator,
        tempoSequence.getTimeSigAt(position).denominator
    );
    
    positionLabel.setText(timeString + " | " + beatString, juce::dontSendNotification);
}

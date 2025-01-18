#include "MainComponent.h"

//==============================================================================
MainComponent::MainComponent()
{
    // Add this near the start of the constructor, before other component setup
    currentTrackLabel.setJustificationType(juce::Justification::centred);
    currentTrackLabel.setText("No Track Loaded", juce::dontSendNotification);
    currentTrackLabel.setFont(juce::Font(16.0f));
    currentTrackLabel.setMinimumHorizontalScale(1.0f);
    addAndMakeVisible(currentTrackLabel);

    // Register our custom plugin with the engine
    // engine.getPluginManager().createBuiltInType<DistortionPlugin>();

    addAndMakeVisible(openButton);
    addAndMakeVisible(saveButton);
    addAndMakeVisible(playButton);
    addAndMakeVisible(stopButton);
    addAndMakeVisible(tempoSlider);
    addAndMakeVisible(crossfaderSlider);
    addAndMakeVisible(recordButton);
    addAndMakeVisible(tempo70Button);
    addAndMakeVisible(tempo75Button);
    addAndMakeVisible(tempo80Button);
    addAndMakeVisible(tempo85Button);
    addAndMakeVisible(tempo100Button);
    addAndMakeVisible(chopButton);
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

    setSize(800, 600);

    playButton.setToggleState(false, juce::NotificationType::dontSendNotification);
    stopButton.setToggleState(true, juce::NotificationType::dontSendNotification);

    playButton.onClick = [this]
    { play(); };
    stopButton.onClick = [this]
    { stop(); };
    openButton.onClick = [this]
    { loadAudioFile(); };

    tempoSlider.setRange(30.0, 220.0, 0.1);
    tempoSlider.setTextValueSuffix(" BPM");
    tempoSlider.onValueChange = [this] { updateTempo(); };

    // Setup crossfader
    crossfaderSlider.setName("Crossfader");
    crossfaderSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    crossfaderSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    crossfaderSlider.setRange(0.0, 1.0, 0.01);
    crossfaderSlider.setValue(0.0, juce::dontSendNotification);
    crossfaderSlider.onValueChange = [this] { updateCrossfader(); };
    crossfaderSlider.setPopupDisplayEnabled(true, false, this);
    crossfaderSlider.setPopupMenuEnabled(false);

    // Setup reverb control
    reverbComponent = std::make_unique<ReverbComponent>(edit);
    addAndMakeVisible(*reverbComponent);

    // Configure tempo preset buttons (add after other button configurations)
    tempo70Button.setButtonText("70%");
    tempo75Button.setButtonText("75%");
    tempo80Button.setButtonText("80%");
    tempo85Button.setButtonText("85%");
    tempo100Button.setButtonText("100%");

    tempo70Button.onClick = [this]
    { setTempoPercentage(0.70); };
    tempo75Button.onClick = [this]
    { setTempoPercentage(0.75); };
    tempo80Button.onClick = [this]
    { setTempoPercentage(0.80); };
    tempo85Button.onClick = [this]
    { setTempoPercentage(0.85); };
    tempo100Button.onClick = [this]
    { setTempoPercentage(1.00); };

    // setup effects

    recordButton.setToggleState(false, juce::NotificationType::dontSendNotification);
    recordButton.onClick = [this]
    {
        if (edit.getTransport().isRecording())
            stopRecording();
        else
            startRecording();
    };

    // Setup track offset label
    trackOffsetLabel.setJustificationType(juce::Justification::centred);
    updateTrackOffsetLabel(trackOffset);
    addAndMakeVisible(trackOffsetLabel);

    chopButton.addMouseListener(this, false);
    chopButton.setButtonText(chopButton.getButtonText()); // Trigger text update
    getLookAndFeel().setDefaultSansSerifTypefaceName("Arial");
    chopButton.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
    chopButton.setColour(juce::TextButton::buttonColourId, juce::Colours::darkred);

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
    resized();

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
    }

    if (auto track2 = EngineHelpers::getOrInsertAudioTrackAt(edit, 1))
    {
        EngineHelpers::removeAllClips(*track2);
        volumeAndPan2 = dynamic_cast<te::VolumeAndPanPlugin*>(track2->pluginList.insertPlugin(te::VolumeAndPanPlugin::create(), 0).get());
    }

    vinylBrakeComponent = std::make_unique<VinylBrakeComponent>(edit);
    addAndMakeVisible(*vinylBrakeComponent);
}

MainComponent::~MainComponent()
{
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

    // Row 1: Thumbnail (about 1/3 of height)
    mainColumn.items.add(juce::FlexItem(*thumbnail).withFlex(1.0f).withMargin(5));

    // Row 2: Control Bar
    juce::FlexBox controlBarBox;
    controlBarBox.flexDirection = juce::FlexBox::Direction::row;
    controlBarBox.justifyContent = juce::FlexBox::JustifyContent::flexStart;
    controlBarBox.alignItems = juce::FlexBox::AlignItems::center;

    // Track label with border - modified to take full width
    juce::FlexBox trackLabelBox;
    trackLabelBox.flexDirection = juce::FlexBox::Direction::row;
    trackLabelBox.items.add(juce::FlexItem(currentTrackLabel).withFlex(1.0f).withHeight(30).withMargin(2));

    // Transport controls
    juce::FlexBox transportBox;
    transportBox.flexDirection = juce::FlexBox::Direction::row;
    transportBox.items.add(juce::FlexItem(playButton).withWidth(80).withHeight(30).withMargin(2));
    transportBox.items.add(juce::FlexItem(stopButton).withWidth(80).withHeight(30).withMargin(2));
    transportBox.items.add(juce::FlexItem(recordButton).withWidth(80).withHeight(30).withMargin(2));

    // Modified to make trackLabelBox take remaining space
    controlBarBox.items.add(juce::FlexItem(trackLabelBox).withFlex(1.0f));
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

    // Tempo buttons row
    juce::FlexBox tempoButtonBox;
    tempoButtonBox.flexDirection = juce::FlexBox::Direction::row;
    tempoButtonBox.items.add(juce::FlexItem(tempo70Button).withFlex(1.0f).withMargin(2));
    tempoButtonBox.items.add(juce::FlexItem(tempo75Button).withFlex(1.0f).withMargin(2));
    tempoButtonBox.items.add(juce::FlexItem(tempo80Button).withFlex(1.0f).withMargin(2));
    tempoButtonBox.items.add(juce::FlexItem(tempo85Button).withFlex(1.0f).withMargin(2));
    tempoButtonBox.items.add(juce::FlexItem(tempo100Button).withFlex(1.0f).withMargin(2));

    column2.items.add(juce::FlexItem(tempoButtonBox).withHeight(30).withMargin(5));
    column2.items.add(juce::FlexItem(tempoSlider).withHeight(30).withMargin(5));
    column2.items.add(juce::FlexItem(trackOffsetLabel).withHeight(30).withMargin(5));
    column2.items.add(juce::FlexItem(crossfaderSlider).withHeight(40).withMargin(5));
    column2.items.add(juce::FlexItem(chopButton).withHeight(30).withMargin(5));

    // Column 3 (Effects)
    juce::FlexBox column3;
    column3.flexDirection = juce::FlexBox::Direction::column;
    column3.items.add(juce::FlexItem(*reverbComponent).withHeight(100).withMargin(5));
    column3.items.add(juce::FlexItem(*flangerComponent).withHeight(100).withMargin(5));
    column3.items.add(juce::FlexItem(*vinylBrakeComponent).withHeight(100).withMargin(5));

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
        // Create SoundTouch BPM detector
        soundtouch::BPMDetect bpmDetector(2, 48000); // Assuming stereo, 48kHz

        // Load audio file and get samples
        juce::AudioFormatManager formatManager;
        formatManager.registerBasicFormats();

        std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file));
        if (reader)
        {
            // Read audio data
            const int numSamples = static_cast<int>(reader->lengthInSamples);
            juce::AudioBuffer<float> buffer(reader->numChannels, numSamples);
            reader->read(&buffer, 0, numSamples, 0, true, true);

            // Process samples through BPM detector
            bpmDetector.inputSamples(buffer.getReadPointer(0), numSamples);

            // Get detected BPM
            float detectedBPM = bpmDetector.getBpm();
            DBG("Detected BPM: " << detectedBPM);
            if (detectedBPM > 0)
            {
                baseTempo = detectedBPM;
                trackOffset = (60.0 / baseTempo) * 1000.0; // Convert to milliseconds
                tempoSlider.setValue(baseTempo, juce::dontSendNotification);
                updateTrackOffsetLabel(trackOffset);
            }
            else
            {
                baseTempo = 120.0;                         // fallback value
                trackOffset = (60.0 / baseTempo) * 1000.0; // Convert to milliseconds
                tempoSlider.setValue(baseTempo, juce::dontSendNotification);
                updateTrackOffsetLabel(trackOffset);
            }
        }

        // Disable auto tempo and pitch for first clip
        clip1->setAutoTempo(false);
        clip1->setAutoPitch(false);
        clip1->setTimeStretchMode(te::TimeStretcher::defaultMode);

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
                                                    {{tracktion::TimePosition::fromSeconds(trackOffset / 1000.0), // Convert ms to seconds
                                                      tracktion::TimeDuration::fromSeconds(clip1->getSourceLength().inSeconds())},
                                                     {}},
                                                    false))
            {
                // Configure second clip
                clip2->setAutoTempo(false);
                clip2->setAutoPitch(false);
                clip2->setTimeStretchMode(te::TimeStretcher::defaultMode);
                clip2->setGainDB(0.0f);
            }
        }

        // Reset crossfader to first track
        crossfaderSlider.setValue(0.0, juce::dontSendNotification);
        updateCrossfader();
    }

    updateButtonStates();
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
        
        // Update clip position based on new track offset
        trackOffset = (60.0 / tempoSlider.getValue()) * 1000.0; // Convert to milliseconds
        clip2->setStart(tracktion::TimePosition::fromSeconds(trackOffset / 1000.0), false, true);
    }

    // Update beat duration based on new tempo
    updateTrackOffsetLabel(trackOffset);
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
    const float position = crossfaderSlider.getValue();
    // Convert linear position (0.0 to 1.0) to decibels (-20dB to 0dB)
    // Track 1 volume goes from 0dB to -20dB
    setTrackVolume(0, position <= 0.0f ? 0.0f : -20.0f * position);
    // Track 2 volume goes from -20dB to 0dB
    setTrackVolume(1, position >= 1.0f ? 0.0f : -20.0f * (1.0f - position));
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

void MainComponent::updateTrackOffsetLabel(double offset)
{
    trackOffsetLabel.setText("Beat Duration: " + juce::String(offset, 1) + " ms", juce::dontSendNotification);
}

void MainComponent::setTempoPercentage(double percentage)
{
    // Update slider value which will trigger the slider's callback
    tempoSlider.setValue(baseTempo * percentage, juce::sendNotification);
    
}

void MainComponent::gamepadButtonPressed(int buttonId)
{
    float currentPosition;  // Moved outside switch
    
    switch (buttonId)
    {
        case SDL_CONTROLLER_BUTTON_A:  // Cross
            chopStartTime = juce::Time::getMillisecondCounterHiRes();
            currentPosition = crossfaderSlider.getValue();
            crossfaderSlider.setValue(currentPosition <= 0.5f ? 1.0f : 0.0f, juce::sendNotification);
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
            float currentPosition = crossfaderSlider.getValue();
            crossfaderSlider.setValue(currentPosition <= 0.5f ? 1.0f : 0.0f, juce::sendNotification);
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
        crossfaderSlider.setValue(crossfaderValue, juce::sendNotification);
    }
}
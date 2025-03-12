#include "TransportBar.h"

TransportBar::TransportBar(tracktion::engine::Edit& e, ZoomState& zs)
    : edit(e),
      transport(e.getTransport()),
      zoomState(zs)
{
    // Add and make visible all buttons
    addAndMakeVisible(playButton);
    addAndMakeVisible(stopButton);
    addAndMakeVisible(timeDisplay);
    addAndMakeVisible(zoomInButton);
    addAndMakeVisible(zoomOutButton);
    addAndMakeVisible(automationWriteButton);
    addAndMakeVisible(automationReadButton);
    addAndMakeVisible(snapButton);
    addAndMakeVisible(gridSizeComboBox);

    // Listen to transport changes
    edit.getTransport().addChangeListener(this);
    
    // Get colors from the look and feel
    const auto primary = juce::Colour(0xFF505050);      // Medium gray
    const auto primaryVariant = juce::Colour(0xFF404040); // Darker gray
    const auto secondary = juce::Colour(0xFF707070);    // Light gray accent
    const auto surface = juce::Colour(0xFF1E1E1E);      // Surface color
    const auto textPrimary = juce::Colours::white;      // White text
    
    // Set up play and stop button shapes with modern styling
    playButton.setShape(getPlayPath(), false, true, false);
    playButton.setOutline(secondary, 1.0f);
    playButton.setColours(textPrimary, primary, primaryVariant);
    playButton.setClickingTogglesState(true);

    stopButton.setShape(getStopPath(), false, true, false);
    stopButton.setOutline(secondary, 1.0f);
    stopButton.setColours(textPrimary, primary, primaryVariant);

    // Setup grid size combo box with modern styling
    gridSizeComboBox.addItem("1/16", 1);  // 0.0625
    gridSizeComboBox.addItem("1/8", 2);   // 0.125
    gridSizeComboBox.addItem("1/4", 3);   // 0.25
    gridSizeComboBox.addItem("1/2", 4);   // 0.5
    gridSizeComboBox.addItem("1/1", 5);   // 1.0
    
    // Set initial grid size based on ZoomState
    float currentGridSize = zoomState.getGridSize();
    int itemId = 3; // Default to 1/4
    if (currentGridSize <= 0.0625f) itemId = 1;
    else if (currentGridSize <= 0.125f) itemId = 2;
    else if (currentGridSize <= 0.25f) itemId = 3;
    else if (currentGridSize <= 0.5f) itemId = 4;
    else itemId = 5;
    
    gridSizeComboBox.setSelectedId(itemId, juce::dontSendNotification);
    gridSizeComboBox.setColour(juce::ComboBox::backgroundColourId, surface);
    gridSizeComboBox.setColour(juce::ComboBox::textColourId, textPrimary);
    gridSizeComboBox.setColour(juce::ComboBox::outlineColourId, primary);
    
    gridSizeComboBox.onChange = [this]() {
        float newGridSize = 1.0f; // Default
        switch (gridSizeComboBox.getSelectedId()) {
            case 1: newGridSize = 0.0625f; break; // 1/16
            case 2: newGridSize = 0.125f; break;  // 1/8
            case 3: newGridSize = 0.25f; break;   // 1/4
            case 4: newGridSize = 0.5f; break;    // 1/2
            case 5: newGridSize = 1.0f; break;    // 1/1
        }
        zoomState.setGridSize(newGridSize);
    };

    // Listen to zoom state changes
    zoomState.addListener(this);
    
    // Setup snap button with modern styling
    snapButton.setClickingTogglesState(true);
    snapButton.setToggleState(true, juce::dontSendNotification);
    snapButton.setColour(juce::TextButton::buttonColourId, primary);
    snapButton.setColour(juce::TextButton::buttonOnColourId, primaryVariant);
    snapButton.setColour(juce::TextButton::textColourOffId, textPrimary);
    snapButton.setColour(juce::TextButton::textColourOnId, textPrimary);
    snapButton.onClick = [this]() {
        bool snapEnabled = snapButton.getToggleState();
        if (onSnapStateChanged)
            onSnapStateChanged(snapEnabled);
    };

    // Setup button callbacks
    playButton.onClick = [this] {
        auto& tempoSequence = edit.tempoSequence;

        if (transport.isPlaying())
            transport.stop(false, false);
        else
        {
            // Ensure we're synced to tempo before playing
            auto position = createPosition(tempoSequence);
            position.set(transport.getPosition());

            // Update transport to sync with tempo
            transport.setPosition(position.getTime());
            transport.play(false);
        }
        updateTransportState();
    };

    stopButton.onClick = [this] {
        transport.stop(false, false);
        transport.setPosition(tracktion::TimePosition::fromSeconds(0.0));
        updateTransportState();
    };

    loopButton.setClickingTogglesState(true);
    loopButton.onClick = [this] {
        transport.looping = loopButton.getToggleState();
        updateTransportState();
    };

    // Initialize time display with modern styling
    timeDisplay.setJustificationType(juce::Justification::centred);
    timeDisplay.setFont(CustomLookAndFeel::getMonospaceFont().withHeight(18.0f));
    timeDisplay.setColour(juce::Label::textColourId, textPrimary);
    timeDisplay.setColour(juce::Label::backgroundColourId, surface);
    updateTimeDisplay();

    // Set up automation buttons with modern styling
    automationReadButton.setClickingTogglesState(true);
    automationWriteButton.setClickingTogglesState(true);
    
    // Set up automation read button with sine wave shape
    automationReadButton.setShape(getAutomationPath(), false, true, false);
    automationReadButton.setColours(
        textPrimary.withAlpha(0.7f),     // normal
        primary,                          // over
        primaryVariant                    // down
    );
    automationReadButton.setOutline(secondary, 1.0f);
    
    // Setup automation write button with record circle
    automationWriteButton.setShape(getRecordPath(), false, true, false);
    automationWriteButton.setColours(
        juce::Colours::red.withAlpha(0.7f),  // normal
        juce::Colours::red,                   // over
        juce::Colours::red.darker(0.2f)       // down
    );
    automationWriteButton.setOutline(secondary, 1.0f);
    
    automationReadButton.setToggleState(edit.getAutomationRecordManager().isReadingAutomation(), juce::dontSendNotification);
    automationWriteButton.setToggleState(edit.getAutomationRecordManager().isWritingAutomation(), juce::dontSendNotification);
    
    automationReadButton.onClick = [this] {
        edit.getAutomationRecordManager().setReadingAutomation(automationReadButton.getToggleState());
    };
    
    automationWriteButton.onClick = [this] {
        edit.getAutomationRecordManager().setWritingAutomation(automationWriteButton.getToggleState());
    };

    // Setup zoom buttons with modern styling
    zoomInButton.setColour(juce::TextButton::buttonColourId, primary);
    zoomInButton.setColour(juce::TextButton::buttonOnColourId, primaryVariant);
    zoomInButton.setColour(juce::TextButton::textColourOffId, textPrimary);
    zoomInButton.setColour(juce::TextButton::textColourOnId, textPrimary);
    
    zoomOutButton.setColour(juce::TextButton::buttonColourId, primary);
    zoomOutButton.setColour(juce::TextButton::buttonOnColourId, primaryVariant);
    zoomOutButton.setColour(juce::TextButton::textColourOffId, textPrimary);
    zoomOutButton.setColour(juce::TextButton::textColourOnId, textPrimary);
    
    // Add zoom button callbacks
    zoomInButton.onClick = [this] {
        zoomState.setZoomLevel(zoomState.getZoomLevel() * 1.5);
    };
    
    zoomOutButton.onClick = [this] {
        zoomState.setZoomLevel(zoomState.getZoomLevel() / 1.5);
    };

    startTimerHz(30);
}

TransportBar::~TransportBar()
{
    stopTimer();
    
    if (&edit != nullptr && &(edit.getTransport()) != nullptr)
    {
        edit.getTransport().removeChangeListener(this);
    }
    
    zoomState.removeListener(this);
}

void TransportBar::resized()
{
    auto bounds = getLocalBounds();
    
    const int controlSpacing = 4;  // Spacing between controls
    const int verticalPadding = 2; // Vertical padding for control bar
    
    // Adjust bounds for vertical padding
    bounds = bounds.reduced(0, verticalPadding);
    
    // Calculate widths for time display and grid control
    const int timeDisplayWidth = bounds.getWidth() * 0.3;  // 30% for time display
    const int gridControlWidth = bounds.getWidth() * 0.1;  // 10% for grid control
    
    // Create FlexBox for button layout
    juce::FlexBox buttonFlex;
    buttonFlex.flexDirection = juce::FlexBox::Direction::row;
    buttonFlex.justifyContent = juce::FlexBox::JustifyContent::flexStart;
    buttonFlex.alignItems = juce::FlexBox::AlignItems::center;
    
    // Calculate remaining width for buttons
    const int remainingWidth = bounds.getWidth() - timeDisplayWidth - gridControlWidth;
    const int buttonWidth = (remainingWidth - (controlSpacing * 6)) / 7; // 7 controls, 6 spaces
    
    // Make transport buttons slightly smaller
    const float buttonSizeReduction = 0.85f; // Reduce button size by 15%
    const int transportButtonWidth = static_cast<int>(buttonWidth * buttonSizeReduction);
    const int transportButtonHeight = static_cast<int>(bounds.getHeight() * buttonSizeReduction);
    
    // Add buttons to flex layout with spacing, making transport-related buttons smaller
    buttonFlex.items.add(juce::FlexItem(transportButtonWidth, transportButtonHeight, playButton)
        .withMargin(juce::FlexItem::Margin(0, controlSpacing, 0, 0))
        .withAlignSelf(juce::FlexItem::AlignSelf::center));
    
    buttonFlex.items.add(juce::FlexItem(transportButtonWidth, transportButtonHeight, stopButton)
        .withMargin(juce::FlexItem::Margin(0, controlSpacing, 0, 0))
        .withAlignSelf(juce::FlexItem::AlignSelf::center));
    
    buttonFlex.items.add(juce::FlexItem(transportButtonWidth, transportButtonHeight, automationWriteButton)
        .withMargin(juce::FlexItem::Margin(0, controlSpacing, 0, 0))
        .withAlignSelf(juce::FlexItem::AlignSelf::center));
    
    buttonFlex.items.add(juce::FlexItem(transportButtonWidth, transportButtonHeight, automationReadButton)
        .withMargin(juce::FlexItem::Margin(0, controlSpacing, 0, 0))
        .withAlignSelf(juce::FlexItem::AlignSelf::center));
    
    // Keep other buttons at original size
    buttonFlex.items.add(juce::FlexItem(buttonWidth, bounds.getHeight(), zoomInButton)
        .withMargin(juce::FlexItem::Margin(0, controlSpacing, 0, 0)));
    
    buttonFlex.items.add(juce::FlexItem(buttonWidth, bounds.getHeight(), zoomOutButton)
        .withMargin(juce::FlexItem::Margin(0, controlSpacing, 0, 0)));
    
    buttonFlex.items.add(juce::FlexItem(buttonWidth, bounds.getHeight(), snapButton)
        .withMargin(juce::FlexItem::Margin(0, controlSpacing, 0, 0)));
    
    // Create area for buttons
    auto buttonArea = bounds.removeFromLeft(remainingWidth);
    buttonFlex.performLayout(buttonArea);
    
    // Layout grid control and time display
    gridSizeComboBox.setBounds(bounds.removeFromLeft(gridControlWidth));
    timeDisplay.setBounds(bounds);
}

void TransportBar::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    if (source == &(edit.getTransport()))
    {
        updateTransportState();
        snapButton.setToggleState(transport.snapToTimecode, juce::dontSendNotification);
        repaint();
    }
}

void TransportBar::timerCallback()
{
    updateTimeDisplay();
}

void TransportBar::updateTimeDisplay()
{
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
                            minutes,
                            (int)seconds % 60,
                            millis,
                            timeSignature.numerator,
                            timeSignature.denominator,
                            barsBeats.bars + 1,
                            tempo),
        juce::dontSendNotification);
}

void TransportBar::updateTransportState()
{
    bool isPlaying = transport.isPlaying();
    playButton.setToggleState(isPlaying, juce::dontSendNotification);
    playButton.setShape(isPlaying ? getPausePath() : getPlayPath(), false, true, false);
    loopButton.setToggleState(transport.looping, juce::dontSendNotification);
}

void TransportBar::gridSizeChanged(float newGridSize)
{
    // Update combo box selection based on new grid size
    int itemId = 3; // Default to 1/4
    if (newGridSize <= 0.0625f) itemId = 1;
    else if (newGridSize <= 0.125f) itemId = 2;
    else if (newGridSize <= 0.25f) itemId = 3;
    else if (newGridSize <= 0.5f) itemId = 4;
    else itemId = 5;
    
    gridSizeComboBox.setSelectedId(itemId, juce::dontSendNotification);
} 
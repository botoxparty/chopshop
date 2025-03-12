#include "TransportBar.h"

TransportBar::TransportBar(tracktion::engine::Edit& e)
    : edit(e),
      transport(e.getTransport())
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
    
    // Set up play and stop button shapes
    playButton.setShape(getPlayPath(), false, true, false);
    playButton.setOutline(juce::Colours::white, 1.0f);
    playButton.setColours(juce::Colours::white, juce::Colours::lightgrey.darker(0.2f), juce::Colours::white.darker(0.2f));
    playButton.setClickingTogglesState(true);

    stopButton.setShape(getStopPath(), false, true, false);
    stopButton.setOutline(juce::Colours::white, 1.0f);
    stopButton.setColours(juce::Colours::white, juce::Colours::lightgrey.darker(0.2f), juce::Colours::white.darker(0.2f));

    // Setup grid size combo box
    gridSizeComboBox.addItem("1/16", 1);  // 0.0625
    gridSizeComboBox.addItem("1/8", 2);   // 0.125
    gridSizeComboBox.addItem("1/4", 3);   // 0.25
    gridSizeComboBox.addItem("1/2", 4);   // 0.5
    gridSizeComboBox.addItem("1/1", 5);     // 1.0
    gridSizeComboBox.setSelectedId(4);    // Default to 1/4

    // Setup snap button
    snapButton.setClickingTogglesState(true);
    snapButton.setToggleState(true, juce::dontSendNotification);
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

    // Initialize time display
    timeDisplay.setJustificationType(juce::Justification::centred);
    timeDisplay.setFont(CustomLookAndFeel::getMonospaceFont().withHeight(18.0f));
    timeDisplay.setColour(juce::Label::textColourId, juce::Colours::white);
    timeDisplay.setColour(juce::Label::backgroundColourId, juce::Colours::darkgrey.darker(0.7f));
    updateTimeDisplay();

    // Set up automation buttons
    automationReadButton.setClickingTogglesState(true);
    automationWriteButton.setClickingTogglesState(true);
    
    // Set up automation read button with sine wave shape
    automationReadButton.setShape(getAutomationPath(), false, true, false);
    automationReadButton.setColours(
        juce::Colours::white.withAlpha(0.7f),     // normal
        juce::Colours::green.darker(),            // over
        juce::Colours::green                      // down
    );
    automationReadButton.setOutline(juce::Colours::white, 1.0f);
    
    // Setup automation write button with record circle
    automationWriteButton.setShape(getRecordPath(), false, true, false);
    automationWriteButton.setColours(
        juce::Colours::red.withAlpha(0.7f),  // normal
        juce::Colours::red,                   // over
        juce::Colours::red.brighter(0.2f)     // down
    );
    automationWriteButton.setOutline(juce::Colours::transparentWhite, 0.0f);
    
    automationReadButton.setToggleState(edit.getAutomationRecordManager().isReadingAutomation(), juce::dontSendNotification);
    automationWriteButton.setToggleState(edit.getAutomationRecordManager().isWritingAutomation(), juce::dontSendNotification);
    
    automationReadButton.onClick = [this] {
        edit.getAutomationRecordManager().setReadingAutomation(automationReadButton.getToggleState());
    };
    
    automationWriteButton.onClick = [this] {
        edit.getAutomationRecordManager().setWritingAutomation(automationWriteButton.getToggleState());
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
    const int buttonWidth = (remainingWidth - (controlSpacing * 7)) / 8; // 9 controls, 8 spaces
    
    // Add buttons to flex layout with spacing
    buttonFlex.items.add(juce::FlexItem(buttonWidth, bounds.getHeight(), playButton).withMargin(juce::FlexItem::Margin(0, controlSpacing, 0, 0)));
    buttonFlex.items.add(juce::FlexItem(buttonWidth, bounds.getHeight(), stopButton).withMargin(juce::FlexItem::Margin(0, controlSpacing, 0, 0)));
    buttonFlex.items.add(juce::FlexItem(buttonWidth, bounds.getHeight(), automationWriteButton).withMargin(juce::FlexItem::Margin(0, controlSpacing, 0, 0)));
    buttonFlex.items.add(juce::FlexItem(buttonWidth, bounds.getHeight(), automationReadButton).withMargin(juce::FlexItem::Margin(0, controlSpacing, 0, 0)));
    buttonFlex.items.add(juce::FlexItem(buttonWidth, bounds.getHeight(), zoomInButton).withMargin(juce::FlexItem::Margin(0, controlSpacing, 0, 0)));
    buttonFlex.items.add(juce::FlexItem(buttonWidth, bounds.getHeight(), zoomOutButton).withMargin(juce::FlexItem::Margin(0, controlSpacing, 0, 0)));
    buttonFlex.items.add(juce::FlexItem(buttonWidth, bounds.getHeight(), snapButton).withMargin(juce::FlexItem::Margin(0, controlSpacing, 0, 0)));
    
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
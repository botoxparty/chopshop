#include "ControlBarComponent.h"

ControlBarComponent::ControlBarComponent(tracktion_engine::Edit& e)
    : edit(e)
{
    // Set up track label
    currentTrackLabel.setJustificationType(juce::Justification::centred);
    currentTrackLabel.setText("No Track Loaded", juce::dontSendNotification);
    currentTrackLabel.setFont(juce::FontOptions(18.0f));
    currentTrackLabel.setMinimumHorizontalScale(1.0f);
    currentTrackLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(currentTrackLabel);
    
    // Set up position label
    positionLabel.setJustificationType(juce::Justification::centred);
    positionLabel.setFont(CustomLookAndFeel::getMonospaceFont().withHeight(18.0f));
    positionLabel.setMinimumHorizontalScale(1.0f);
    positionLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(positionLabel);
    
    // Set up buttons with custom styling
    setupButton(playButton, "Play", juce::Colours::green.withAlpha(0.7f));
    setupButton(stopButton, "Stop", juce::Colours::red.withAlpha(0.7f));
    
    // Set initial button states
    playButton.setToggleState(false, juce::NotificationType::dontSendNotification);
    stopButton.setToggleState(true, juce::NotificationType::dontSendNotification);
    
    // Set up button callbacks
    playButton.onClick = [this] {
        if (onPlayButtonClicked)
            onPlayButtonClicked();
    };
    
    stopButton.onClick = [this] {
        if (onStopButtonClicked)
            onStopButtonClicked();
    };
    
    // Start the timer for animation effects
    startTimerHz(30);
}

ControlBarComponent::~ControlBarComponent()
{
    stopTimer();
}

void ControlBarComponent::setupButton(juce::TextButton& button, const juce::String& text, juce::Colour baseColour)
{
    button.setButtonText(text);
    button.setColour(juce::TextButton::buttonColourId, baseColour);
    button.setColour(juce::TextButton::buttonOnColourId, baseColour.brighter(0.3f));
    button.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    button.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
    addAndMakeVisible(button);
}

void ControlBarComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    
    // Draw gradient background
    juce::ColourGradient gradient(
        juce::Colour(0xFF1E1E1E), bounds.getX(), bounds.getY(),
        juce::Colour(0xFF2D2D2D), bounds.getX(), bounds.getBottom(),
        false);
    g.setGradientFill(gradient);
    g.fillRoundedRectangle(bounds, 4.0f);
    
    // Draw border
    g.setColour(juce::Colour(0xFF3A3A3A));
    g.drawRoundedRectangle(bounds.reduced(0.5f), 4.0f, 1.0f);
    
    // Draw playback position indicator
    if (playButton.getToggleState()) {
        // Draw a pulsing highlight around the position label
        auto positionBounds = positionLabel.getBounds().toFloat().expanded(4.0f);
        float alpha = 0.2f + 0.1f * std::sin(pulsePhase);
        g.setColour(juce::Colours::lightblue.withAlpha(alpha));
        g.drawRoundedRectangle(positionBounds, 4.0f, 1.5f);
    }
}

void ControlBarComponent::resized()
{
    auto bounds = getLocalBounds().reduced(4);
    
    // Create control bar layout
    juce::FlexBox controlBarBox;
    controlBarBox.flexDirection = juce::FlexBox::Direction::row;
    controlBarBox.justifyContent = juce::FlexBox::JustifyContent::spaceBetween;
    controlBarBox.alignItems = juce::FlexBox::AlignItems::center;
    controlBarBox.alignContent = juce::FlexBox::AlignContent::center;

    // Track label with border - modified to take full width
    controlBarBox.items.add(juce::FlexItem(currentTrackLabel)
                           .withFlex(1.0f)
                           .withHeight(24)
                           .withMargin(juce::FlexItem::Margin(0, 5, 0, 0))
                           .withAlignSelf(juce::FlexItem::AlignSelf::center));

    // Position label - increased right margin from 5 to 15 pixels
    controlBarBox.items.add(juce::FlexItem(positionLabel)
                           .withWidth(200)
                           .withHeight(24)
                           .withMargin(juce::FlexItem::Margin(0, 15, 0, 5))
                           .withAlignSelf(juce::FlexItem::AlignSelf::center));

    // Transport controls - now directly in the main FlexBox
    controlBarBox.items.add(juce::FlexItem(playButton)
                           .withWidth(70)
                           .withHeight(28)
                           .withMargin(juce::FlexItem::Margin(0, 3, 0, 0))
                           .withAlignSelf(juce::FlexItem::AlignSelf::center));
                           
    controlBarBox.items.add(juce::FlexItem(stopButton)
                           .withWidth(70)
                           .withHeight(28)
                           .withMargin(juce::FlexItem::Margin(0, 15, 0, 0))
                           .withAlignSelf(juce::FlexItem::AlignSelf::center));

    // Perform layout
    controlBarBox.performLayout(bounds);
}

void ControlBarComponent::updatePositionLabel()
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
        tempoSequence.getTimeSigAt(position).denominator);

    // Display both time and beat information
    positionLabel.setText(timeString + " | " + beatString, juce::dontSendNotification);
}

void ControlBarComponent::setPlayButtonState(bool isPlaying)
{
    playButton.setToggleState(isPlaying, juce::NotificationType::dontSendNotification);
    playButton.setButtonText(isPlaying ? "Pause" : "Play");
    
    // Update button colors based on state
    playButton.setColour(juce::TextButton::buttonColourId, 
                         isPlaying ? juce::Colours::orange.withAlpha(0.7f) : juce::Colours::green.withAlpha(0.7f));
    
    repaint(); // Ensure visual update
}

void ControlBarComponent::setStopButtonState(bool isStopped)
{
    stopButton.setToggleState(isStopped, juce::NotificationType::dontSendNotification);
    
    // Update button colors based on state
    stopButton.setColour(juce::TextButton::buttonColourId, 
                         isStopped ? juce::Colours::red.withAlpha(0.9f) : juce::Colours::red.withAlpha(0.5f));
    
    repaint(); // Ensure visual update
}

void ControlBarComponent::setTrackName(const juce::String& name)
{
    currentTrackLabel.setText(name, juce::dontSendNotification);
}

void ControlBarComponent::timerCallback()
{
    // Update pulse phase for animations
    pulsePhase += 0.1f;
    if (pulsePhase > juce::MathConstants<float>::twoPi)
        pulsePhase -= juce::MathConstants<float>::twoPi;
    
    // Only repaint if playing (for animation effects)
    if (playButton.getToggleState())
        repaint();
} 
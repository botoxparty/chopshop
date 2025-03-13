#include "ChopComponent.h"
#include "Plugins/ChopPlugin.h"
#include "Utilities.h"

ChopComponent::ChopComponent (tracktion::engine::Edit& editIn)
    : BaseEffectComponent (editIn)
{
    chopTrack = EngineHelpers::getChopTrack(edit);

    if (!chopTrack)
    {
        titleLabel.setText ("Error loading Chop", juce::dontSendNotification);
        DBG ("Error: No chop track found");
        return;
    }

    titleLabel.setText ("Chop Controls", juce::dontSendNotification);

    // Configure labels
    durationLabel.setText ("Duration", juce::dontSendNotification);
    durationLabel.setJustificationType (juce::Justification::left);

    // Configure combo box
    chopDurationComboBox.addItem ("1/4 Beat", 1);
    chopDurationComboBox.addItem ("1/2 Beat", 2);
    chopDurationComboBox.addItem ("1 Beat", 3);
    chopDurationComboBox.addItem ("2 Beats", 4);
    chopDurationComboBox.addItem ("4 Beats", 5);
    chopDurationComboBox.setSelectedId (3, juce::dontSendNotification);

    // Configure chop button
    chopButton.setColour (juce::TextButton::textColourOnId, juce::Colours::white);
    chopButton.setColour (juce::TextButton::buttonColourId, juce::Colours::darkred);
    chopButton.setButtonText("CHOP");
    chopButton.onClick = [this]() { handleChopButtonPressed(); };

    // Add components to the content component
    contentComponent.addAndMakeVisible (durationLabel);
    contentComponent.addAndMakeVisible (chopDurationComboBox);
    contentComponent.addAndMakeVisible (chopButton);

    // Make sure this component can receive keyboard focus
    setWantsKeyboardFocus (true);
}

ChopComponent::~ChopComponent()
{
    stopTimer(); // Make sure to stop the timer
    if (commandManager != nullptr)
        commandManager->removeListener (this);
}

void ChopComponent::setCommandManager (juce::ApplicationCommandManager* manager)
{
    // Remove from previous manager if exists
    if (commandManager != nullptr)
        commandManager->removeListener (this);

    commandManager = manager;

    if (commandManager != nullptr)
    {
        // Register commands with the manager
        commandManager->registerAllCommandsForTarget (this);

        // Add as key listener
        addKeyListener (commandManager->getKeyMappings());

        // Register the space key for the chop command
        commandManager->getKeyMappings()->addKeyPress (CommandIDs::chopEffect, juce::KeyPress (juce::KeyPress::spaceKey));
    }
}

void ChopComponent::resized()
{
    // First let the base component handle its layout
    BaseEffectComponent::resized();

    auto bounds = contentComponent.getLocalBounds();

    // Layout dimensions
    const int labelHeight = 24;
    const int comboBoxHeight = 36;  // Taller height specifically for the ComboBox
    const int controlWidth = 120;
    const int spacing = 12;

    // Split the bounds into left and right sections
    auto leftSection = bounds.removeFromLeft(controlWidth);
    bounds.removeFromLeft(spacing / 2); // Add spacing between sections

    // Left section: Label and ComboBox vertically
    auto labelSection = leftSection.removeFromTop(labelHeight);
    leftSection.removeFromTop(4); // Add spacing between label and combo
    auto comboSection = leftSection.removeFromTop(comboBoxHeight);

    durationLabel.setBounds(labelSection);
    chopDurationComboBox.setBounds(comboSection);

    // Right section: Button (takes remaining space)
    chopButton.setBounds(bounds);
}

double ChopComponent::getChopDurationInBeats() const
{
    juce::String description = chopDurationComboBox.getText();
    if (description == "1/4 Beat")
        return 0.25;
    else if (description == "1/2 Beat")
        return 0.5;
    else if (description == "1 Beat")
        return 1.0;
    else if (description == "2 Beats")
        return 2.0;
    else if (description == "4 Beats")
        return 4.0;

    return 1.0; // Default to 1 beat
}

void ChopComponent::handleChopButtonPressed()
{
    if (!chopTrack)
        return;

    auto& transport = edit.getTransport();
    double currentPositionSeconds = transport.getPosition().inSeconds();
    
    // Convert duration from beats to time
    auto durationInBeats = getChopDurationInBeats();
    auto startBeat = edit.tempoSequence.toBeats(tracktion::TimePosition::fromSeconds(currentPositionSeconds));
    auto endBeat = tracktion::BeatPosition::fromBeats(startBeat.inBeats() + durationInBeats);
    auto endTime = edit.tempoSequence.toTime(endBeat).inSeconds();

    // Create a new clip at the current position
    auto timeRange = tracktion::TimeRange::between(
        tracktion::TimePosition::fromSeconds(currentPositionSeconds),
        tracktion::TimePosition::fromSeconds(endTime)
    );

    auto newClip = chopTrack->insertNewClip(tracktion::engine::TrackItem::Type::arranger, timeRange, nullptr);
    if (newClip != nullptr)
    {
        newClip->setName("Chop " + juce::String(chopTrack->getClips().size()));
        DBG("Created new chop clip from " + juce::String(currentPositionSeconds) + " to " + juce::String(endTime));
    }
}

void ChopComponent::handleChopButtonReleased()
{
    // Nothing needed here as clips are created on press
}

void ChopComponent::timerCallback()
{
    stopTimer();
    chopReleaseDelay = 0;
}

// Implement ApplicationCommandTarget methods
juce::ApplicationCommandTarget* ChopComponent::getNextCommandTarget()
{
    return findFirstTargetParentComponent();
}

void ChopComponent::getAllCommands (juce::Array<juce::CommandID>& commands)
{
    commands.add (CommandIDs::chopEffect); // Chop command
}

void ChopComponent::getCommandInfo (juce::CommandID commandID, juce::ApplicationCommandInfo& result)
{
    if (commandID == CommandIDs::chopEffect) // Chop command
    {
        result.setInfo ("Chop", "Activates the chop effect", "Chop", 0);
        result.addDefaultKeypress (juce::KeyPress::spaceKey, 0);

        // This is the key line - tell the command manager we want key up/down callbacks
        result.flags |= juce::ApplicationCommandInfo::wantsKeyUpDownCallbacks;
    }
}

bool ChopComponent::perform (const juce::ApplicationCommandTarget::InvocationInfo& info)
{
    if (info.commandID == CommandIDs::chopEffect) // Chop command
    {
        if (info.isKeyDown)
        {
            handleChopButtonPressed();
        }
        else
        {
            handleChopButtonReleased();
        }
        return true;
    }

    return false;
}
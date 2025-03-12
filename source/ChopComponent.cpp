#include "ChopComponent.h"
#include "Plugins/ChopPlugin.h"

ChopComponent::ChopComponent (tracktion::engine::Edit& editIn)
    : BaseEffectComponent (editIn)
{
    plugin = EngineHelpers::getPluginFromMasterTrack (edit, ChopPlugin::xmlTypeName);

    if (!plugin)
    {
        titleLabel.setText ("Error loading Chop", juce::dontSendNotification);
        DBG ("Error: No chop plugin found");
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
    chopButton.addMouseListener (this, false);

    // Add components to the content component instead of the base
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

double ChopComponent::getChopDurationInMs (double currentTempo) const
{
    double beatDuration = (60.0 / currentTempo) * 1000.0; // One beat duration in ms

    juce::String description = chopDurationComboBox.getText();
    if (description == "1/4 Beat")
        return beatDuration * 0.25;
    else if (description == "1/2 Beat")
        return beatDuration * 0.5;
    else if (description == "1 Beat")
        return beatDuration;
    else if (description == "2 Beats")
        return beatDuration * 2.0;
    else if (description == "4 Beats")
        return beatDuration * 4.0;

    return beatDuration; // Default to 1 beat
}

void ChopComponent::mouseDown (const juce::MouseEvent& event)
{
    if (event.eventComponent == &chopButton)
    {
        DBG ("Mouse down on chop button");
        handleChopButtonPressed();
    }
}

void ChopComponent::mouseUp (const juce::MouseEvent& event)
{
    if (event.eventComponent == &chopButton)
    {
        DBG ("Mouse up on chop button");
        handleChopButtonReleased();
    }
}

void ChopComponent::handleChopButtonPressed()
{
    chopStartTime = juce::Time::getMillisecondCounterHiRes();
        float currentPosition = getCrossfaderValue();
    if (plugin != nullptr)
        setCrossfaderValue(currentPosition <= 0.5f ? 1.0f : 0.0f);
}

void ChopComponent::handleChopButtonReleased()
{
    if (!getTempoCallback)
        return;

    double elapsedTime = juce::Time::getMillisecondCounterHiRes() - chopStartTime;
    double minimumTime = getChopDurationInMs(getTempoCallback());

    if (elapsedTime >= minimumTime)
    {
        float currentPosition = getCrossfaderValue();
        if (plugin != nullptr)
            setCrossfaderValue(currentPosition <= 0.5f ? 1.0f : 0.0f);
    }
    else
    {
        chopReleaseDelay = minimumTime - elapsedTime;
        startTimer(static_cast<int>(chopReleaseDelay));
    }
}

void ChopComponent::timerCallback()
{
    stopTimer();
    float currentPosition = getCrossfaderValue();
    if (plugin != nullptr)
        setCrossfaderValue(currentPosition <= 0.5f ? 1.0f : 0.0f);
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
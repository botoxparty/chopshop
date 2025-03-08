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
    chopButton.addMouseListener (this, false);

    // Configure crossfader
    crossfaderSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    crossfaderSlider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    crossfaderSlider.setRange (0.0, 1.0, 0.01);
    crossfaderSlider.setValue (0.0, juce::dontSendNotification);
    crossfaderSlider.setPopupMenuEnabled (false);

    // Get the crossfader parameter
    if (auto crossfaderParam = plugin->getAutomatableParameterByID("crossfader"))
    {
        // Bind the crossfader parameter
        bindSliderToParameter(crossfaderSlider, *crossfaderParam);
    }

    // Add components to the content component instead of the base
    contentComponent.addAndMakeVisible (durationLabel);
    contentComponent.addAndMakeVisible (chopDurationComboBox);
    contentComponent.addAndMakeVisible (chopButton);
    contentComponent.addAndMakeVisible (crossfaderSlider);

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

    // Now layout the content within the content component
    auto bounds = contentComponent.getLocalBounds();

    // Add left and right padding
    bounds.removeFromLeft (10);
    bounds.removeFromRight (10);

    // Create a grid layout
    juce::Grid grid;
    grid.rowGap = juce::Grid::Px (4);
    grid.columnGap = juce::Grid::Px (4);

    using Track = juce::Grid::TrackInfo;
    using Fr = juce::Grid::Fr;

    grid.templateRows = { Track (Fr (1)), Track (Fr (1)), Track (Fr (1)) };
    grid.templateColumns = { Track (Fr (1)), Track (Fr (2)) };

    grid.items = {
        juce::GridItem (durationLabel).withColumn ({ 1 }).withAlignSelf (juce::GridItem::AlignSelf::center),
        juce::GridItem (chopDurationComboBox).withColumn ({ 2 }).withHeight (30).withAlignSelf (juce::GridItem::AlignSelf::center),
        juce::GridItem (chopButton).withColumn ({ 1, 3 }).withHeight (30),
        // juce::GridItem (crossfaderSlider).withColumn ({ 1, 3 })
    };

    grid.performLayout (bounds);
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
#include "ChopComponent.h"

ChopComponent::ChopComponent(tracktion::engine::Edit& edit)
    : BaseEffectComponent(edit)
{
    titleLabel.setText("Chop Controls", juce::dontSendNotification);
    
    // Configure labels
    durationLabel.setText("Duration", juce::dontSendNotification);
    durationLabel.setJustificationType(juce::Justification::left);
    
    // Configure combo box
    chopDurationComboBox.addItem("1/4 Beat", 1);
    chopDurationComboBox.addItem("1/2 Beat", 2);
    chopDurationComboBox.addItem("1 Beat", 3);
    chopDurationComboBox.addItem("2 Beats", 4);
    chopDurationComboBox.addItem("4 Beats", 5);
    chopDurationComboBox.setSelectedId(3, juce::dontSendNotification);
    
    // Configure chop button
    chopButton.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
    chopButton.setColour(juce::TextButton::buttonColourId, juce::Colours::darkred);
    chopButton.addMouseListener(this, false);
    
    // Configure crossfader
    crossfaderSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    crossfaderSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    crossfaderSlider.setRange(0.0, 1.0, 0.01);
    crossfaderSlider.setValue(0.0, juce::dontSendNotification);
    // crossfaderSlider.setPopupDisplayEnabled(true, false, this);
    crossfaderSlider.setPopupMenuEnabled(false);
    crossfaderSlider.onValueChange = [this] {
        if (onCrossfaderValueChanged)
            onCrossfaderValueChanged(crossfaderSlider.getValue());
    };
    
    addAndMakeVisible(durationLabel);
    addAndMakeVisible(chopDurationComboBox);
    addAndMakeVisible(chopButton);
    addAndMakeVisible(crossfaderSlider);
    
    // Make sure this component can receive keyboard focus
    setWantsKeyboardFocus(true);
}

ChopComponent::~ChopComponent()
{
    if (commandManager != nullptr)
        commandManager->removeListener(this);
}

void ChopComponent::setCommandManager(juce::ApplicationCommandManager* manager)
{
    // Remove from previous manager if exists
    if (commandManager != nullptr)
        commandManager->removeListener(this);
    
    commandManager = manager;
    
    if (commandManager != nullptr)
    {
        // Register commands with the manager
        commandManager->registerAllCommandsForTarget(this);
        
        // Add as key listener
        addKeyListener(commandManager->getKeyMappings());
        
        // Register the space key for the chop command
        commandManager->getKeyMappings()->addKeyPress(CommandIDs::chopEffect, juce::KeyPress(juce::KeyPress::spaceKey));
    }
}

void ChopComponent::resized()
{
    auto bounds = getEffectiveArea();
    BaseEffectComponent::resized();
    
    // Create a grid layout
    juce::Grid grid;
    grid.rowGap = juce::Grid::Px(4);
    grid.columnGap = juce::Grid::Px(4);
    
    using Track = juce::Grid::TrackInfo;
    using Fr = juce::Grid::Fr;
    
    grid.templateRows = { Track(Fr(1)), Track(Fr(1)), Track(Fr(1)) };
    grid.templateColumns = { Track(Fr(1)), Track(Fr(2)) };
    
    grid.items = {
        juce::GridItem(durationLabel).withColumn({1}).withAlignSelf(juce::GridItem::AlignSelf::center),
        juce::GridItem(chopDurationComboBox).withColumn({2}).withHeight(30).withAlignSelf(juce::GridItem::AlignSelf::center),
        juce::GridItem(chopButton).withColumn({1, 3}).withHeight(30),
        juce::GridItem(crossfaderSlider).withColumn({1, 3})
    };
    
    grid.performLayout(bounds.toNearestInt());
}

double ChopComponent::getChopDurationInMs(double currentTempo) const
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

void ChopComponent::mouseDown(const juce::MouseEvent& event)
{
    if (event.eventComponent == &chopButton && onChopButtonPressed)
    {
        DBG("Mouse down on chop button");
        onChopButtonPressed();
    }
}

void ChopComponent::mouseUp(const juce::MouseEvent& event)
{
    if (event.eventComponent == &chopButton && onChopButtonReleased)
    {
        DBG("Mouse up on chop button");
        onChopButtonReleased();
    }
}

// Implement ApplicationCommandTarget methods
juce::ApplicationCommandTarget* ChopComponent::getNextCommandTarget()
{
    return findFirstTargetParentComponent();
}

void ChopComponent::getAllCommands(juce::Array<juce::CommandID>& commands)
{
    commands.add(CommandIDs::chopEffect); // Chop command
}

void ChopComponent::getCommandInfo(juce::CommandID commandID, juce::ApplicationCommandInfo& result)
{
    if (commandID == CommandIDs::chopEffect) // Chop command
    {
        result.setInfo("Chop", "Activates the chop effect", "Chop", 0);
        result.addDefaultKeypress(juce::KeyPress::spaceKey, 0);
        
        // This is the key line - tell the command manager we want key up/down callbacks
        result.flags |= juce::ApplicationCommandInfo::wantsKeyUpDownCallbacks;
    }
}

bool ChopComponent::perform(const juce::ApplicationCommandTarget::InvocationInfo& info)
{
    if (info.commandID == CommandIDs::chopEffect) // Chop command
    {
        if (info.isKeyDown)
        {
            if (onChopButtonPressed)
                onChopButtonPressed();
        }
        else
        {
            if (onChopButtonReleased)
                onChopButtonReleased();
        }
        return true;
    }
    
    return false;
} 
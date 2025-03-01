/*
  ==============================================================================

    DelayComponent.cpp
    Created: 18 Jan 2025 9:27:36am
    Author:  Adam Hammad

  ==============================================================================
*/

#include "DelayComponent.h"

DelayComponent::DelayComponent(tracktion::engine::Edit& edit)
    : BaseEffectComponent(edit)
{
    setMixParameterId("mix proportion");
    mixSlider.setComponentID("mix proportion");
    titleLabel.setText("Delay", juce::dontSendNotification);
    
    // Configure labels
    feedbackLabel.setText("Feedback", juce::dontSendNotification);
    mixLabel.setText("Mix", juce::dontSendNotification);
    timeLabel.setText("Time", juce::dontSendNotification);
    
    feedbackLabel.setJustificationType(juce::Justification::centred);
    mixLabel.setJustificationType(juce::Justification::centred);
    timeLabel.setJustificationType(juce::Justification::centred);
    
    // Configure sliders
    feedbackSlider.setTextValueSuffix(" dB");
    feedbackSlider.setNumDecimalPlacesToDisplay(1);
    feedbackSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    feedbackSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 15);
    
    mixSlider.setTextValueSuffix("%");
    mixSlider.setNumDecimalPlacesToDisplay(0);
    mixSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    mixSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 15);
    
    // Configure note value combo box
    noteValueBox.addItem("1/16", 1);
    noteValueBox.addItem("1/8", 2);
    noteValueBox.addItem("1/4", 3);
    noteValueBox.addItem("1/2", 4);
    noteValueBox.addItem("1", 5);
    noteValueBox.setSelectedId(3); // Default to 1/4 note
    
    noteValueBox.onChange = [this] { updateDelayTimeFromNote(); };
    
    feedbackSlider.setDoubleClickReturnValue(true, -30.0);
    mixSlider.setDoubleClickReturnValue(true, 0.0);

    addAndMakeVisible(feedbackLabel);
    addAndMakeVisible(mixLabel);
    addAndMakeVisible(timeLabel);
    addAndMakeVisible(feedbackSlider);
    addAndMakeVisible(mixSlider);
    addAndMakeVisible(noteValueBox);

    // Create and setup plugin
    plugin = createPlugin(AutoDelayPlugin::xmlTypeName);
    
    if (plugin != nullptr)
    {
        if (auto feedbackParam = plugin->getAutomatableParameterByID("feedback"))
            bindSliderToParameter(feedbackSlider, *feedbackParam);
            
        if (auto mixParam = plugin->getAutomatableParameterByID("mix proportion"))
        {
            bindSliderToParameter(mixSlider, *mixParam);
            mixParam->setParameter(0.0f, juce::sendNotification);
        }
            
        if (auto lengthParam = plugin->getAutomatableParameterByID("length"))
        {
            updateDelayTimeFromNote();
        }
    }

    mixRamp.onValueChange = [this](float value) {
        mixSlider.setValue(value, juce::sendNotification);
    };
}

void DelayComponent::resized()
{
    auto bounds = getEffectiveArea();
    BaseEffectComponent::resized();
    
    // Create a grid layout
    juce::Grid grid;
    grid.rowGap = juce::Grid::Px(4);
    grid.columnGap = juce::Grid::Px(4);
    
    using Track = juce::Grid::TrackInfo;
    using Fr = juce::Grid::Fr;
    
    grid.templateRows = { Track(Fr(1)), Track(Fr(2)) };    // Label row, Dial row
    grid.templateColumns = { Track(Fr(1)), Track(Fr(1)), Track(Fr(1)) };
    
    // Add items to grid
    grid.items = {
        juce::GridItem(feedbackLabel),
        juce::GridItem(mixLabel),
        juce::GridItem(timeLabel),
        juce::GridItem(feedbackSlider).withSize(60, 60).withJustifySelf(juce::GridItem::JustifySelf::center),
        juce::GridItem(mixSlider).withSize(60, 60).withJustifySelf(juce::GridItem::JustifySelf::center),
        juce::GridItem(noteValueBox).withSize(60, 60).withJustifySelf(juce::GridItem::JustifySelf::center)
    };
    
    grid.performLayout(bounds.toNearestInt());
}

void DelayComponent::setDelayTime(double milliseconds)
{
    if (plugin != nullptr)
    {
        if (auto lengthParam = plugin->getAutomatableParameterByID("length"))
            lengthParam->setParameter(static_cast<float>(milliseconds), juce::sendNotification);
        else
            DBG("Length parameter not found");
    }
}

void DelayComponent::rampMixLevel(bool rampUp)
{
    if (rampUp)
    {
        storedMixValue = mixSlider.getValue();
        mixRamp.startRamp(1.0);
    }
    else
    {
        mixRamp.startRamp(storedMixValue);
    }
}

void DelayComponent::updateDelayTimeFromNote()
{
    if (plugin == nullptr)
        return;

    double beatDuration = 60.0 / tempo * 1000.0; // Convert to milliseconds
    double delayTime = 0.0;
    
    switch (noteValueBox.getSelectedId())
    {
        case 1: delayTime = beatDuration * 0.25; break;  // 1/16 note
        case 2: delayTime = beatDuration * 0.5; break;   // 1/8 note
        case 3: delayTime = beatDuration; break;         // 1/4 note
        case 4: delayTime = beatDuration * 2.0; break;   // 1/2 note
        case 5: delayTime = beatDuration * 4.0; break;   // whole note
    }
    
    if (auto lengthParam = plugin->getAutomatableParameterByID("length"))
        lengthParam->setParameter(static_cast<float>(delayTime), juce::sendNotification);
}

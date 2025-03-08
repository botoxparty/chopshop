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
    
    // Configure time label
    timeLabel.setText("Time", juce::dontSendNotification);
    timeLabel.setJustificationType(juce::Justification::centred);
    timeLabel.setFont(juce::Font(14.0f));
    
    // Configure sliders
    feedbackSlider.getSlider().setTextValueSuffix(" dB");
    feedbackSlider.getSlider().setNumDecimalPlacesToDisplay(1);
    feedbackSlider.getSlider().setDoubleClickReturnValue(true, -30.0);
    
    mixSlider.getSlider().setTextValueSuffix("%");
    mixSlider.getSlider().setNumDecimalPlacesToDisplay(0);
    mixSlider.getSlider().setDoubleClickReturnValue(true, 0.0);
    
    // Configure note value combo box
    noteValueBox.addItem("1/16", 1);
    noteValueBox.addItem("1/8", 2);
    noteValueBox.addItem("1/4", 3);
    noteValueBox.addItem("1/2", 4);
    noteValueBox.addItem("1", 5);
    noteValueBox.setSelectedId(3); // Default to 1/4 note
    noteValueBox.onChange = [this] { updateDelayTimeFromNote(); };

    // Add components to content component
    contentComponent.addAndMakeVisible(feedbackSlider);
    contentComponent.addAndMakeVisible(mixSlider);
    contentComponent.addAndMakeVisible(timeLabel);
    contentComponent.addAndMakeVisible(noteValueBox);

    // Create and setup plugin
    plugin = EngineHelpers::getPluginFromRack(edit, AutoDelayPlugin::xmlTypeName);
    
    if (plugin != nullptr)
    {
        if (auto feedbackParam = plugin->getAutomatableParameterByID("feedback"))
            bindSliderToParameter(feedbackSlider.getSlider(), *feedbackParam);
            
        if (auto mixParam = plugin->getAutomatableParameterByID("mix proportion"))
        {
            bindSliderToParameter(mixSlider.getSlider(), *mixParam);
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
    // First let the base component handle its layout
    BaseEffectComponent::resized();
    
    // Now layout the content within the content component
    auto bounds = contentComponent.getLocalBounds().reduced(4);
    
    // Calculate component sizes
    auto componentWidth = (bounds.getWidth() - 16) / 3; // -16 for gaps between components
    
    // Position components
    feedbackSlider.setBounds(bounds.removeFromLeft(componentWidth));
    bounds.removeFromLeft(8); // gap
    
    mixSlider.setBounds(bounds.removeFromLeft(componentWidth));
    bounds.removeFromLeft(8); // gap
    
    // For the time section, create a vertical layout
    auto timeSection = bounds;
    timeLabel.setBounds(timeSection.removeFromTop(20));
    noteValueBox.setBounds(timeSection.withSizeKeepingCentre(60, 25));
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

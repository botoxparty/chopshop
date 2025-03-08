/*
  ==============================================================================

    FlangerComponent.cpp
    Created: 17 Jan 2025 10:12:43pm
    Author:  Adam Hammad

  ==============================================================================
*/

#include "FlangerComponent.h"

FlangerComponent::FlangerComponent(tracktion::engine::Edit &edit)
    : BaseEffectComponent(edit)
{
    setMixParameterId("mix");
    mixSlider.setComponentID("mix");
    titleLabel.setText("Flanger", juce::dontSendNotification);

    // Configure sliders
    depthSlider.getSlider().setTextValueSuffix("");
    depthSlider.getSlider().setNumDecimalPlacesToDisplay(2);
    depthSlider.getSlider().setDoubleClickReturnValue(true, 0.0);

    speedSlider.getSlider().setTextValueSuffix(" Hz");
    speedSlider.getSlider().setNumDecimalPlacesToDisplay(2);
    speedSlider.getSlider().setDoubleClickReturnValue(true, 0.0);

    widthSlider.getSlider().setTextValueSuffix("");
    widthSlider.getSlider().setNumDecimalPlacesToDisplay(2);
    widthSlider.getSlider().setDoubleClickReturnValue(true, 0.0);

    mixSlider.getSlider().setTextValueSuffix("%");
    mixSlider.getSlider().setNumDecimalPlacesToDisplay(0);
    mixSlider.getSlider().setDoubleClickReturnValue(true, 0.0);
    
    // Add components to content component
    contentComponent.addAndMakeVisible(depthSlider);
    contentComponent.addAndMakeVisible(speedSlider);
    contentComponent.addAndMakeVisible(widthSlider);
    contentComponent.addAndMakeVisible(mixSlider);

    // Create and setup plugin
    plugin = getPluginFromRack(edit, FlangerPlugin::xmlTypeName);

    if (plugin != nullptr)
    {
        // Initialize all parameters to zero first
        for (auto param : plugin->getAutomatableParameters())
        {
            param->setParameter(0.0f, juce::sendNotification);
        }

        if (auto depthParam = plugin->getAutomatableParameterByID("depth"))
            bindSliderToParameter(depthSlider.getSlider(), *depthParam);
        else
            DBG("Depth parameter not found");

        if (auto speedParam = plugin->getAutomatableParameterByID("speed"))
            bindSliderToParameter(speedSlider.getSlider(), *speedParam);
        else
            DBG("Speed parameter not found");

        if (auto widthParam = plugin->getAutomatableParameterByID("width"))
            bindSliderToParameter(widthSlider.getSlider(), *widthParam);
        else
            DBG("Width parameter not found");

        if (auto mixParam = plugin->getAutomatableParameterByID("mix"))
        {   
            mixParam->setParameter(0.7f, juce::sendNotification);
            bindSliderToParameter(mixSlider.getSlider(), *mixParam);
        }
        else
            DBG("Mix parameter not found");
    }

    mixRamp.onValueChange = [this](float value) {
        mixSlider.setValue(value, juce::sendNotification);
    };
}

void FlangerComponent::resized()
{
    // First let the base component handle its layout
    BaseEffectComponent::resized();
    
    // Now layout the content within the content component
    auto bounds = contentComponent.getLocalBounds().reduced(4);
    
    // Calculate component sizes
    auto componentWidth = (bounds.getWidth() - 24) / 4; // -24 for gaps between components
    
    // Position components
    depthSlider.setBounds(bounds.removeFromLeft(componentWidth));
    bounds.removeFromLeft(8); // gap
    
    speedSlider.setBounds(bounds.removeFromLeft(componentWidth));
    bounds.removeFromLeft(8); // gap
    
    widthSlider.setBounds(bounds.removeFromLeft(componentWidth));
    bounds.removeFromLeft(8); // gap
    
    mixSlider.setBounds(bounds);
}

void FlangerComponent::setDepth(float value)
{
    depthSlider.setValue(value, juce::sendNotification);
}

void FlangerComponent::setSpeed(float value)
{
    speedSlider.setValue(value, juce::sendNotification);
}

void FlangerComponent::setWidth(float value)
{
    widthSlider.setValue(value, juce::sendNotification);
}

void FlangerComponent::setMix(float value)
{
    mixSlider.setValue(value, juce::sendNotification);
}

void FlangerComponent::rampMixLevel(bool rampUp)
{
    mixRamp.startRamp(rampUp ? 1.0 : 0.0);
}

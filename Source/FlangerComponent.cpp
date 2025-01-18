/*
  ==============================================================================

    FlangerComponent.cpp
    Created: 17 Jan 2025 10:12:43pm
    Author:  Adam Hammad

  ==============================================================================
*/

#include "FlangerComponent.h"

FlangerComponent::FlangerComponent(tracktion_engine::Edit& edit)
    : BaseEffectComponent(edit)
{
    // Configure labels
    depthLabel.setText("Depth", juce::dontSendNotification);
    speedLabel.setText("Speed", juce::dontSendNotification);
    widthLabel.setText("Width", juce::dontSendNotification);
    mixLabel.setText("Mix", juce::dontSendNotification);
    
    depthLabel.setJustificationType(juce::Justification::centred);
    speedLabel.setJustificationType(juce::Justification::centred);
    widthLabel.setJustificationType(juce::Justification::centred);
    mixLabel.setJustificationType(juce::Justification::centred);
    
    // Configure sliders
    depthSlider.setTextValueSuffix("");
    depthSlider.setNumDecimalPlacesToDisplay(2);
    
    speedSlider.setTextValueSuffix(" Hz");
    speedSlider.setNumDecimalPlacesToDisplay(2);
    
    widthSlider.setTextValueSuffix("");
    widthSlider.setNumDecimalPlacesToDisplay(2);
    
    mixSlider.setTextValueSuffix("%");
    mixSlider.setNumDecimalPlacesToDisplay(0);
    
    addAndMakeVisible(depthLabel);
    addAndMakeVisible(speedLabel);
    addAndMakeVisible(widthLabel);
    addAndMakeVisible(mixLabel);
    addAndMakeVisible(depthSlider);
    addAndMakeVisible(speedSlider);
    addAndMakeVisible(widthSlider);
    addAndMakeVisible(mixSlider);

    // Create and setup plugin
    plugin = createPlugin(tracktion_engine::ChorusPlugin::xmlTypeName);
    
    if (plugin != nullptr)
    {
        if (auto depthParam = plugin->getAutomatableParameterByID("depth"))
            bindSliderToParameter(depthSlider, *depthParam);
        else
            DBG("Depth parameter not found");
            
        if (auto speedParam = plugin->getAutomatableParameterByID("speed"))
            bindSliderToParameter(speedSlider, *speedParam);
        else
            DBG("Speed parameter not found");
            
        if (auto widthParam = plugin->getAutomatableParameterByID("width"))
            bindSliderToParameter(widthSlider, *widthParam);
        else
            DBG("Width parameter not found");
            
        if (auto mixParam = plugin->getAutomatableParameterByID("mix"))
            bindSliderToParameter(mixSlider, *mixParam);
        else
            DBG("Mix parameter not found");
    }
}

void FlangerComponent::resized()
{
    auto bounds = getLocalBounds();
    auto sliderHeight = bounds.getHeight() / 4;
    
    // Depth section
    auto depthBounds = bounds.removeFromTop(sliderHeight);
    depthLabel.setBounds(depthBounds.removeFromTop(20));
    depthSlider.setBounds(depthBounds);
    
    // Speed section
    auto speedBounds = bounds.removeFromTop(sliderHeight);
    speedLabel.setBounds(speedBounds.removeFromTop(20));
    speedSlider.setBounds(speedBounds);
    
    // Width section
    auto widthBounds = bounds.removeFromTop(sliderHeight);
    widthLabel.setBounds(widthBounds.removeFromTop(20));
    widthSlider.setBounds(widthBounds);
    
    // Mix section
    auto mixBounds = bounds;
    mixLabel.setBounds(mixBounds.removeFromTop(20));
    mixSlider.setBounds(mixBounds);
}

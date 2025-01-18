/*
  ==============================================================================

    DelayComponent.cpp
    Created: 18 Jan 2025 9:27:36am
    Author:  Adam Hammad

  ==============================================================================
*/

#include "DelayComponent.h"

DelayComponent::DelayComponent(tracktion_engine::Edit& edit)
    : BaseEffectComponent(edit)
{
    // Configure labels
    feedbackLabel.setText("Feedback", juce::dontSendNotification);
    mixLabel.setText("Mix", juce::dontSendNotification);
    lengthLabel.setText("Delay Time", juce::dontSendNotification);
    
    feedbackLabel.setJustificationType(juce::Justification::centred);
    mixLabel.setJustificationType(juce::Justification::centred);
    lengthLabel.setJustificationType(juce::Justification::centred);
    
    // Configure sliders
    feedbackSlider.setTextValueSuffix(" dB");
    feedbackSlider.setNumDecimalPlacesToDisplay(1);
    
    mixSlider.setTextValueSuffix("%");
    mixSlider.setNumDecimalPlacesToDisplay(0);
    
    lengthSlider.setTextValueSuffix(" ms");
    lengthSlider.setNumDecimalPlacesToDisplay(0);
    
    addAndMakeVisible(feedbackLabel);
    addAndMakeVisible(mixLabel);
    addAndMakeVisible(lengthLabel);
    addAndMakeVisible(feedbackSlider);
    addAndMakeVisible(mixSlider);
    addAndMakeVisible(lengthSlider);

    // Create and setup plugin
    plugin = createPlugin(tracktion_engine::DelayPlugin::xmlTypeName);
    
    if (plugin != nullptr)
    {
        if (auto feedbackParam = plugin->getAutomatableParameterByID("feedback"))
            bindSliderToParameter(feedbackSlider, *feedbackParam);
        else
            DBG("Feedback parameter not found");
            
        if (auto mixParam = plugin->getAutomatableParameterByID("mix proportion"))
            bindSliderToParameter(mixSlider, *mixParam);
        else
            DBG("Mix parameter not found");
            
        if (auto lengthParam = plugin->getAutomatableParameterByID("length"))
            bindSliderToParameter(lengthSlider, *lengthParam);
        else
            DBG("Length parameter not found");
    }
}

void DelayComponent::resized()
{
    auto bounds = getLocalBounds();
    auto sliderHeight = bounds.getHeight() / 3;
    
    // Feedback section
    auto feedbackBounds = bounds.removeFromTop(sliderHeight);
    feedbackLabel.setBounds(feedbackBounds.removeFromTop(20));
    feedbackSlider.setBounds(feedbackBounds);
    
    // Mix section
    auto mixBounds = bounds.removeFromTop(sliderHeight);
    mixLabel.setBounds(mixBounds.removeFromTop(20));
    mixSlider.setBounds(mixBounds);
    
    // Length section
    auto lengthBounds = bounds;
    lengthLabel.setBounds(lengthBounds.removeFromTop(20));
    lengthSlider.setBounds(lengthBounds);
}

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
    // In your effect components (DelayComponent, ReverbComponent, etc.), modify the slider style:
    feedbackSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    feedbackSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 15);
    
    mixSlider.setTextValueSuffix("%");
    mixSlider.setNumDecimalPlacesToDisplay(0);
    mixSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    mixSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 15);
    
    lengthSlider.setTextValueSuffix(" ms");
    lengthSlider.setNumDecimalPlacesToDisplay(0);
    lengthSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    lengthSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 15);
    
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
        {
            bindSliderToParameter(mixSlider, *mixParam);
            mixParam->setParameter(0.0f, juce::sendNotification); // Set mix to 0% by default
        }
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
    auto bounds = getEffectiveArea();
    const int dialSize = juce::jmin(bounds.getWidth() / 3, bounds.getHeight() / 2);
    
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
        juce::GridItem(lengthLabel),
        juce::GridItem(feedbackSlider),
        juce::GridItem(mixSlider),
        juce::GridItem(lengthSlider)
    };
    
    grid.performLayout(bounds.toNearestInt());
}

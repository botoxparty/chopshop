#include "PhaserComponent.h"

PhaserComponent::PhaserComponent(tracktion::engine::Edit& edit)
    : BaseEffectComponent(edit)
{
    titleLabel.setText("Phaser", juce::dontSendNotification);
    
    // Configure sliders
    depthSlider.getSlider().setTextValueSuffix("");
    depthSlider.getSlider().setNumDecimalPlacesToDisplay(2);
    depthSlider.getSlider().setDoubleClickReturnValue(true, 0.0);
    
    rateSlider.getSlider().setTextValueSuffix(" Hz");
    rateSlider.getSlider().setNumDecimalPlacesToDisplay(2);
    rateSlider.getSlider().setDoubleClickReturnValue(true, 0.0);
    
    feedbackSlider.getSlider().setTextValueSuffix("");
    feedbackSlider.getSlider().setNumDecimalPlacesToDisplay(2);
    feedbackSlider.getSlider().setDoubleClickReturnValue(true, 0.0);

    // Add components to content component
    contentComponent.addAndMakeVisible(depthSlider);
    contentComponent.addAndMakeVisible(rateSlider);
    contentComponent.addAndMakeVisible(feedbackSlider);

    // Create and setup plugin
    plugin = createPlugin(AutoPhaserPlugin::xmlTypeName);
    
    if (plugin != nullptr)
    {
        // Initialize all parameters to zero first
        for (auto param : plugin->getAutomatableParameters())
        {
            param->setParameter(0.0f, juce::sendNotification);
        }

        // Use the correct parameter IDs from the plugin
        if (auto depthParam = plugin->getAutomatableParameterByID("depth"))
        {
            bindSliderToParameter(depthSlider.getSlider(), *depthParam);
        }
        
        if (auto rateParam = plugin->getAutomatableParameterByID("rate"))
        {
            bindSliderToParameter(rateSlider.getSlider(), *rateParam);
        }
        
        if (auto feedbackParam = plugin->getAutomatableParameterByID("feedback"))
        {
            bindSliderToParameter(feedbackSlider.getSlider(), *feedbackParam);
        }
    }
}

void PhaserComponent::resized()
{
    // First let the base component handle its layout
    BaseEffectComponent::resized();
    
    // Now layout the content within the content component
    auto bounds = contentComponent.getLocalBounds().reduced(4);
    
    // Calculate component sizes
    auto componentWidth = (bounds.getWidth() - 16) / 3; // -16 for gaps between components
    
    // Position components
    depthSlider.setBounds(bounds.removeFromLeft(componentWidth));
    bounds.removeFromLeft(8); // gap
    
    rateSlider.setBounds(bounds.removeFromLeft(componentWidth));
    bounds.removeFromLeft(8); // gap
    
    feedbackSlider.setBounds(bounds);
}

void PhaserComponent::setDepth(float value)
{
    depthSlider.setValue(value * 10.0f, juce::sendNotification);
}

void PhaserComponent::setRate(float value)
{
    rateSlider.setValue(value * 10.0f, juce::sendNotification);
}

void PhaserComponent::setFeedback(float value)
{
    feedbackSlider.setValue(value, juce::sendNotification);
} 
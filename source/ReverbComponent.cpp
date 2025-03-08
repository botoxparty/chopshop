#include "ReverbComponent.h"

ReverbComponent::ReverbComponent(tracktion::engine::Edit& edit)
    : BaseEffectComponent(edit)
{
    setMixParameterId("wet level");
    wetSlider.setComponentID("wet level");
    titleLabel.setText("Reverb", juce::dontSendNotification);
    
    // Make sure contentComponent is visible
    addAndMakeVisible(contentComponent);
    
    // Add components to content component
    contentComponent.addAndMakeVisible(roomSizeSlider);
    contentComponent.addAndMakeVisible(wetSlider);

    // Create and setup plugin
    plugin = getPluginFromRack(edit, tracktion::engine::ReverbPlugin::xmlTypeName);
    
    if (plugin != nullptr)
    {
        if (auto roomSizeParam = plugin->getAutomatableParameterByID("room size"))
            bindSliderToParameter(roomSizeSlider.getSlider(), *roomSizeParam);
            
        if (auto wetParam = plugin->getAutomatableParameterByID("wet level"))
            bindSliderToParameter(wetSlider.getSlider(), *wetParam);
    }
}

void ReverbComponent::resized()
{
    // First let the base component handle its layout
    BaseEffectComponent::resized();
    
    // Now layout the content within the content component
    auto bounds = contentComponent.getLocalBounds().reduced(4);
    
    // Split space evenly between the two sliders
    auto sliderWidth = (bounds.getWidth() - 8) / 2; // -8 for gap between sliders
    
    roomSizeSlider.setBounds(bounds.removeFromLeft(sliderWidth));
    bounds.removeFromLeft(8); // gap between sliders
    wetSlider.setBounds(bounds);
}

void ReverbComponent::rampMixLevel(bool rampUp)
{
    if (rampUp)
    {
        storedMixValue = wetSlider.getValue();
        wetSlider.setValue(1.0);
    }
    else
    {
        wetSlider.setValue(storedMixValue);
    }
}

void ReverbComponent::restoreMixLevel()
{
    rampMixLevel(false);
} 
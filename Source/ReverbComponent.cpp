#include "ReverbComponent.h"

ReverbComponent::ReverbComponent(tracktion_engine::Edit& edit)
    : BaseEffectComponent(edit)
{
    // Configure labels
    roomSizeLabel.setText("Room Size", juce::dontSendNotification);
    wetLabel.setText("Wet Level", juce::dontSendNotification);
    
    roomSizeLabel.setJustificationType(juce::Justification::centred);
    wetLabel.setJustificationType(juce::Justification::centred);
    
    // Configure sliders
    reverbRoomSizeSlider.setTextValueSuffix("");
    reverbRoomSizeSlider.setNumDecimalPlacesToDisplay(2);
    
    reverbWetSlider.setTextValueSuffix("");
    reverbWetSlider.setNumDecimalPlacesToDisplay(2);
    
    addAndMakeVisible(roomSizeLabel);
    addAndMakeVisible(wetLabel);
    addAndMakeVisible(reverbRoomSizeSlider);
    addAndMakeVisible(reverbWetSlider);

    // Create and setup plugin
    plugin = createPlugin(tracktion_engine::ReverbPlugin::xmlTypeName);
    
    if (plugin != nullptr)
    {
        if (auto roomSizeParam = plugin->getAutomatableParameterByID("room size"))
            bindSliderToParameter(reverbRoomSizeSlider, *roomSizeParam);
            
        if (auto wetParam = plugin->getAutomatableParameterByID("wet level"))
            bindSliderToParameter(reverbWetSlider, *wetParam);
    }
}

void ReverbComponent::resized()
{
    auto bounds = getLocalBounds();
    auto sliderHeight = bounds.getHeight() / 2;
    
    // Room size section
    auto roomSizeBounds = bounds.removeFromTop(sliderHeight);
    roomSizeLabel.setBounds(roomSizeBounds.removeFromTop(20));
    reverbRoomSizeSlider.setBounds(roomSizeBounds);
    
    // Wet level section
    auto wetBounds = bounds;
    wetLabel.setBounds(wetBounds.removeFromTop(20));
    reverbWetSlider.setBounds(wetBounds);
} 
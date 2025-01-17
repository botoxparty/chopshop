#include "ReverbComponent.h"
#include "Parameters.h"
#include <tracktion_engine/tracktion_engine.h>

ReverbComponent::ReverbComponent(tracktion_engine::Edit& edit)
{
    // Configure labels
    roomSizeLabel.setText("Room Size", juce::dontSendNotification);
    wetLabel.setText("Wet Level", juce::dontSendNotification);
    
    roomSizeLabel.setJustificationType(juce::Justification::centred);
    wetLabel.setJustificationType(juce::Justification::centred);
    
    // Configure sliders to show 2 decimal places
    reverbRoomSizeSlider.setTextValueSuffix("");
    reverbRoomSizeSlider.setNumDecimalPlacesToDisplay(2);
    
    reverbWetSlider.setTextValueSuffix("");
    reverbWetSlider.setNumDecimalPlacesToDisplay(2);
    
    addAndMakeVisible(roomSizeLabel);
    addAndMakeVisible(wetLabel);
    addAndMakeVisible(reverbRoomSizeSlider);
    addAndMakeVisible(reverbWetSlider);

    // Create and add reverb plugin to master track
    if (auto track = edit.getMasterTrack())
    {
        reverbPlugin = edit.getPluginCache().createNewPlugin(tracktion_engine::ReverbPlugin::xmlTypeName, {});
        track->pluginList.insertPlugin(reverbPlugin.get(), 0, nullptr);
        
        if (auto roomSizeParam = reverbPlugin->getAutomatableParameterByID("room size"))
        {
            bindSliderToParameter(reverbRoomSizeSlider, *roomSizeParam);
        }
        
        if (auto wetParam = reverbPlugin->getAutomatableParameterByID("wet level"))
        {
            bindSliderToParameter(reverbWetSlider, *wetParam);
        }
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
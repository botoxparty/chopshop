#include "ReverbComponent.h"
#include "Parameters.h"
#include <tracktion_engine/tracktion_engine.h>

ReverbComponent::ReverbComponent(tracktion_engine::Edit& edit)
{
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
    
    reverbRoomSizeSlider.setBounds(bounds.removeFromTop(sliderHeight));
    reverbWetSlider.setBounds(bounds);
} 
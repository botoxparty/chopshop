/*
  ==============================================================================

    BaseEffectComponent.cpp
    Created: 17 Jan 2025 10:08:20pm
    Author:  Adam Hammad

  ==============================================================================
*/

#include "BaseEffectComponent.h"

BaseEffectComponent::BaseEffectComponent(tracktion_engine::Edit& e)
    : edit(e)
{
}

void BaseEffectComponent::resized()
{
    // Base implementation - override in derived classes
}

void BaseEffectComponent::bindSliderToParameter(juce::Slider& slider, tracktion_engine::AutomatableParameter& param)
{
    slider.setRange(param.getValueRange().getStart(), param.getValueRange().getEnd(), 0.01);
    slider.setValue(param.getCurrentValue(), juce::dontSendNotification);
    
    slider.onValueChange = [&param, &slider] {
        param.setParameter(static_cast<float>(slider.getValue()), juce::sendNotification);
    };
    
    slider.onDragStart = [&param] { param.beginParameterChangeGesture(); };
    slider.onDragEnd = [&param] { param.endParameterChangeGesture(); };
}

tracktion_engine::Plugin::Ptr BaseEffectComponent::createPlugin(const juce::String& xmlType)
{
    if (auto track = edit.getMasterTrack())
    {
        auto plugin = edit.getPluginCache().createNewPlugin(xmlType, {});
        track->pluginList.insertPlugin(plugin.get(), 0, nullptr);
        return plugin;
    }
    return {};
}

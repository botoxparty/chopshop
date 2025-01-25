/*
  ==============================================================================

    BaseEffectComponent.h
    Created: 17 Jan 2025 10:08:20pm
    Author:  Adam Hammad

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

class BaseEffectComponent : public juce::Component
{
public:
    explicit BaseEffectComponent(tracktion_engine::Edit& edit);
    ~BaseEffectComponent() override = default;
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    
protected:
    void bindSliderToParameter(juce::Slider& slider, tracktion_engine::AutomatableParameter& param);
    tracktion_engine::Plugin::Ptr createPlugin(const juce::String& xmlType);
    juce::Rectangle<float> getEffectiveArea() const;
    
    tracktion_engine::Edit& edit;
    tracktion_engine::Plugin::Ptr plugin;
    juce::Label titleLabel;
    
private:
    void drawScrew(juce::Graphics& g, float x, float y);
    juce::Random random;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BaseEffectComponent)
};

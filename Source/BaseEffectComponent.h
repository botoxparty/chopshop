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
    
    virtual void storeAndSetMixLevel(float newLevel)
    {
        if (auto mixParam = plugin->getAutomatableParameterByID(mixParameterId))
        {
            storedMixLevel = mixParam->getCurrentValue();
            mixParam->setParameter(newLevel, juce::sendNotification);
            if (auto* slider = findChildWithID(mixParameterId))
                if (auto* mixSlider = dynamic_cast<juce::Slider*>(slider))
                    mixSlider->setValue(newLevel, juce::sendNotification);
        }
    }
    
    virtual void restoreMixLevel()
    {
        if (auto mixParam = plugin->getAutomatableParameterByID(mixParameterId))
        {
            mixParam->setParameter(storedMixLevel, juce::sendNotification);
            if (auto* slider = findChildWithID(mixParameterId))
                if (auto* mixSlider = dynamic_cast<juce::Slider*>(slider))
                    mixSlider->setValue(storedMixLevel, juce::sendNotification);
        }
    }
    
    void setMixParameterId(const juce::String& id) { mixParameterId = id; }
    tracktion_engine::Plugin::Ptr getPlugin() const { return plugin; }
    
protected:
    void bindSliderToParameter(juce::Slider& slider, tracktion_engine::AutomatableParameter& param);
    tracktion_engine::Plugin::Ptr createPlugin(const juce::String& xmlType);
    juce::Rectangle<float> getEffectiveArea() const;
    
    tracktion_engine::Edit& edit;
    tracktion_engine::Plugin::Ptr plugin;
    juce::Label titleLabel;
    
    float storedMixLevel = 0.0f;
    juce::String mixParameterId = "mix"; // Default ID, can be overridden by derived classes
    
private:
    void drawScrew(juce::Graphics& g, float x, float y);
    juce::Random random;

    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BaseEffectComponent)
};

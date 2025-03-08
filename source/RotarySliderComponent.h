#pragma once

#include "juce_gui_basics/juce_gui_basics.h"

class RotarySliderComponent : public juce::Component
{
public:
    RotarySliderComponent(const juce::String& labelText = "")
    {
        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 50, 15);
        slider.setNumDecimalPlacesToDisplay(2);
        slider.setTextValueSuffix("");
        addAndMakeVisible(slider);
        
        label.setText(labelText, juce::dontSendNotification);
        label.setJustificationType(juce::Justification::centred);
        label.setFont(juce::Font(14.0f));
        addAndMakeVisible(label);
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        
        // Reserve space for label at top
        const int labelHeight = 20;
        label.setBounds(bounds.removeFromTop(labelHeight));
        
        // Center the slider in remaining space with minimum size of 50
        auto sliderSize = juce::jmax(50, juce::jmin(bounds.getWidth(), bounds.getHeight() - 20)); // Account for text box
        auto sliderBounds = bounds.withSizeKeepingCentre(sliderSize, sliderSize);
        slider.setBounds(sliderBounds);
    }

    // Convenience methods to access the slider
    void setRange(double newMinimum, double newMaximum, double newInterval = 0.0)
    {
        slider.setRange(newMinimum, newMaximum, newInterval);
    }

    void setValue(double newValue, juce::NotificationType notification = juce::sendNotificationAsync)
    {
        slider.setValue(newValue, notification);
    }

    double getValue() const
    {
        return slider.getValue();
    }

    void setSkewFactor(double newSkewFactor)
    {
        slider.setSkewFactor(newSkewFactor);
    }

    void onValueChange(std::function<void()> callback)
    {
        slider.onValueChange = callback;
    }

    void setComponentID(const juce::String& newID)
    {
        slider.setComponentID(newID);
    }

    juce::Slider& getSlider() { return slider; }
    const juce::Slider& getSlider() const { return slider; }

private:
    juce::Slider slider;
    juce::Label label;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RotarySliderComponent)
}; 
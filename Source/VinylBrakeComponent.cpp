/*
  ==============================================================================

    VinylBrakeComponent.cpp
    Created: 18 Jan 2025 1:17:39am
    Author:  Adam Hammad

  ==============================================================================
*/

#include "VinylBrakeComponent.h"

VinylBrakeComponent::VinylBrakeComponent(tracktion_engine::Edit& edit)
    : BaseEffectComponent(edit)
{
    // Use the base class's titleLabel instead of creating a new one
    titleLabel.setText("Vinyl Brake", juce::dontSendNotification);
    
    // Configure brake slider
    brakeSlider.setRange(0.0, 1.0, 0.01);
    brakeSlider.setValue(0.0, juce::dontSendNotification);
    brakeSlider.setTextValueSuffix("");
    brakeSlider.setNumDecimalPlacesToDisplay(2);
    brakeSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    brakeSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 15);
    brakeSlider.setPopupDisplayEnabled(true, false, this);
    brakeSlider.setTextBoxIsEditable(false);
    
    // Add slider listeners
    brakeSlider.addListener(this);
    
    addAndMakeVisible(brakeSlider);
}

void VinylBrakeComponent::resized()
{
    auto bounds = getLocalBounds();
    BaseEffectComponent::resized(); // This will handle the title label
    
    // Get the effective area for the slider (area below title)
    auto sliderBounds = getEffectiveArea().toNearestInt();
    brakeSlider.setBounds(sliderBounds);
}

void VinylBrakeComponent::sliderValueChanged(juce::Slider* slider)
{
    if (slider == &brakeSlider)
    {
        const double value = slider->getValue();
        auto& transport = edit.getTransport();
        auto context = transport.getCurrentPlaybackContext();
        
        if (context != nullptr)
        {
            // Store original tempo adjustment when brake starts
            if (!hasStoredAdjustment && value > 0.0)
            {
                originalTempoAdjustment = getCurrentTempoAdjustment ? getCurrentTempoAdjustment() : 0.0;
                hasStoredAdjustment = true;
            }
            
            if (!isSpringAnimating)  // Only update directly if not animating
            {
                const double plusOrMinusProportion = originalTempoAdjustment - value;
                context->setTempoAdjustment(plusOrMinusProportion);
            }
        }
    }
}

void VinylBrakeComponent::startSpringAnimation()
{
    if (!isSpringAnimating)
    {
        isSpringAnimating = true;
        springStartValue = brakeSlider.getValue();
        springStartTime = juce::Time::getMillisecondCounterHiRes();
        startTimerHz(60);
    }
}

void VinylBrakeComponent::timerCallback()
{
    if (isSpringAnimating)
    {
        // Track elapsed time
        const double elapsedMs = (juce::Time::getMillisecondCounterHiRes() - springStartTime);
        const double duration = 500.0; // Match the 500ms duration from React
        const double progress = juce::jmin(elapsedMs / duration, 1.0);
        
        // Cubic easing function to match React implementation
        const double easeOut = 1.0 - pow(1.0 - progress, 3.0);
        currentSpringValue = springStartValue * (1.0 - easeOut);
        
        // Update slider and tempo
        brakeSlider.setValue(currentSpringValue, juce::dontSendNotification);
        
        auto& transport = edit.getTransport();
        auto context = transport.getCurrentPlaybackContext();
        
        if (context != nullptr)
        {
            const double plusOrMinusProportion = originalTempoAdjustment - currentSpringValue;
            context->setTempoAdjustment(plusOrMinusProportion);
        }
        
        // Stop animation when complete
        if (progress >= 1.0)
        {
            isSpringAnimating = false;
            currentSpringValue = 0.0;
            brakeSlider.setValue(0.0, juce::dontSendNotification);
            stopTimer();
            
            if (context != nullptr)
            {
                context->setTempoAdjustment(originalTempoAdjustment);
                hasStoredAdjustment = false;
            }
        }
    }
}

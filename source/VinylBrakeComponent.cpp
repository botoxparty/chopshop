/*
  ==============================================================================

    VinylBrakeComponent.cpp
    Created: 18 Jan 2025 1:17:39am
    Author:  Adam Hammad

  ==============================================================================
*/

#include "VinylBrakeComponent.h"

VinylBrakeComponent::VinylBrakeComponent(tracktion::engine::Edit& edit)
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
    // brakeSlider.setPopupDisplayEnabled(true, false, this);
    brakeSlider.setTextBoxIsEditable(false);
    
    // Add slider listeners
    brakeSlider.addListener(this);
    
    addAndMakeVisible(brakeSlider);
}

void VinylBrakeComponent::resized()
{
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
                // Apply brake effect by subtracting from the original adjustment
                setSpeed(originalTempoAdjustment - value);
            }
        }
    }
}

void VinylBrakeComponent::setSpeed(double value)
{
    // Get the tempo sequence from the edit
    auto& tempoSequence = edit.tempoSequence;
    
    // Calculate the speed ratio based on the brake value
    // value is the adjustment from original tempo (negative for brake effect)
    double speedRatio = 1.0 / (1.0 + value);
    
    // Get the current position
    // auto currentPosition = edit.getTransport().getPosition();
    
    // Get the base tempo from the tempo sequence
    double baseBpm = getEffectiveTempo();
    
    // Calculate the actual tempo that should be used (base tempo * screw adjustment * brake effect)
    // The tempoAdjustment is already (ratio - 1.0), so we add 1.0 to get the full ratio
    double currentBpm = baseBpm / speedRatio;
    
    // Insert a new tempo at the current position
    auto tempo = tempoSequence.insertTempo(tracktion::TimePosition::fromSeconds(0.0));
    DBG("Setting tempo adjustment to: " + juce::String(currentBpm));
    
    // Set the new tempo
    tempo->setBpm(currentBpm);
}

void VinylBrakeComponent::startSpringAnimation()
{
    if (!isSpringAnimating)
    {
        // Make sure we have the latest tempo adjustment before starting animation
        if (!hasStoredAdjustment && getCurrentTempoAdjustment)
        {
            originalTempoAdjustment = getCurrentTempoAdjustment();
            hasStoredAdjustment = true;
        }
        
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
        
        setSpeed(originalTempoAdjustment - currentSpringValue);

        
        // Stop animation when complete
        if (progress >= 1.0)
        {
            isSpringAnimating = false;
            currentSpringValue = 0.0;
            brakeSlider.setValue(0.0, juce::dontSendNotification);
            stopTimer();
            setSpeed(originalTempoAdjustment);
            hasStoredAdjustment = false;
        }
    }
}

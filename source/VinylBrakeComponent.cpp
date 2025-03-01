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
                DBG("Setting tempo adjustment to: " + juce::String(originalTempoAdjustment - value));
                setSpeed(originalTempoAdjustment - value);
            }
        }
    }
}

void VinylBrakeComponent::setSpeed(double value)
{
    // Get both clips and apply time stretching
    auto& transport = edit.getTransport();
    
    // Find the clips in the first two tracks
    tracktion::engine::WaveAudioClip* clip1 = nullptr;
    tracktion::engine::WaveAudioClip* clip2 = nullptr;
    
    if (auto track1 = EngineHelpers::getOrInsertAudioTrackAt(edit, 0))
        if (!track1->getClips().isEmpty())
            clip1 = dynamic_cast<tracktion::engine::WaveAudioClip*>(track1->getClips()[0]);
            
    if (auto track2 = EngineHelpers::getOrInsertAudioTrackAt(edit, 1))
        if (!track2->getClips().isEmpty())
            clip2 = dynamic_cast<tracktion::engine::WaveAudioClip*>(track2->getClips()[0]);
    
    // Calculate the speed ratio based on the brake value
    // value is the adjustment from original tempo (negative for brake effect)
    double speedRatio = 1.0 / (1.0 + value);
    
    // Apply to both clips
    if (clip1)
        clip1->setSpeedRatio(speedRatio);
        
    if (clip2)
        clip2->setSpeedRatio(speedRatio);
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

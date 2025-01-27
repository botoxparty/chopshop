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
    titleLabel.setText("Vinyl Brake", juce::dontSendNotification);
    
    // Configure brake slider
    brakeSlider.setRange(0.0, 1.0, 0.01);
    brakeSlider.setValue(0.0, juce::dontSendNotification);
    brakeSlider.setTextValueSuffix("");
    brakeSlider.setNumDecimalPlacesToDisplay(2);
    brakeSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    brakeSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 15);
    
    // Add slider listeners
    brakeSlider.addListener(this);
    
    addAndMakeVisible(titleLabel);
    addAndMakeVisible(brakeSlider);
}

void VinylBrakeComponent::resized()
{
    auto bounds = getEffectiveArea().toNearestInt();
    brakeSlider.setBounds(bounds);
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
            
            // Reset to original adjustment when returning to normal speed
            if (value == 0.0)
            {
                const double plusOrMinusProportion = originalTempoAdjustment;
                context->setTempoAdjustment(plusOrMinusProportion);
                hasStoredAdjustment = false;
            }
            else
            {
                const double plusOrMinusProportion = originalTempoAdjustment - value;
                context->setTempoAdjustment(plusOrMinusProportion);
            }
        }
    }
}

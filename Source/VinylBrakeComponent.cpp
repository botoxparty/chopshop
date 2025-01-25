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
    
    // Configure label
    decayTimeLabel.setText("Decay Time", juce::dontSendNotification);
    decayTimeLabel.setJustificationType(juce::Justification::centred);
    
    // Configure slider
    decayTimeSlider.setRange(0.1, 5.0, 0.1);
    decayTimeSlider.setValue(1.0);
    decayTimeSlider.setTextValueSuffix(" s");
    decayTimeSlider.setNumDecimalPlacesToDisplay(1);
    
    // Configure brake button
    brakeButton.onClick = [this] { triggerBrakeEffect(); };
    
    addAndMakeVisible(decayTimeLabel);
    addAndMakeVisible(decayTimeSlider);
    addAndMakeVisible(brakeButton);
}

void VinylBrakeComponent::resized()
{
    auto bounds = getEffectiveArea().toNearestInt();
    auto buttonHeight = 30;
    
    // Decay time section
    auto decayBounds = bounds.removeFromTop(bounds.getHeight() - buttonHeight);
    decayTimeLabel.setBounds(decayBounds.removeFromTop(20));
    decayTimeSlider.setBounds(decayBounds);
    
    // Brake button
    brakeButton.setBounds(bounds);
}

void VinylBrakeComponent::triggerBrakeEffect()
{
    const double decayTime = decayTimeSlider.getValue();
    double startSpeed = 1.0; // Default speed
    double startTime = edit.getTransport().getPosition().inSeconds();

    // Retrieve the current speed from the transport's play speed automation parameter
    if (auto speedParam = edit.getTempoTrack()->getAllAutomatableParams().getFirst())
    {
        startSpeed = speedParam->getCurrentValue(); // Retrieve current playback speed

        // Clear existing automation points
        auto& curve = speedParam->getCurve();
        juce::ValueTree automationState = curve.state;
        automationState.removeAllChildren(nullptr); // Remove existing points

        // Create new automation points for speed reduction
        const int numPoints = 50;
        for (int i = 0; i < numPoints; ++i)
        {
            double t = static_cast<double>(i) / (numPoints - 1);
            double speed = startSpeed * std::exp(-5.0 * t); // Exponential decay
            double time = startTime + t * decayTime;

            // Create a new automation point using fromSeconds
            tracktion::TimePosition pos = tracktion::TimePosition::fromSeconds(time);
            curve.addPoint(pos, static_cast<float>(speed), juce::Justification::centred);
        }

        // Add final point at zero speed
        tracktion::TimePosition finalPos = tracktion::TimePosition::fromSeconds(startTime + decayTime);
        curve.addPoint(finalPos, 0.0f, juce::Justification::centred);
    }

    // Stop transport after decay time
    juce::Timer::callAfterDelay(static_cast<int>(decayTime * 1000), [this]
    {
        edit.getTransport().stop(false, false);
    });
}

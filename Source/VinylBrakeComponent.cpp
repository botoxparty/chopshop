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
    
    addAndMakeVisible(titleLabel);
    addAndMakeVisible(brakeButton);
    addAndMakeVisible(decayTimeLabel);
    addAndMakeVisible(decayTimeSlider);
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
    // Multiply the decay time by 3 for a longer effect
    const double decayTime = decayTimeSlider.getValue() * 3.0;
    auto startPosition = edit.getTransport().getPosition();
    double startTime = startPosition.inSeconds();
    
    // Get the current tempo
    double startTempo = edit.tempoSequence.getTempoAt(startPosition).getBpm();
    
    // Store the timer pointer
    if (currentBrakeTimer != nullptr)
    {
        currentBrakeTimer->startRelease();
    }
    else
    {
        currentBrakeTimer = new BrakeTimer(edit, startTempo, decayTime);
        currentBrakeTimer->startTimer(30); // Update 30 times per second
    }
}

void VinylBrakeComponent::mouseDown(const juce::MouseEvent& event)
{
    if (event.eventComponent == &brakeButton)
    {
        triggerBrakeEffect();
    }
}

void VinylBrakeComponent::mouseUp(const juce::MouseEvent& event)
{
    if (event.eventComponent == &brakeButton && currentBrakeTimer != nullptr)
    {
        currentBrakeTimer->startRelease();
        currentBrakeTimer = nullptr;
    }
}

VinylBrakeComponent::BrakeTimer::BrakeTimer(tracktion_engine::Edit& e, double startTempo, double decayTime)
    : edit(e), initialTempo(startTempo), decayTimeMs(decayTime * 1000), isReleasing(false)
{
    startTime = juce::Time::getMillisecondCounter();
}

void VinylBrakeComponent::BrakeTimer::timerCallback()
{
    auto currentTime = juce::Time::getMillisecondCounter();
    auto elapsedMs = currentTime - startTime;
    
    double t;
    if (isReleasing)
    {
        // When releasing, go from current tempo back to initial tempo
        t = juce::jmin(1.0, elapsedMs / releaseTimeMs);
        double releaseProgress = t; // Linear interpolation for release
        double tempoMultiplier = currentTempoMultiplier + (1.0 - currentTempoMultiplier) * releaseProgress;
        
        // Update tempo through playback context
        const double ratio = tempoMultiplier;
        const double plusOrMinusProportion = ratio - 1.0;
        edit.getTransport().getCurrentPlaybackContext()->setTempoAdjustment(plusOrMinusProportion);
        
        if (t >= 1.0)
        {
            stopTimer();
            delete this;
        }
    }
    else
    {
        // Normal brake behavior
        t = juce::jmin(1.0, elapsedMs / decayTimeMs);
        // Increased power to 6.0 for an even more gradual initial slowdown
        currentTempoMultiplier = std::pow(1.0 - t, 6.0);
        
        // Update tempo through playback context
        const double ratio = currentTempoMultiplier;
        const double plusOrMinusProportion = ratio - 1.0;
        edit.getTransport().getCurrentPlaybackContext()->setTempoAdjustment(plusOrMinusProportion);
    }
}

void VinylBrakeComponent::BrakeTimer::startRelease()
{
    if (!isReleasing)
    {
        isReleasing = true;
        startTime = juce::Time::getMillisecondCounter();
        releaseTimeMs = 500.0; // 500ms for release time - adjust as needed
    }
}

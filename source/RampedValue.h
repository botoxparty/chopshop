#pragma once

#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>


class RampedValue : public juce::Timer
{
public:
    RampedValue(double initialValue = 0.0, int rampLengthMs = 500) 
        : currentValue(initialValue), rampDurationMs(rampLengthMs) {}
    
    void startRamp(double targetVal)
    {
        startValue = currentValue;
        targetValue = targetVal;
        startTime = juce::Time::getMillisecondCounterHiRes();
        isRamping = true;
        startTimerHz(60);
    }
    
    void timerCallback() override
    {
        if (!isRamping) return;
        
        double elapsed = juce::Time::getMillisecondCounterHiRes() - startTime;
        double progress = juce::jmin(elapsed / rampDurationMs, 1.0);
        
        // Use cubic easing for smooth ramping
        double easedProgress = 1.0 - std::pow(1.0 - progress, 3.0);
        currentValue = startValue + (targetValue - startValue) * easedProgress;
        
        if (onValueChange)
            onValueChange(currentValue);
            
        if (progress >= 1.0)
        {
            isRamping = false;
            stopTimer();
        }
    }
    
    std::function<void(double)> onValueChange;
    
private:
    double startValue = 0.0;
    double targetValue = 0.0;
    double currentValue = 0.0;
    double startTime = 0.0;
    int rampDurationMs = 100;
    bool isRamping = false;
}; 
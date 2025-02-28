#pragma once
#include <JuceHeader.h>

class RampedValue : public juce::Timer
{
public:
    RampedValue(float startValue = 0.0f, int rampTimeMs = 500) 
        : currentValue(startValue), rampDurationMs(rampTimeMs) {}
    
    void startRamp(float targetVal, bool rampUp)
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
    
    std::function<void(float)> onValueChange;
    
private:
    float startValue = 0.0f;
    float targetValue = 0.0f;
    float currentValue = 0.0f;
    double startTime = 0.0;
    int rampDurationMs = 100;
    bool isRamping = false;
}; 
#pragma once

#include <juce_core/juce_core.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <tracktion_engine/tracktion_engine.h>
#include "BaseEffectComponent.h"
#include "Utilities.h"
#include "RampedValue.h"
#include "Plugins/ScratchPlugin.h"
class ScratchComponent : public BaseEffectComponent
{
public:
    explicit ScratchComponent(tracktion::engine::Edit& edit);
    ~ScratchComponent() override;
    
    void resized() override;
    void paint(juce::Graphics& g) override;
    
    // Add callback for getting parent's tempo adjustment
    std::function<double()> getCurrentTempoAdjustment;
    std::function<double()> getEffectiveTempo;
    
    void setScratchValue(double value);
    
    double getScratchValue() const
    {
        return scratchSlider->getValue();
    }
    
    void startSpringAnimation();
    
    // Audio processing method
    void processAudioBuffer(juce::AudioBuffer<double>& buffer);
    
    // Plugin interface methods
    void prepareToPlay(double sampleRate, int samplesPerBlock);
    void releaseResources();
    
private:
    class SpringSlider : public juce::Slider
    {
    public:
        SpringSlider()
        {
            setSliderStyle(juce::Slider::SliderStyle::LinearHorizontal);
            setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
            setRange(-1.0, 1.0);
            setValue(0.0);
            
            // Spring back to center when mouse is released
            onDragEnd = [this]()
            {
                setValue(0.0, juce::sendNotification);
            };
        }
    };
    
    std::unique_ptr<SpringSlider> scratchSlider;
    std::unique_ptr<juce::Slider> mixSlider;
    
    // Audio processing components
    juce::IIRFilter resonantFilter;
    juce::IIRFilter stateFilter;
    juce::dsp::WaveShaper<double> distortion;
    
    // Filter parameters
    double resonanceAmount = 2.0;
    double filterCutoff = 2000.0;
    double filterQ = 1.0;
    double currentSampleRate = 44100.0;
    
    // Original members
    double originalTempoAdjustment = 0.0;
    bool hasStoredAdjustment = false;
    bool isSpringAnimating = false;
    double currentSpringValue = 0.0;
    double springStartTime = 0.0;
    double springStartValue = 0.0;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScratchComponent)
}; 
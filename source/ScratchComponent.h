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
    
    juce::Point<float> getScratchPosition() const
    {
        return scratchPad->getCurrentPosition();
    }
    
    void startSpringAnimation();
    
    // Audio processing method
    void processAudioBuffer(juce::AudioBuffer<double>& buffer);
    
    // Plugin interface methods
    void prepareToPlay(double sampleRate, int samplesPerBlock);
    void releaseResources();

    void setScratchSpeed(float speed);
    
private:
    class ScratchPad : public juce::Component
    {
    public:
        ScratchPad()
        {
            setSize(200, 200);
        }
        
        void paint(juce::Graphics& g) override
        {
            auto bounds = getLocalBounds().toFloat();
            
            // Draw background
            g.setColour(juce::Colours::darkgrey);
            g.fillRoundedRectangle(bounds, 10.0f);
            
            // Draw grid lines
            g.setColour(juce::Colours::grey);
            g.drawLine(bounds.getWidth() * 0.5f, 0, bounds.getWidth() * 0.5f, bounds.getHeight(), 1.0f);
            g.drawLine(0, bounds.getHeight() * 0.5f, bounds.getWidth(), bounds.getHeight() * 0.5f, 1.0f);
            
            // Draw current position
            g.setColour(juce::Colours::white);
            auto dotSize = 10.0f;
            g.fillEllipse(currentPosition.x * bounds.getWidth() - dotSize * 0.5f,
                         currentPosition.y * bounds.getHeight() - dotSize * 0.5f,
                         dotSize, dotSize);
        }
        
        void mouseDown(const juce::MouseEvent& e) override
        {
            updatePosition(e);
            isDragging = true;
        }
        
        void mouseDrag(const juce::MouseEvent& e) override
        {
            updatePosition(e);
        }
        
        void mouseUp(const juce::MouseEvent&) override
        {
            isDragging = false;
            springBackToCenter();
        }
        
        juce::Point<float> getCurrentPosition() const { return currentPosition; }
        
        std::function<void(float, float)> onPositionChange;
        
    private:
        void updatePosition(const juce::MouseEvent& e)
        {
            auto bounds = getLocalBounds().toFloat();
            currentPosition.x = juce::jlimit(0.0f, 1.0f, e.position.x / bounds.getWidth());
            currentPosition.y = juce::jlimit(0.0f, 1.0f, e.position.y / bounds.getHeight());
            
            // Convert to -1 to 1 range for X (scratch) and 0 to 1 for Y (depth)
            float scratchValue = (currentPosition.x - 0.5f) * 2.0f;
            float depthValue = 1.0f - currentPosition.y; // Invert Y so up = more depth
            
            if (onPositionChange)
                onPositionChange(scratchValue, depthValue);
                
            repaint();
        }
        
        void springBackToCenter()
        {
            auto& animator = juce::Desktop::getInstance().getAnimator();
            
            animator.animateComponent(this, getBounds(),
                                   1.0f, 200, false, 0.5, 0.5);
            
            currentPosition = { 0.5f, 0.5f };
            if (onPositionChange)
                onPositionChange(0.0f, 0.5f);
                
            repaint();
        }
        
        juce::Point<float> currentPosition { 0.5f, 0.5f };
        bool isDragging = false;
    };
    
    std::unique_ptr<ScratchPad> scratchPad;
    
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
#pragma once

#include <JuceHeader.h>
#include "GamepadManager.h"

class ControllerMappingComponent : public juce::Component
{
public:
    ControllerMappingComponent();
    ~ControllerMappingComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    struct ControllerMapping
    {
        int buttonId;
        juce::String actionName;
        bool isAxis;
    };

private:
    void drawPS5Controller(juce::Graphics& g);
    void drawButton(juce::Graphics& g, juce::Point<float> center, float radius, 
                   const juce::String& label, bool isHighlighted);
    
    juce::TextButton mappingButton {"Controller Mapping"};
    juce::DialogWindow* mappingDialog = nullptr;
    
    std::vector<ControllerMapping> mappings;
    
    // Store button positions for hit detection
    struct ButtonPosition
    {
        juce::Point<float> center;
        float radius;
        juce::String name;
        int buttonId;
    };
    
    std::vector<ButtonPosition> buttonPositions;
    
    void drawButton(juce::Graphics& g, 
                    juce::Point<float> center,
                    float radius,
                    const juce::String& symbol,
                    int buttonId,
                    const juce::String& mapping);
                    
    juce::Colour getButtonColor(int buttonId);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ControllerMappingComponent)
}; 
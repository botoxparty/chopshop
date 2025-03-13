#pragma once


#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "GamepadManager.h"

// Forward declare the window class
class ControllerMappingWindow;

class ControllerMappingComponent : public juce::Component,
                                  private juce::Timer
{
public:
    ControllerMappingComponent();
    ~ControllerMappingComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    
    // Add timer callback
    void timerCallback() override;

    struct ControllerMapping
    {
        int buttonId;
        juce::String actionName;
        bool isAxis;
    };

    void drawMappingsList(juce::Graphics& g, juce::Rectangle<float> bounds);

private:
    void drawButton(juce::Graphics& g, juce::Point<float> center, float radius, 
                   const juce::String& label, bool isHighlighted);
    
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
    
    void drawTouchpad(juce::Graphics& g, juce::Rectangle<float> bounds);
    void drawAnalogSticks(juce::Graphics& g, juce::Rectangle<float> bounds);
    void drawDpad(juce::Graphics& g, juce::Rectangle<float> bounds);
    void drawTriggers(juce::Graphics& g, juce::Rectangle<float> bounds);
    
    bool isControllerConnected = false;
    juce::String connectedControllerName;
    
    void checkControllerConnection();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ControllerMappingComponent)
};

// Add new window class
class ControllerMappingWindow : public juce::DocumentWindow
{
public:
    ControllerMappingWindow() 
        : DocumentWindow("Controller Mapping", 
                        juce::Desktop::getInstance().getDefaultLookAndFeel()
                            .findColour(juce::ResizableWindow::backgroundColourId),
                        DocumentWindow::closeButton)
    {
        mappingComponent = std::make_unique<ControllerMappingComponent>();
        setContentOwned(mappingComponent.get(), true);
        setUsingNativeTitleBar(true);
        setResizable(false, false);
        centreWithSize(400, 420);
    }

    void closeButtonPressed() override
    {
        setVisible(false);
    }

private:
    std::unique_ptr<ControllerMappingComponent> mappingComponent;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ControllerMappingWindow)
}; 
#include "ControllerMappingComponent.h"
#include "CustomLookAndFeel.h"

ControllerMappingComponent::ControllerMappingComponent()
{
    // Initialize default mappings with updated functions
    mappings = {
        {SDL_GAMEPAD_BUTTON_SOUTH, "Chop", false},
        {SDL_GAMEPAD_BUTTON_EAST, "Load Audio", false},
        {SDL_GAMEPAD_BUTTON_WEST, "Stop", false},
        {SDL_GAMEPAD_BUTTON_NORTH, "Play/Pause", false},
        {SDL_GAMEPAD_BUTTON_DPAD_UP, "Reverb", false},
        {SDL_GAMEPAD_BUTTON_DPAD_RIGHT, "Delay", false},
        {SDL_GAMEPAD_BUTTON_DPAD_DOWN, "Flanger", false},
        {SDL_GAMEPAD_BUTTON_LEFT_SHOULDER, "Vinyl Brake", false},
        {SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER, "Screw", false},
        {SDL_GAMEPAD_AXIS_LEFTX, "Flanger Rate", true},
        {SDL_GAMEPAD_AXIS_LEFTY, "Flanger Depth", true},
        {SDL_GAMEPAD_AXIS_RIGHTX, "Phaser Rate", true},
        {SDL_GAMEPAD_AXIS_RIGHTY, "Phaser Depth", true},
        {SDL_GAMEPAD_AXIS_RIGHT_TRIGGER, "Vinyl Brake", true}
    };

    // Initialize button positions with updated layout
    buttonPositions = {
        {{300, 250}, 15, "Cross", SDL_GAMEPAD_BUTTON_SOUTH},
        {{340, 210}, 15, "Circle", SDL_GAMEPAD_BUTTON_EAST},
        {{260, 210}, 15, "Square", SDL_GAMEPAD_BUTTON_WEST},
        {{300, 170}, 15, "Triangle", SDL_GAMEPAD_BUTTON_NORTH}
    };

    // Start timer to check connection status
    startTimer(100);
}

ControllerMappingComponent::~ControllerMappingComponent()
{
    stopTimer();
}

void ControllerMappingComponent::resized()
{
    // No longer need to position the mapping button
}

void ControllerMappingComponent::paint(juce::Graphics& g)
{
    // Check controller connection before drawing
    checkControllerConnection();
    
    // Draw background
    auto bounds = getLocalBounds().toFloat().reduced(10);
    g.setColour(juce::Colour(0xFF121212));   // Dark background
    g.fillRoundedRectangle(bounds, 8.0f);
    
    // Draw connection status panel at the top
    auto statusBounds = bounds.removeFromTop(50);
    
    // Draw status background
    g.setColour(juce::Colour(0xFF1E1E1E));   // Surface color
    g.fillRoundedRectangle(statusBounds, 6.0f);
    
    // Draw subtle border
    g.setColour(juce::Colour(0xFF505050).withAlpha(0.5f));     // Medium gray with transparency
    g.drawRoundedRectangle(statusBounds, 6.0f, 1.0f);

    // Draw connection status text
    g.setColour(juce::Colours::white);
    g.setFont(CustomLookAndFeel::getCustomFont().withHeight(16.0f));
    
    juce::String statusText;
    if (isControllerConnected)
    {
        statusText = "Controller Connected: " + connectedControllerName;
    }
    else
    {
        statusText = "No Controller Connected - Please Connect Your Game Controller";
        g.setColour(juce::Colours::white.withAlpha(0.7f));  // Dimmer color for warning state
    }
    
    g.drawText(statusText, statusBounds.reduced(10, 0), juce::Justification::centred, true);
    
    // Draw mappings list
    drawMappingsList(g, bounds);
}

void ControllerMappingComponent::drawTouchpad(juce::Graphics& g, juce::Rectangle<float> bounds)
{
    const float touchpadWidth = bounds.getWidth() * 0.4f;
    const float touchpadHeight = bounds.getHeight() * 0.6f;
    auto touchpadBounds = juce::Rectangle<float>(
        bounds.getCentreX() - touchpadWidth / 2,
        bounds.getCentreY() - touchpadHeight / 2,
        touchpadWidth,
        touchpadHeight
    );
    
    g.setColour(juce::Colours::darkgrey);
    g.fillRoundedRectangle(touchpadBounds, 5.0f);
    
    g.setColour(juce::Colours::white);
    g.setFont(12.0f);
    g.drawText("Phaser Controls:", touchpadBounds.removeFromTop(20), 
               juce::Justification::centred);
    g.drawText("X: Depth | Y: Rate | Diagonal: Feedback",
               touchpadBounds.removeFromBottom(20), 
               juce::Justification::centred);
}

void ControllerMappingComponent::drawAnalogSticks(juce::Graphics& g, juce::Rectangle<float> bounds)
{
    const float stickRadius = bounds.getHeight() * 0.3f;
    
    // Left stick
    g.setColour(juce::Colours::black);
    g.fillEllipse(bounds.getX() + bounds.getWidth() * 0.25f - stickRadius,
                  bounds.getCentreY() - stickRadius,
                  stickRadius * 2, stickRadius * 2);
                  
    // Right stick
    g.fillEllipse(bounds.getX() + bounds.getWidth() * 0.75f - stickRadius,
                  bounds.getCentreY() - stickRadius,
                  stickRadius * 2, stickRadius * 2);
}

void ControllerMappingComponent::drawButton(juce::Graphics& g, 
                                          juce::Point<float> center,
                                          float radius,
                                          const juce::String& symbol,
                                          int buttonId,
                                          const juce::String& mapping)
{
    if (symbol.isEmpty() || mapping.isEmpty())
        return;

    // Draw button background
    g.setColour(juce::Colours::black);
    g.fillEllipse(center.x - radius, center.y - radius, radius * 2, radius * 2);
    
    // Draw button symbol - use a standard font size instead of scaling with radius
    g.setColour(getButtonColor(buttonId));
    g.setFont(18.0f);
    g.drawText(symbol, 
               (int)(center.x - radius), (int)(center.y - radius), 
               (int)(radius * 2), (int)(radius * 2), 
               juce::Justification::centred, false);
               
    // Draw mapping text
    g.setColour(juce::Colours::white);
    g.setFont(12.0f);
    g.drawText(mapping,
               (int)(center.x - 40), (int)(center.y + radius + 5),
               80, 20,
               juce::Justification::centred, false);
}

juce::Colour ControllerMappingComponent::getButtonColor(int buttonId)
{
    switch (buttonId)
    {
        case SDL_GAMEPAD_BUTTON_SOUTH: return juce::Colours::lightblue;   // Cross
        case SDL_GAMEPAD_BUTTON_EAST: return juce::Colours::red;         // Circle
        case SDL_GAMEPAD_BUTTON_WEST: return juce::Colours::pink;        // Square
        case SDL_GAMEPAD_BUTTON_NORTH: return juce::Colours::green;       // Triangle
        default: return juce::Colours::grey;
    }
}

void ControllerMappingComponent::drawDpad(juce::Graphics& g, juce::Rectangle<float> bounds)
{
    const float dpadSize = bounds.getHeight() * 0.6f;
    const float buttonSize = dpadSize / 3.0f;
    
    // Calculate center position for D-pad
    float centerX = bounds.getCentreX();
    float centerY = bounds.getCentreY();
    
    // Draw D-pad buttons
    drawButton(g, {centerX, centerY - buttonSize}, buttonSize/2, "^", 
               SDL_GAMEPAD_BUTTON_DPAD_UP, "Reverb");
    drawButton(g, {centerX + buttonSize, centerY}, buttonSize/2, ">", 
               SDL_GAMEPAD_BUTTON_DPAD_RIGHT, "Delay");
    drawButton(g, {centerX, centerY + buttonSize}, buttonSize/2, "v", 
               SDL_GAMEPAD_BUTTON_DPAD_DOWN, "Flanger");
}

void ControllerMappingComponent::drawTriggers(juce::Graphics& g, juce::Rectangle<float> bounds)
{
    const float triggerWidth = bounds.getWidth() * 0.15f;
    const float triggerHeight = bounds.getHeight() * 0.3f;
    
    // Draw right trigger
    auto rightTriggerBounds = juce::Rectangle<float>(
        bounds.getRight() - triggerWidth - 10,
        bounds.getY(),
        triggerWidth,
        triggerHeight
    );
    
    g.setColour(juce::Colours::darkgrey);
    g.fillRoundedRectangle(rightTriggerBounds, 5.0f);
    
    g.setColour(juce::Colours::white);
    g.setFont(12.0f);
    g.drawText("R2", rightTriggerBounds.removeFromTop(20), 
               juce::Justification::centred);
    g.drawText("Vinyl Brake", rightTriggerBounds.removeFromBottom(20), 
               juce::Justification::centred);
}

void ControllerMappingComponent::checkControllerConnection()
{
    if (auto* gamepadManager = GamepadManager::getInstance())
    {
        isControllerConnected = gamepadManager->isGamepadConnected();
        connectedControllerName = gamepadManager->getConnectedGamepadName();
    }
    else
    {
        isControllerConnected = false;
        connectedControllerName = "";
    }
}

void ControllerMappingComponent::timerCallback()
{
    if (getTimerInterval() != 500)
    {
        stopTimer();
        startTimerHz(2);
    }
    
    bool wasConnected = isControllerConnected;
    checkControllerConnection();
    
    if (wasConnected != isControllerConnected)
    {
        repaint();
    }
}

void ControllerMappingComponent::drawMappingsList(juce::Graphics& g, juce::Rectangle<float> bounds)
{
    // Draw list background
    g.setColour(juce::Colour(0xFF1E1E1E));   // Surface color
    g.fillRoundedRectangle(bounds, 8.0f);
    
    // Draw subtle border
    g.setColour(juce::Colour(0xFF505050).withAlpha(0.5f));     // Medium gray with transparency
    g.drawRoundedRectangle(bounds, 8.0f, 1.0f);
    
    // Draw title
    g.setColour(juce::Colours::white);
    g.setFont(CustomLookAndFeel::getCustomFont().withHeight(14.0f));
    auto titleBounds = bounds.removeFromTop(25.0f);
    g.drawText("Controller Mappings", titleBounds, juce::Justification::centred, false);
    
    // Draw separator line
    g.setColour(juce::Colour(0xFF505050));   // Medium gray
    g.drawLine(bounds.getX() + 10.0f, titleBounds.getBottom(), bounds.getRight() - 10.0f, titleBounds.getBottom(), 1.0f);
    
    // Create list layout
    auto listArea = bounds.reduced(10);
    int rowHeight = 20;
    int currentY = static_cast<int>(listArea.getY());
    
    g.setFont(CustomLookAndFeel::getCustomFont().withHeight(12.0f));
    
    // Helper lambda for drawing rows
    auto drawRow = [&](const juce::String& control, const juce::String& action) {
        int width = static_cast<int>(listArea.getWidth());
        int x = static_cast<int>(listArea.getX());
        
        // Draw control name
        g.setColour(juce::Colours::white);
        g.drawText(control, x, currentY, width, rowHeight, 
                  juce::Justification::left, false);
        
        // Draw action name with a modern pill background
        auto actionBounds = juce::Rectangle<float>(
            listArea.getX() + listArea.getWidth() * 0.4f, currentY,
            listArea.getWidth() * 0.6f - 5.0f, rowHeight - 2.0f);
        
        // Draw pill background
        g.setColour(juce::Colour(0xFF505050));   // Medium gray
        g.fillRoundedRectangle(actionBounds, rowHeight / 2.0f);
        
        // Draw action text
        g.setColour(juce::Colours::white);
        g.drawText(action, actionBounds, juce::Justification::centred, false);
        
        currentY += rowHeight;
    };

    // Draw mappings
    for (const auto& mapping : mappings)
    {
        juce::String controlName;
        
        if (mapping.isAxis)
        {
            switch (mapping.buttonId)
            {
                case SDL_GAMEPAD_AXIS_LEFTX: controlName = "Left Stick X"; break;
                case SDL_GAMEPAD_AXIS_RIGHTX: controlName = "Right Stick X"; break;
                case SDL_GAMEPAD_AXIS_RIGHTY: controlName = "Right Stick Y"; break;
                case SDL_GAMEPAD_AXIS_RIGHT_TRIGGER: controlName = "R2"; break;
                default: controlName = "Unknown Axis"; break;
            }
        }
        else
        {
            switch (mapping.buttonId)
            {
                case SDL_GAMEPAD_BUTTON_SOUTH: controlName = "Cross (A)"; break;
                case SDL_GAMEPAD_BUTTON_EAST: controlName = "Circle (B)"; break;
                case SDL_GAMEPAD_BUTTON_WEST: controlName = "Square (X)"; break;
                case SDL_GAMEPAD_BUTTON_NORTH: controlName = "Triangle (Y)"; break;
                case SDL_GAMEPAD_BUTTON_DPAD_UP: controlName = "D-Pad Up"; break;
                case SDL_GAMEPAD_BUTTON_DPAD_RIGHT: controlName = "D-Pad Right"; break;
                case SDL_GAMEPAD_BUTTON_DPAD_DOWN: controlName = "D-Pad Down"; break;
                case SDL_GAMEPAD_BUTTON_LEFT_SHOULDER: controlName = "L1"; break;
                case SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER: controlName = "R1"; break;
                default: controlName = "Unknown Button"; break;
            }
        }
        
        drawRow(controlName, mapping.actionName);
    }
} 
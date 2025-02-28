#include "ControllerMappingComponent.h"
#include "CustomLookAndFeel.h"

namespace {
    juce::Font getCustomFont()
    {
        return juce::Font(16.0f); // Default font as fallback
    }
}

ControllerMappingComponent::ControllerMappingComponent()
{
    addAndMakeVisible(mappingButton);
    
    mappingButton.onClick = [this]()
    {
        if (mappingDialog == nullptr)
        {
            // Create a new instance of ControllerMappingComponent for the dialog
            auto content = std::make_unique<ControllerMappingComponent>();
            content->setSize(400, 420);
            content->removeChildComponent(&content->mappingButton); // Remove the button from the dialog version
            
            juce::DialogWindow::LaunchOptions options;
            options.content.setOwned(content.release());
            options.dialogTitle = "Controller Mapping";
            options.dialogBackgroundColour = getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId);
            options.escapeKeyTriggersCloseButton = true;
            options.useNativeTitleBar = true;
            options.resizable = false;
            
            mappingDialog = options.launchAsync();
            mappingDialog->centreWithSize(400, 420);
            mappingDialog->addComponentListener(new ComponentListener(*this));
        }
    };

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
    
    // Don't check connection status in constructor
    // Instead, we'll do it in the first timer callback
    updateButtonAppearance();

    // Start timer to check connection status immediately and then periodically
    startTimer(100); // First check after 100ms, then will switch to regular interval
}

ControllerMappingComponent::~ControllerMappingComponent()
{
    stopTimer();
    
    if (mappingDialog != nullptr)
        mappingDialog->exitModalState(0);
}

void ControllerMappingComponent::paint(juce::Graphics& g)
{
    // Check controller connection before drawing
    checkControllerConnection();
    
    // Draw background
    auto bounds = getLocalBounds().toFloat().reduced(10);
    g.setColour(juce::Colours::darkgrey.darker(0.2f));
    g.fillRoundedRectangle(bounds, 8.0f);
    
    // Draw connection status panel at the top
    auto statusBounds = bounds.removeFromTop(50);
    
    // Draw status background with metallic gradient matching app style
    const auto matrixGreen = juce::Colour(0xFF00FF41);    // Classic Winamp green
    const auto metalGrey = juce::Colour(0xFF2A2A2A);      // Dark metallic
    const auto metalLight = juce::Colour(0xFF3D3D3D);     // Light metallic
    
    juce::ColourGradient gradient(
        metalLight,
        statusBounds.getX(), statusBounds.getY(),
        metalGrey,
        statusBounds.getX(), statusBounds.getBottom(),
        false);
    
    g.setGradientFill(gradient);
    g.fillRoundedRectangle(statusBounds, 6.0f);
    
    // Add metallic edge effects
    g.setColour(juce::Colours::white.withAlpha(0.1f));
    g.drawLine(statusBounds.getX(), statusBounds.getY(), statusBounds.getRight(), statusBounds.getY(), 1.0f);
    g.drawLine(statusBounds.getX(), statusBounds.getY(), statusBounds.getX(), statusBounds.getBottom(), 1.0f);
    
    g.setColour(juce::Colours::black.withAlpha(0.2f));
    g.drawLine(statusBounds.getX(), statusBounds.getBottom(), statusBounds.getRight(), statusBounds.getBottom(), 1.0f);
    g.drawLine(statusBounds.getRight(), statusBounds.getY(), statusBounds.getRight(), statusBounds.getBottom(), 1.0f);
    
    // Draw connection icon
    float iconSize = 20.0f;
    float iconX = statusBounds.getX() + 15.0f;
    float iconY = statusBounds.getCentreY() - iconSize / 2.0f;
    
    if (isControllerConnected) {
        // Draw connected icon (gamepad with glow)
        g.setColour(juce::Colours::black);
        g.fillRoundedRectangle(iconX, iconY, iconSize, iconSize, 4.0f);
        
        // Draw glow effect
        for (int i = 0; i < 3; ++i) {
            g.setColour(matrixGreen.withAlpha(0.1f - i * 0.03f));
            g.drawRoundedRectangle(iconX - i, iconY - i, 
                                  iconSize + i * 2, iconSize + i * 2, 
                                  4.0f, 1.0f);
        }
        
        // Draw simple gamepad shape
        g.setColour(matrixGreen);
        g.fillRoundedRectangle(iconX + 3.0f, iconY + 5.0f, iconSize - 6.0f, iconSize - 10.0f, 2.0f);
        g.fillRoundedRectangle(iconX + 2.0f, iconY + 3.0f, 5.0f, 3.0f, 1.0f);
        g.fillRoundedRectangle(iconX + iconSize - 7.0f, iconY + 3.0f, 5.0f, 3.0f, 1.0f);
    } else {
        // Draw disconnected icon (gamepad with X)
        g.setColour(juce::Colours::black);
        g.fillRoundedRectangle(iconX, iconY, iconSize, iconSize, 4.0f);
        
        // Draw border
        g.setColour(juce::Colours::red.darker(0.2f));
        g.drawRoundedRectangle(iconX, iconY, iconSize, iconSize, 4.0f, 1.0f);
        
        // Draw X
        g.setColour(juce::Colours::red.darker(0.2f));
        g.drawLine(iconX + 5.0f, iconY + 5.0f, iconX + iconSize - 5.0f, iconY + iconSize - 5.0f, 2.0f);
        g.drawLine(iconX + iconSize - 5.0f, iconY + 5.0f, iconX + 5.0f, iconY + iconSize - 5.0f, 2.0f);
    }
    
    // Draw status text
    // g.setFont(getCustomFont().withHeight(16.0f));
    
    juce::String statusText = isControllerConnected 
        ? "Connected: " + connectedControllerName
        : "No controller connected";
    
    if (isControllerConnected) {
        g.setColour(matrixGreen);
    } else {
        g.setColour(juce::Colours::red.brighter(0.2f));
    }
    
    g.drawText(statusText, 
               statusBounds.reduced(iconSize + 25.0f, 0).withTrimmedLeft(5.0f), 
               juce::Justification::centredLeft, false);
    
    // Draw the controller visualization
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

void ControllerMappingComponent::resized()
{
    auto bounds = getLocalBounds();
    mappingButton.setBounds(bounds.removeFromTop(30));
}

// Add this helper class to handle the window closing
class ComponentListener : public juce::ComponentListener
{
public:
    ComponentListener(ControllerMappingComponent& owner) : owner_(owner) {}
    
    void componentBeingDeleted(juce::Component& component) override
    {
        owner_.mappingDialog = nullptr;
        delete this; // Clean up the listener
    }
    
private:
    ControllerMappingComponent& owner_;
};

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
    // This assumes you have access to the GamepadManager
    // You might need to pass it in or make it accessible
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

// Add timer callback
void ControllerMappingComponent::timerCallback()
{
    // If this is the first timer callback, switch to regular interval
    if (getTimerInterval() != 500)
    {
        stopTimer();
        startTimerHz(2); // Check twice per second
    }
    
    bool wasConnected = isControllerConnected;
    checkControllerConnection();
    
    // Update button appearance if connection status changed
    if (wasConnected != isControllerConnected)
    {
        updateButtonAppearance();
        repaint();
    }
    else
    {
        repaint(); // Still repaint for other updates
    }
}

void ControllerMappingComponent::updateButtonAppearance()
{
    // Define a neon green color to match the app style
    juce::Colour matrixGreen(0xFF00FF41);
    
    if (isControllerConnected)
    {
        mappingButton.setButtonText("Game Controller: Connected");
        mappingButton.setColour(juce::TextButton::textColourOffId, matrixGreen);
    }
    else
    {
        mappingButton.setButtonText("No Controller Connected");
        mappingButton.setColour(juce::TextButton::textColourOffId, juce::Colours::red.brighter(0.2f));
    }
    
    // Force a repaint of the button
    mappingButton.repaint();
}

void ControllerMappingComponent::drawMappingsList(juce::Graphics& g, juce::Rectangle<float> bounds)
{
    // Draw list background with metallic gradient
    const auto metalGrey = juce::Colour(0xFF2A2A2A);      // Dark metallic
    const auto metalLight = juce::Colour(0xFF3D3D3D);     // Light metallic
    
    juce::ColourGradient gradient(
        metalLight,
        bounds.getX(), bounds.getY(),
        metalGrey,
        bounds.getX(), bounds.getBottom(),
        false);
    
    g.setGradientFill(gradient);
    g.fillRoundedRectangle(bounds, 8.0f);
    
    // Add metallic edge effects
    g.setColour(juce::Colours::white.withAlpha(0.1f));
    g.drawLine(bounds.getX(), bounds.getY(), bounds.getRight(), bounds.getY(), 1.0f);
    g.drawLine(bounds.getX(), bounds.getY(), bounds.getX(), bounds.getBottom(), 1.0f);
    
    g.setColour(juce::Colours::black.withAlpha(0.2f));
    g.drawLine(bounds.getX(), bounds.getBottom(), bounds.getRight(), bounds.getBottom(), 1.0f);
    g.drawLine(bounds.getRight(), bounds.getY(), bounds.getRight(), bounds.getBottom(), 1.0f);
    
    // Draw title
    const auto matrixGreen = juce::Colour(0xFF00FF41);
    g.setColour(matrixGreen);
    // g.setFont(getCustomFont().withHeight(14.0f).boldened());
    auto titleBounds = bounds.removeFromTop(25.0f);
    g.drawText("Controller Mappings", titleBounds, juce::Justification::centred, false);
    
    // Draw separator line
    g.setColour(matrixGreen.withAlpha(0.3f));
    g.drawLine(bounds.getX() + 10.0f, titleBounds.getBottom(), bounds.getRight() - 10.0f, titleBounds.getBottom(), 1.0f);
    
    // Create list layout
    auto listArea = bounds.reduced(10);
    float rowHeight = 20.0f;
    float currentY = listArea.getY();
    
    // g.setFont(getCustomFont().withHeight(12.0f));
    
    // Helper lambda for drawing rows
    auto drawRow = [&](const juce::String& control, const juce::String& action) {
        // Draw control name
        g.setColour(matrixGreen);
        g.drawText(control, listArea.getX(), currentY, listArea.getWidth() * 0.4f, rowHeight, 
                  juce::Justification::left, false);
        
        // Draw action name with a pill background
        auto actionBounds = juce::Rectangle<float>(
            listArea.getX() + listArea.getWidth() * 0.4f, currentY,
            listArea.getWidth() * 0.6f - 5.0f, rowHeight - 2.0f);
        
        // Draw pill with glow effect
        g.setColour(juce::Colours::black);
        g.fillRoundedRectangle(actionBounds, rowHeight / 2.0f);
        
        for (int i = 0; i < 2; ++i) {
            g.setColour(matrixGreen.withAlpha(0.1f - i * 0.03f));
            g.drawRoundedRectangle(actionBounds.expanded(i), rowHeight / 2.0f, 1.0f);
        }
        
        g.setColour(matrixGreen);
        g.drawText(action, actionBounds, juce::Justification::centred, false);
        
        currentY += rowHeight;
    };
    
    // Draw headers
    g.setColour(matrixGreen.brighter(0.2f));
    // Draw mappings (just show a few to avoid cluttering)
    g.setColour(matrixGreen);
    int count = 0;
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
        count++;
    }
} 
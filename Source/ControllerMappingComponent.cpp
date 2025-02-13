#include "ControllerMappingComponent.h"

ControllerMappingComponent::ControllerMappingComponent()
{
    addAndMakeVisible(mappingButton);
    
    mappingButton.onClick = [this]()
    {
        if (mappingDialog == nullptr)
        {
            // Create a new instance of ControllerMappingComponent for the dialog
            auto content = std::make_unique<ControllerMappingComponent>();
            content->setSize(400, 400);
            content->removeChildComponent(&content->mappingButton); // Remove the button from the dialog version
            
            juce::DialogWindow::LaunchOptions options;
            options.content.setOwned(content.release());
            options.dialogTitle = "Controller Mapping";
            options.dialogBackgroundColour = getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId);
            options.escapeKeyTriggersCloseButton = true;
            options.useNativeTitleBar = false;
            options.resizable = false;
            
            mappingDialog = options.launchAsync();
            mappingDialog->centreWithSize(400, 400);
            mappingDialog->addComponentListener(new ComponentListener(*this));
        }
    };

    // Initialize default mappings with updated functions
    mappings = {
        {SDL_CONTROLLER_BUTTON_A, "Chop", false},
        {SDL_CONTROLLER_BUTTON_B, "Load Audio", false},
        {SDL_CONTROLLER_BUTTON_X, "Stop", false},
        {SDL_CONTROLLER_BUTTON_Y, "Play/Pause", false},
        {SDL_CONTROLLER_BUTTON_DPAD_UP, "Reverb", false},
        {SDL_CONTROLLER_BUTTON_DPAD_RIGHT, "Delay", false},
        {SDL_CONTROLLER_BUTTON_DPAD_DOWN, "Flanger", false},
        {SDL_CONTROLLER_BUTTON_LEFTSHOULDER, "Vinyl Brake", false},
        {SDL_CONTROLLER_BUTTON_RIGHTSHOULDER, "Screw", false},
        {SDL_CONTROLLER_AXIS_LEFTX, "Flanger Rate", true},
        {SDL_CONTROLLER_AXIS_LEFTY, "Flanger Depth", true},
        {SDL_CONTROLLER_AXIS_RIGHTX, "Phaser Rate", true},
        {SDL_CONTROLLER_AXIS_RIGHTY, "Phaser Depth", true},
        {SDL_CONTROLLER_AXIS_TRIGGERRIGHT, "Vinyl Brake", true}
    };

    // Initialize button positions with updated layout
    buttonPositions = {
        {{300, 250}, 15, "Cross", SDL_CONTROLLER_BUTTON_A},
        {{340, 210}, 15, "Circle", SDL_CONTROLLER_BUTTON_B},
        {{260, 210}, 15, "Square", SDL_CONTROLLER_BUTTON_X},
        {{300, 170}, 15, "Triangle", SDL_CONTROLLER_BUTTON_Y}
    };
}

ControllerMappingComponent::~ControllerMappingComponent()
{
    if (mappingDialog != nullptr)
        mappingDialog->exitModalState(0);
}

void ControllerMappingComponent::paint(juce::Graphics& g)
{
    drawPS5Controller(g);
}

void ControllerMappingComponent::drawPS5Controller(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced(10);  // Reduced padding
    
    // Draw background
    g.setColour(juce::Colours::darkgrey);
    g.fillRoundedRectangle(bounds, 5.0f);  // Smaller corner radius
    
    // Title
    g.setColour(juce::Colours::white);
    g.setFont(14.0f);  // Smaller font
    g.drawText("Controller Mapping", bounds.removeFromTop(20), juce::Justification::centred, false);
    
    // Create list layout
    auto listArea = bounds.reduced(10);  // Smaller padding
    float rowHeight = 20.0f;  // Smaller row height
    float currentY = listArea.getY();
    
    g.setFont(12.0f);  // Smaller font for list
    
    // Helper lambda for drawing rows
    auto drawRow = [&](const juce::String& control, const juce::String& action) {
        g.drawText(control, listArea.getX(), currentY, listArea.getWidth() * 0.4f, rowHeight, 
                  juce::Justification::left, false);
        g.drawText(action, listArea.getX() + listArea.getWidth() * 0.4f, currentY, 
                  listArea.getWidth() * 0.6f, rowHeight, juce::Justification::left, false);
        currentY += rowHeight;
    };
    
    // Draw headers
    g.setColour(juce::Colours::yellow);
    drawRow("Control", "Action");
    
    // Draw mappings
    g.setColour(juce::Colours::white);
    for (const auto& mapping : mappings)
    {
        juce::String controlName;
        
        if (mapping.isAxis)
        {
            switch (mapping.buttonId)
            {
                case SDL_CONTROLLER_AXIS_LEFTX: controlName = "Left Stick X"; break;
                case SDL_CONTROLLER_AXIS_RIGHTX: controlName = "Right Stick X"; break;
                case SDL_CONTROLLER_AXIS_RIGHTY: controlName = "Right Stick Y"; break;
                case SDL_CONTROLLER_AXIS_TRIGGERRIGHT: controlName = "R2"; break;
                default: controlName = "Unknown Axis"; break;
            }
        }
        else
        {
            switch (mapping.buttonId)
            {
                case SDL_CONTROLLER_BUTTON_A: controlName = "Cross (A)"; break;
                case SDL_CONTROLLER_BUTTON_B: controlName = "Circle (B)"; break;
                case SDL_CONTROLLER_BUTTON_X: controlName = "Square (X)"; break;
                case SDL_CONTROLLER_BUTTON_Y: controlName = "Triangle (Y)"; break;
                case SDL_CONTROLLER_BUTTON_DPAD_UP: controlName = "D-Pad Up"; break;
                case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: controlName = "D-Pad Right"; break;
                case SDL_CONTROLLER_BUTTON_DPAD_DOWN: controlName = "D-Pad Down"; break;
                case SDL_CONTROLLER_BUTTON_LEFTSHOULDER: controlName = "L1"; break;
                case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER: controlName = "R1"; break;
                default: controlName = "Unknown Button"; break;
            }
        }
        
        drawRow(controlName, mapping.actionName);
    }
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
        case SDL_CONTROLLER_BUTTON_A: return juce::Colours::lightblue;   // Cross
        case SDL_CONTROLLER_BUTTON_B: return juce::Colours::red;         // Circle
        case SDL_CONTROLLER_BUTTON_X: return juce::Colours::pink;        // Square
        case SDL_CONTROLLER_BUTTON_Y: return juce::Colours::green;       // Triangle
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
               SDL_CONTROLLER_BUTTON_DPAD_UP, "Reverb");
    drawButton(g, {centerX + buttonSize, centerY}, buttonSize/2, ">", 
               SDL_CONTROLLER_BUTTON_DPAD_RIGHT, "Delay");
    drawButton(g, {centerX, centerY + buttonSize}, buttonSize/2, "v", 
               SDL_CONTROLLER_BUTTON_DPAD_DOWN, "Flanger");
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
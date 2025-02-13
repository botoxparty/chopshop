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
            content->setSize(600, 400);
            content->removeChildComponent(&content->mappingButton); // Remove the button from the dialog version
            
            juce::DialogWindow::LaunchOptions options;
            options.content.setOwned(content.release());
            options.dialogTitle = "Controller Mapping";
            options.dialogBackgroundColour = getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId);
            options.escapeKeyTriggersCloseButton = true;
            options.useNativeTitleBar = false;
            options.resizable = false;
            
            mappingDialog = options.launchAsync();
            mappingDialog->centreWithSize(600, 400);
            mappingDialog->addComponentListener(new ComponentListener(*this));
        }
    };

    // Initialize default mappings
    mappings = {
        {SDL_CONTROLLER_BUTTON_A, "Chop", false},
        {SDL_CONTROLLER_BUTTON_B, "Load Audio", false},
        {SDL_CONTROLLER_BUTTON_X, "Stop", false},
        {SDL_CONTROLLER_BUTTON_Y, "Play/Pause", false},
        {SDL_CONTROLLER_AXIS_LEFTX, "Crossfader", true}
    };

    // Initialize button positions
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
    // Draw controller body
    g.setColour(juce::Colours::darkgrey);
    auto bounds = getLocalBounds().toFloat().reduced(20);
    g.fillRoundedRectangle(bounds, 10.0f);
    
    g.setColour(juce::Colours::grey);
    g.drawRoundedRectangle(bounds, 10.0f, 2.0f);

    // Draw face buttons
    const float buttonRadius = 15.0f;
    const float buttonSpacing = 40.0f;
    const float centerX = bounds.getCentreX();
    const float centerY = bounds.getCentreY() + 50.0f;

    // Cross button (A)
    drawButton(g, {centerX, centerY + buttonSpacing}, buttonRadius, "X", 
               SDL_CONTROLLER_BUTTON_A, "Chop");

    // Circle button (B)
    drawButton(g, {centerX + buttonSpacing, centerY}, buttonRadius, "O", 
               SDL_CONTROLLER_BUTTON_B, "Load");

    // Square button (X)
    drawButton(g, {centerX - buttonSpacing, centerY}, buttonRadius, "[]", 
               SDL_CONTROLLER_BUTTON_X, "Stop");

    // Triangle button (Y)
    drawButton(g, {centerX, centerY - buttonSpacing}, buttonRadius, "/\\", 
               SDL_CONTROLLER_BUTTON_Y, "Play");

    // Draw D-pad
    const float dpadSize = 80.0f;
    const float dpadX = centerX - 100.0f;
    const float dpadY = centerY + 50.0f;
    g.setColour(juce::Colours::darkgrey);
    g.fillRoundedRectangle(dpadX - dpadSize/2, dpadY - dpadSize/2, dpadSize, dpadSize, 5.0f);

    // Draw analog sticks
    const float stickRadius = 25.0f;
    g.setColour(juce::Colours::black);
    g.fillEllipse(centerX - 120.0f - stickRadius, centerY - stickRadius, stickRadius * 2, stickRadius * 2);
    g.fillEllipse(centerX + 80.0f - stickRadius, centerY - stickRadius, stickRadius * 2, stickRadius * 2);

    // Draw shoulder buttons
    const float shoulderWidth = 60.0f;
    const float shoulderHeight = 20.0f;
    g.setColour(juce::Colours::darkgrey);
    
    // L1/R1
    g.fillRoundedRectangle(bounds.getX() + 20, bounds.getY() + 20, shoulderWidth, shoulderHeight, 5.0f);
    g.fillRoundedRectangle(bounds.getRight() - shoulderWidth - 20, bounds.getY() + 20, shoulderWidth, shoulderHeight, 5.0f);
    
    // L2/R2
    g.fillRoundedRectangle(bounds.getX() + 20, bounds.getY(), shoulderWidth, shoulderHeight, 5.0f);
    g.fillRoundedRectangle(bounds.getRight() - shoulderWidth - 20, bounds.getY(), shoulderWidth, shoulderHeight, 5.0f);

    // Draw touchpad
    const float touchpadWidth = 120.0f;
    const float touchpadHeight = 60.0f;
    const float touchpadX = bounds.getCentreX() - touchpadWidth / 2;
    const float touchpadY = bounds.getCentreY() - 20;
    
    g.setColour(juce::Colours::darkgrey);
    g.fillRoundedRectangle(touchpadX, touchpadY, touchpadWidth, touchpadHeight, 5.0f);
    
    // Draw touchpad mapping text
    g.setColour(juce::Colours::white);
    g.setFont(12.0f);
    g.drawText("Phaser Controls:", 
               touchpadX, touchpadY - 20, 
               touchpadWidth, 20, 
               juce::Justification::centred);
               
    g.drawText("X: Depth | Y: Rate | Diagonal: Feedback",
               touchpadX, touchpadY + touchpadHeight, 
               touchpadWidth, 20, 
               juce::Justification::centred);

    // Draw labels
    g.setColour(juce::Colours::white);
    g.setFont(16.0f);
    auto labelBounds = bounds.removeFromTop(30).toNearestInt();
    g.drawText("Controller Mapping", labelBounds, juce::Justification::centred, false);
    
    // Draw instructions
    g.setFont(12.0f);
    auto instructionBounds = bounds.removeFromBottom(30).toNearestInt();
    g.drawText("Click a button to remap its function", instructionBounds, juce::Justification::centred, false);
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
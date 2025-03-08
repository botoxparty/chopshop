#include "ControllerMappingComponent.h"
#include "CustomLookAndFeel.h"

ControllerMappingComponent::ControllerMappingComponent()
{
    addAndMakeVisible(mappingButton);
    mappingButton.setButtonText(""); // Clear text since we're using an icon
    mappingButton.setTriggeredOnMouseDown(false);
    
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
    float iconX = statusBounds.getCentreX() - iconSize / 2.0f;
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

void ControllerMappingComponent::resized()
{
    auto bounds = getLocalBounds();
    // Give the button a proper size - make it 40x40 pixels
    mappingButton.setBounds(bounds.removeFromTop(40).withSizeKeepingCentre(40, 40));
}

// Add this helper class to handle the window closing
class ComponentListener : public juce::ComponentListener
{
public:
    ComponentListener(ControllerMappingComponent& owner) : owner_(owner) {}
    
    void componentBeingDeleted(juce::Component&) override
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

std::unique_ptr<juce::Drawable> ControllerMappingComponent::createControllerIcon(bool connected)
{
    auto path = std::make_unique<juce::DrawablePath>();
    
    // Create a gamepad shape using the SVG path
    juce::Path p;
    
    p.clear();
    p.restoreFromString(
        // Main controller shape
        "M220.728,168.956l-8.427-38.206c2.793-6.18,4.208-12.768,4.208-19.609c0-15.451-7.482-29.837-19.848-38.754v-4.24"
        "c0-8.236-6.701-14.937-14.938-14.937H160.98c-6.569,0-12.161,4.262-14.156,10.167h-31.708V23.921c0-2.761-2.239-5-5-5s-5,2.239-5,5"
        "v39.456H73.408c-1.995-5.904-7.587-10.167-14.156-10.167H38.508c-8.236,0-14.938,6.701-14.938,14.937v1.844"
        "C8.988,78.569,0.053,94.064,0.053,111.141c0,8.696,2.341,17.137,6.789,24.545L0.763,163.25c-3.751,17.01,6.695,34.216,23.286,38.355"
        "c2.549,0.636,5.162,0.958,7.764,0.958c14.833,0,27.921-10.526,31.12-25.03l5.099-23.117c7.208-3.366,13.414-8.448,18.166-14.878"
        "h44.157c5.625,7.595,13.363,13.356,22.215,16.543l4.731,21.452c3.199,14.503,16.286,25.03,31.119,25.03h0"
        "c2.125,0,4.262-0.215,6.36-0.641c8.642-0.139,16.104-3.593,21.063-9.764C220.903,185.86,222.638,177.62,220.728,168.956z"
        // D-pad
        "M72.991,115.239c0,2.761-2.239,5-5,5H56.596v11.396c0,2.761-2.239,5-5,5h-9.584c-2.761,0-5-2.239-5-5v-11.396H25.616"
        "c-2.761,0-5-2.239-5-5v-9.584c0-2.761,2.239-5,5-5h11.396V89.26c0-2.761,2.239-5,5-5h9.584c2.761,0,5,2.239,5,5v11.395h11.396"
        "c2.761,0,5,2.239,5,5V115.239z"
        // Face buttons
        "M150.492,119.006c-5.142,0-9.326-4.183-9.326-9.325s4.184-9.326,9.326-9.326s9.326,4.184,9.326,9.326"
        "S155.634,119.006,150.492,119.006z M169.67,138.184c-5.142,0-9.326-4.184-9.326-9.326c0-5.142,4.184-9.325,9.326-9.325"
        "s9.325,4.183,9.325,9.325C178.995,134.001,174.812,138.184,169.67,138.184z M169.67,99.828c-5.142,0-9.326-4.184-9.326-9.326"
        "s4.184-9.325,9.326-9.325s9.325,4.183,9.325,9.325S174.812,99.828,169.67,99.828z M188.848,119.006"
        "c-5.142,0-9.325-4.183-9.325-9.325s4.183-9.326,9.325-9.326s9.326,4.184,9.326,9.326S193.99,119.006,188.848,119.006z");
    
    // Center the path around origin
    p.applyTransform(juce::AffineTransform::translation(-110.0f, -110.0f));
    
    // Scale the path to fit our button size while maintaining aspect ratio
    p.applyTransform(juce::AffineTransform::scale(0.35f));
    
    path->setPath(p);
    
    // Set the fill color based on connection status
    const auto matrixGreen = juce::Colour(0xFF00FF41);
    path->setFill(juce::FillType(connected ? matrixGreen : juce::Colours::red));
    
    // Add glow effect if connected
    if (connected)
    {
        auto glow = std::make_unique<juce::DrawablePath>();
        glow->setPath(p);
        glow->setFill(juce::FillType());
        glow->setStrokeFill(juce::FillType(matrixGreen.withAlpha(0.5f)));
        glow->setStrokeThickness(2.0f);
        
        auto group = std::make_unique<juce::DrawableComposite>();
        group->addAndMakeVisible(glow.release());
        group->addAndMakeVisible(path.release());
        
        return group;
    }
    
    return path;
}

void ControllerMappingComponent::updateButtonAppearance()
{
    auto icon = createControllerIcon(isControllerConnected);
    mappingButton.setImages(icon.get());
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
    int rowHeight = 20;
    int currentY = static_cast<int>(listArea.getY());
    
    // g.setFont(getCustomFont().withHeight(12.0f));
    
    // Helper lambda for drawing rows
    auto drawRow = [&](const juce::String& control, const juce::String& action) {
        // Draw control name

        int width = static_cast<int>(listArea.getWidth());
        int x = static_cast<int>(listArea.getX());
        g.setColour(matrixGreen);
        g.drawText(control, x, currentY, width, rowHeight, 
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
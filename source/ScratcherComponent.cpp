#include "ScratcherComponent.h"
#include "ScratchAudioNode.h"

ScratcherComponent::ScratcherComponent(tracktion_engine::Edit& edit)
    : BaseEffectComponent(edit)
{
    titleLabel.setText("DJ Scratcher", juce::dontSendNotification);
    
    // Configure labels
    scratchSpeedLabel.setText("Speed", juce::dontSendNotification);
    scratchDepthLabel.setText("Depth", juce::dontSendNotification);
    
    scratchSpeedLabel.setJustificationType(juce::Justification::centred);
    scratchDepthLabel.setJustificationType(juce::Justification::centred);
    
    // Configure sliders
    scratchSpeedSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    scratchSpeedSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 15);
    scratchSpeedSlider.setRange(0.1, 2.0, 0.01);
    scratchSpeedSlider.setValue(1.0);
    
    scratchDepthSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    scratchDepthSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 15);
    scratchDepthSlider.setRange(0.1, 1.0, 0.01);
    scratchDepthSlider.setValue(0.5);
    
    addAndMakeVisible(scratchSpeedLabel);
    addAndMakeVisible(scratchDepthLabel);
    addAndMakeVisible(scratchSpeedSlider);
    addAndMakeVisible(scratchDepthSlider);
    
    // Enable mouse events
    setInterceptsMouseClicks(true, true);
    
    // Start the timer for animation
    startTimerHz(60);
}

ScratcherComponent::~ScratcherComponent()
{
    stopTimer();
}

void ScratcherComponent::timerCallback()
{
    // Apply physics to the turntable when not being dragged
    applyTurntablePhysics();
}

void ScratcherComponent::paint(juce::Graphics& g)
{
    BaseEffectComponent::paint(g);
    
    // Draw the turntable
    drawTurntable(g);
}

void ScratcherComponent::resized()
{
    auto bounds = getEffectiveArea();
    BaseEffectComponent::resized();
    
    // Set up turntable area in the top portion
    turntableArea = bounds.removeFromTop(bounds.getHeight() * 0.7f).reduced(10.0f);
    turntableCenter = turntableArea.getCentre();
    
    // Create a grid layout for controls
    juce::Grid grid;
    grid.rowGap = juce::Grid::Px(4);
    grid.columnGap = juce::Grid::Px(4);
    
    using Track = juce::Grid::TrackInfo;
    using Fr = juce::Grid::Fr;
    
    grid.templateRows = { Track(Fr(1)), Track(Fr(2)) };
    grid.templateColumns = { Track(Fr(1)), Track(Fr(1)) };
    
    // Add items to grid
    grid.items = {
        juce::GridItem(scratchSpeedLabel),
        juce::GridItem(scratchDepthLabel),
        juce::GridItem(scratchSpeedSlider).withSize(60, 60).withJustifySelf(juce::GridItem::JustifySelf::center),
        juce::GridItem(scratchDepthSlider).withSize(60, 60).withJustifySelf(juce::GridItem::JustifySelf::center)
    };
    
    grid.performLayout(bounds.toNearestInt());
    
    // Create turntable paths
    turntablePath = juce::Path();
    turntablePath.addEllipse(turntableArea);
    
    // Create record path (slightly smaller than turntable)
    auto recordArea = turntableArea.reduced(turntableArea.getWidth() * 0.05f);
    recordPath = juce::Path();
    recordPath.addEllipse(recordArea);
    
    // Create grooves path
    groovesPath = juce::Path();
    float grooveSpacing = recordArea.getWidth() / 30.0f;
    for (float radius = grooveSpacing; radius < recordArea.getWidth() / 2.0f; radius += grooveSpacing)
    {
        juce::Path groove;
        groove.addEllipse(turntableCenter.x - radius, turntableCenter.y - radius, radius * 2.0f, radius * 2.0f);
        groovesPath.addPath(groove);
    }
}

void ScratcherComponent::mouseDown(const juce::MouseEvent& event)
{
    if (turntableArea.contains(event.position))
    {
        isDragging = true;
        lastDragPosition = event.position;
        lastAngle = getAngleFromCenter(event.position);
        
        // Activate scratching effect
        isScratchActive = true;
        if (onScratchingStateChanged)
            onScratchingStateChanged(true);
            
        // Apply initial scratch effect
        applyScratch(scratchSpeedSlider.getValue(), scratchDepthSlider.getValue());
    }
}

void ScratcherComponent::mouseDrag(const juce::MouseEvent& event)
{
    if (isDragging)
    {
        float newAngle = getAngleFromCenter(event.position);
        float angleDelta = newAngle - lastAngle;
        
        // Handle angle wrap-around
        if (angleDelta > juce::MathConstants<float>::pi)
            angleDelta -= juce::MathConstants<float>::twoPi;
        else if (angleDelta < -juce::MathConstants<float>::pi)
            angleDelta += juce::MathConstants<float>::twoPi;
        
        // Update rotation
        updateRotation(currentAngle + angleDelta);
        
        // Calculate rotation speed based on mouse movement
        rotationSpeed = angleDelta * 10.0f; // Scale factor for more pronounced effect
        
        // Apply scratch effect based on rotation speed and depth
        float scratchSpeed = rotationSpeed * scratchSpeedSlider.getValue();
        float scratchDepth = scratchDepthSlider.getValue();
        applyScratch(scratchSpeed, scratchDepth);
        
        lastDragPosition = event.position;
        lastAngle = newAngle;
    }
}

void ScratcherComponent::mouseUp(const juce::MouseEvent& event)
{
    if (isDragging)
    {
        isDragging = false;
        
        // Let physics take over for natural slowdown
        // We'll keep isScratchActive true until rotation stops
    }
}

float ScratcherComponent::getAngleFromCenter(juce::Point<float> point)
{
    return std::atan2(point.y - turntableCenter.y, point.x - turntableCenter.x);
}

void ScratcherComponent::updateRotation(float newAngle)
{
    currentAngle = newAngle;
    repaint();
}

void ScratcherComponent::drawTurntable(juce::Graphics& g)
{
    // Save the current state
    g.saveState();
    
    // Translate to center of turntable for rotation
    g.addTransform(juce::AffineTransform::translation(-turntableCenter.x, -turntableCenter.y));
    g.addTransform(juce::AffineTransform::rotation(currentAngle, 0, 0));
    g.addTransform(juce::AffineTransform::translation(turntableCenter.x, turntableCenter.y));
    
    // Draw turntable base
    g.setColour(juce::Colours::black);
    g.fillPath(turntablePath);
    g.setColour(juce::Colour(0xFF00FF41).withAlpha(0.5f));
    g.strokePath(turntablePath, juce::PathStrokeType(2.0f));
    
    // Draw record
    g.setColour(juce::Colours::darkgrey);
    g.fillPath(recordPath);
    
    // Draw grooves
    g.setColour(juce::Colours::black.withAlpha(0.7f));
    g.strokePath(groovesPath, juce::PathStrokeType(0.5f));
    
    // Draw label in center
    float labelRadius = turntableArea.getWidth() * 0.15f;
    g.setColour(juce::Colours::black);
    g.fillEllipse(turntableCenter.x - labelRadius, turntableCenter.y - labelRadius, 
                 labelRadius * 2.0f, labelRadius * 2.0f);
    
    g.setColour(juce::Colour(0xFF00FF41));
    g.drawEllipse(turntableCenter.x - labelRadius, turntableCenter.y - labelRadius, 
                 labelRadius * 2.0f, labelRadius * 2.0f, 1.0f);
    
    // Draw scratch line
    g.setColour(juce::Colour(0xFF00FF41));
    g.drawLine(turntableCenter.x, turntableCenter.y - labelRadius, 
              turntableCenter.x, turntableCenter.y - turntableArea.getHeight() * 0.45f, 2.0f);
    
    // Restore the state
    g.restoreState();
}

void ScratcherComponent::applyTurntablePhysics()
{
    if (!isDragging && std::abs(rotationSpeed) > 0.001f)
    {
        // Apply friction to slow down
        rotationSpeed *= 0.95f;
        
        // Update rotation
        updateRotation(currentAngle + rotationSpeed);
        
        // Apply scratch effect based on current speed
        float scratchSpeed = rotationSpeed * scratchSpeedSlider.getValue();
        float scratchDepth = scratchDepthSlider.getValue();
        applyScratch(scratchSpeed, scratchDepth);
    }
    else if (!isDragging && std::abs(rotationSpeed) <= 0.001f && isScratchActive)
    {
        // Rotation has effectively stopped, reset playback
        rotationSpeed = 0.0f;
        isScratchActive = false;
        resetPlayback();
        
        if (onScratchingStateChanged)
            onScratchingStateChanged(false);
    }
}

std::unique_ptr<tracktion::graph::Node> ScratcherComponent::createScratchNode(std::unique_ptr<tracktion::graph::Node> inputNode)
{
    // Create the scratch node and store a shared pointer to it for control
    auto node = std::make_unique<ScratchAudioNode>(std::move(inputNode));
    scratchNode = std::shared_ptr<ScratchAudioNode>(node.get(), [](auto*){});
    
    return node;
}

void ScratcherComponent::applyScratch(float speed, float depth)
{
    // This method would interact with the audio engine to create the scratch effect
    if (scratchNode)
    {
        scratchNode->setScratchSpeed(speed);
        scratchNode->setScratchDepth(depth);
        scratchNode->setActive(true);
    }
    
    DBG("Applying scratch: speed=" + juce::String(speed) + ", depth=" + juce::String(depth));
}

void ScratcherComponent::resetPlayback()
{
    // Reset playback to normal speed
    if (scratchNode)
    {
        scratchNode->setActive(false);
    }
    
    DBG("Resetting playback to normal");
} 
#include "ScratchComponent.h"
#include "Plugins/ScratchPlugin.h"

ScratchComponent::ScratchComponent(tracktion::engine::Edit& e) : BaseEffectComponent(e)
{
    // Create the scratch plugin
    plugin = EngineHelpers::getPluginFromRack(edit, ScratchPlugin::xmlTypeName);
    
    titleLabel.setText("Scratch", juce::dontSendNotification);

    // Create and setup the scratch pad
    scratchPad = std::make_unique<ScratchPad>();
    addAndMakeVisible(scratchPad.get());

    // Bind pad to parameters
    if (auto* scratchPlugin = dynamic_cast<ScratchPlugin*>(plugin.get()))
    {
        scratchPad->onPositionChange = [scratchPlugin](float scratchValue, float depthValue)
        {
            scratchPlugin->scratchParam->setParameter(scratchValue, juce::sendNotification);
            scratchPlugin->depthParam->setParameter(depthValue, juce::sendNotification);
        };
    }
}

void ScratchComponent::resized()
{
    auto bounds = getLocalBounds().reduced(10);
    
    // Make the pad square and centered
    int padSize = std::min(bounds.getWidth(), bounds.getHeight());
    auto padBounds = bounds.withSizeKeepingCentre(padSize, padSize);
    scratchPad->setBounds(padBounds);
}

void ScratchComponent::paint(juce::Graphics& g)
{
    BaseEffectComponent::paint(g);
    
    g.setColour(juce::Colours::white);
    g.setFont(14.0f);
    
    auto bounds = getLocalBounds().reduced(10);
    g.drawText("Scratch X/Y", bounds.removeFromTop(20), juce::Justification::centred);
    
    // Draw labels for X and Y axes
    g.setFont(12.0f);
    auto padBounds = scratchPad->getBounds();
    g.drawText("Scratch", padBounds.getX(), padBounds.getBottom() + 5, padBounds.getWidth(), 15, juce::Justification::centred);
    g.drawFittedText("Depth", padBounds.getX() - 20, padBounds.getY(), 20, padBounds.getHeight(), juce::Justification::centred, 2);
}

ScratchComponent::~ScratchComponent()
{
    // Clean up any resources if needed
}

void ScratchComponent::setScratchSpeed(float speed)
{
    if (auto* scratchPlugin = dynamic_cast<ScratchPlugin*>(plugin.get()))
    {
        scratchPlugin->scratchParam->setParameter(speed, juce::sendNotification);
    }
}
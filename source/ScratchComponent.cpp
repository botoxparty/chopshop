#include "ScratchComponent.h"

ScratchComponent::ScratchComponent(tracktion::engine::Edit& edit)
    : BaseEffectComponent(edit)
{
    DBG("ScratchComponent: Constructor started");
    
    // Set up the title
    titleLabel.setText("Scratch", juce::dontSendNotification);
    
    // Create the plugin
    DBG("ScratchComponent: Creating plugin");
    plugin = createPlugin("volume");
    
    if (plugin == nullptr)
    {
        DBG("ScratchComponent: Failed to create plugin!");
    }
    else
    {
        DBG("ScratchComponent: Plugin created successfully");
    }
    
    
    DBG("ScratchComponent: Constructor completed");
}

ScratchComponent::~ScratchComponent()
{
}

void ScratchComponent::resized()
{
    BaseEffectComponent::resized();
}

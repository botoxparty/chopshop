/*
  ==============================================================================

    BaseEffectComponent.cpp
    Created: 17 Jan 2025 10:08:20pm
    Author:  Adam Hammad

  ==============================================================================
*/

#include "BaseEffectComponent.h"

BaseEffectComponent::BaseEffectComponent(tracktion_engine::Edit& e)
    : edit(e)
{
    // Configure title label
    titleLabel.setFont(juce::FontOptions(16.0f).withStyle("Bold"));
    titleLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(titleLabel);
    
    // Add some padding for the panel effect
    setPaintingIsUnclipped(true);
}

void BaseEffectComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    const auto wireColor = juce::Colour(0xFF00FF41);
    
    // Draw main wireframe outline
    g.setColour(wireColor.withAlpha(0.4f));
    g.drawRect(bounds, 1.0f);
    
    // Draw corner details
    float cornerSize = 8.0f;
    float inset = 2.0f;
    
    // Draw corner brackets
    auto drawCorner = [&](float x, float y, float xDir, float yDir)
    {
        g.drawLine(x, y + (yDir * inset), x, y + (yDir * cornerSize), 1.0f);
        g.drawLine(x + (xDir * inset), y, x + (xDir * cornerSize), y, 1.0f);
    };
    
    // Draw corners
    drawCorner(bounds.getX(), bounds.getY(), 1.0f, 1.0f);           // Top left
    drawCorner(bounds.getRight(), bounds.getY(), -1.0f, 1.0f);      // Top right
    drawCorner(bounds.getX(), bounds.getBottom(), 1.0f, -1.0f);     // Bottom left
    drawCorner(bounds.getRight(), bounds.getBottom(), -1.0f, -1.0f);// Bottom right
}

void BaseEffectComponent::drawScrew(juce::Graphics& g, float x, float y)
{
    const float screwSize = 8.0f;
    const auto matrixGreen = juce::Colour(0xFF00FF41);
    
    // Enhanced screw shadow with multiple layers
    for (int i = 3; i > 0; --i)
    {
        float offset = i * 0.5f;
        g.setColour(juce::Colours::black.withAlpha(0.3f / i));
        g.fillEllipse(x - screwSize/2 + offset, y - screwSize/2 + offset, screwSize, screwSize);
    }
    
    // Draw screw base with more detailed gradient
    juce::ColourGradient screwGradient(
        juce::Colours::grey.brighter(0.3f),
        x - screwSize/2, y - screwSize/2,
        juce::Colours::grey.darker(0.4f),
        x + screwSize/2, y + screwSize/2,
        true
    );
    
    // Add subtle metallic highlights
    screwGradient.addColour(0.4f, juce::Colours::grey.brighter(0.1f));
    screwGradient.addColour(0.6f, juce::Colours::grey.darker(0.1f));
    
    g.setGradientFill(screwGradient);
    g.fillEllipse(x - screwSize/2, y - screwSize/2, screwSize, screwSize);
    
    // Add metallic ring effect
    g.setColour(juce::Colours::white.withAlpha(0.2f));
    g.drawEllipse(x - screwSize/2, y - screwSize/2, screwSize, screwSize, 0.5f);
    
    // Enhanced screw slot with depth effect
    g.setColour(juce::Colours::black.withAlpha(0.7f));
    const float slotLength = screwSize * 0.7f;
    const float slotWidth = 1.5f;
    g.drawLine(x - slotLength/2, y, x + slotLength/2, y, slotWidth);
    
    // Add highlight to one side of the slot
    g.setColour(juce::Colours::white.withAlpha(0.2f));
    g.drawLine(x - slotLength/2, y - 0.5f, x + slotLength/2, y - 0.5f, 0.5f);
    
    // Matrix-style glow with multiple layers
    for (int i = 0; i < 3; ++i)
    {
        g.setColour(matrixGreen.withAlpha((0.15f - i * 0.04f)));
        g.drawEllipse(x - screwSize/2 - i, y - screwSize/2 - i, 
                     screwSize + i*2, screwSize + i*2, 0.5f);
    }
}

juce::Rectangle<float> BaseEffectComponent::getEffectiveArea() const
{
    // Return the usable area inside the screws
    const float inset = 20.0f;
    return getLocalBounds().reduced(inset).toFloat();
}

void BaseEffectComponent::resized()
{
    auto bounds = getLocalBounds();
    // Reserve space at the top for the title
    titleLabel.setBounds(bounds.removeFromTop(25));
}

void BaseEffectComponent::bindSliderToParameter(juce::Slider& slider, tracktion_engine::AutomatableParameter& param)
{
    slider.setRange(param.getValueRange().getStart(), param.getValueRange().getEnd(), 0.01);
    slider.setValue(param.getCurrentValue(), juce::dontSendNotification);
    
    slider.onValueChange = [&param, &slider] {
        param.setParameter(static_cast<float>(slider.getValue()), juce::sendNotification);
    };
    
    slider.onDragStart = [&param] { param.parameterChangeGestureBegin(); };
    slider.onDragEnd = [&param] { param.parameterChangeGestureEnd(); };
}

tracktion_engine::Plugin::Ptr BaseEffectComponent::createPlugin(const juce::String& xmlType)
{
    auto plugin = edit.getPluginCache().createNewPlugin(xmlType, {});
    return plugin;
}

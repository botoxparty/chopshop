/*
  ==============================================================================

    BaseEffectComponent.cpp
    Created: 17 Jan 2025 10:08:20pm
    Author:  Adam Hammad

  ==============================================================================
*/

#include "BaseEffectComponent.h"

BaseEffectComponent::BaseEffectComponent(tracktion::engine::Edit& e)
    : edit(e)
{
    // Configure title label
    titleLabel.setFont(juce::FontOptions(16.0f).withStyle("Bold"));
    titleLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(titleLabel);
    
    // Add content component
    addAndMakeVisible(contentComponent);
    
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
    return getLocalBounds().toFloat();
}

void BaseEffectComponent::resized()
{
    auto effectiveArea = getEffectiveArea().toNearestInt();
    
    juce::FlexBox flexBox;
    flexBox.flexDirection = juce::FlexBox::Direction::column;
    flexBox.flexWrap = juce::FlexBox::Wrap::noWrap;
    flexBox.alignContent = juce::FlexBox::AlignContent::stretch;
    flexBox.justifyContent = juce::FlexBox::JustifyContent::flexStart;
    
    // Add title with fixed height
    flexBox.items.add(juce::FlexItem(titleLabel)
        .withHeight(25.0f)
        .withWidth(effectiveArea.getWidth())
        .withMargin(juce::FlexItem::Margin(0, 0, 5, 0))  // Bottom margin of 5
        .withFlex(0));  // Don't flex
        
    // Content area will take up remaining space
    flexBox.items.add(juce::FlexItem(contentComponent)
        .withWidth(effectiveArea.getWidth())
        .withFlex(1.0f));  // Flex to fill remaining space
    
    flexBox.performLayout(effectiveArea);
}

void BaseEffectComponent::bindSliderToParameter(juce::Slider& slider, tracktion::engine::AutomatableParameter& param)
{
    slider.setRange(param.getValueRange().getStart(), param.getValueRange().getEnd(), 0.01);
    slider.setValue(param.getCurrentValue(), juce::dontSendNotification);
    
    slider.onValueChange = [&param, &slider] {
        param.setParameter(static_cast<float>(slider.getValue()), juce::sendNotification);
    };
    
    slider.onDragStart = [&param] { param.parameterChangeGestureBegin(); };
    slider.onDragEnd = [&param] { param.parameterChangeGestureEnd(); };
}
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
    // Configure title label with more sophisticated styling
    titleLabel.setFont(juce::FontOptions(12.0f).withStyle("Bold"));
    titleLabel.setJustificationType(juce::Justification::centred);
    titleLabel.setColour(juce::Label::textColourId, juce::Colour(0xFFE1BEE7)); // Light purple
    addAndMakeVisible(titleLabel);
    
    // Add content component
    addAndMakeVisible(contentComponent);
    
    // Add some padding for the panel effect
    setPaintingIsUnclipped(true);
}

void BaseEffectComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    
    // Define colors from our modern flat theme
    const auto surfaceColor = juce::Colour(0xFF1E1E1E);    // Dark surface color
    const auto borderColor = juce::Colour(0xFF505050);     // Medium gray for borders
    const auto accentColor = juce::Colour(0xFF707070);     // Light gray accent
    
    // Main background with solid color
    g.setColour(surfaceColor);
    g.fillRoundedRectangle(bounds, 6.0f);
    
    // Subtle shadow for depth
    {
        g.setColour(juce::Colours::black.withAlpha(0.2f));
        g.drawRoundedRectangle(bounds.translated(0, 1), 6.0f, 1.0f);
    }
    
    // Clean border
    g.setColour(borderColor.withAlpha(0.5f));
    g.drawRoundedRectangle(bounds, 6.0f, 1.0f);
    
    // Subtle top highlight for minimal depth cue
    {
        juce::Path topHighlight;
        topHighlight.startNewSubPath(bounds.getX() + 6.0f, bounds.getY() + 0.5f);
        topHighlight.lineTo(bounds.getRight() - 6.0f, bounds.getY() + 0.5f);
        
        g.setColour(juce::Colours::white.withAlpha(0.05f));
        g.strokePath(topHighlight, juce::PathStrokeType(1.0f));
    }
}

void BaseEffectComponent::drawScrew(juce::Graphics& g, float x, float y)
{
    const float screwSize = 8.0f;  // Slightly smaller for modern look
    const auto accentColor = juce::Colour(0xFF707070);  // Light gray accent
    
    // Simple shadow
    g.setColour(juce::Colours::black.withAlpha(0.2f));
    g.fillEllipse(x - screwSize/2 + 1, y - screwSize/2 + 1, screwSize, screwSize);
    
    // Solid color base
    g.setColour(accentColor);
    g.fillEllipse(x - screwSize/2, y - screwSize/2, screwSize, screwSize);
    
    // Simple slot
    g.setColour(juce::Colours::black.withAlpha(0.3f));
    const float slotLength = screwSize * 0.7f;
    const float slotWidth = 1.5f;
    g.drawLine(x - slotLength/2, y, x + slotLength/2, y, slotWidth);
}

juce::Rectangle<float> BaseEffectComponent::getEffectiveArea() const
{
    return getLocalBounds().toFloat();
}

void BaseEffectComponent::resized()
{
    auto effectiveArea = getLocalBounds().toNearestInt();
    
    juce::FlexBox flexBox;
    flexBox.flexDirection = juce::FlexBox::Direction::column;
    flexBox.flexWrap = juce::FlexBox::Wrap::noWrap;
    flexBox.alignContent = juce::FlexBox::AlignContent::stretch;
    flexBox.justifyContent = juce::FlexBox::JustifyContent::flexStart;
    
    // Add title with fixed height and padding
    flexBox.items.add(juce::FlexItem(titleLabel)
        .withHeight(14.0f)
        .withWidth(effectiveArea.getWidth() - 12.0f)  // Account for rounded corners
        .withMargin(juce::FlexItem::Margin(8, 6, 5, 6))  // Top, right, bottom, left margins
        .withFlex(0));  // Don't flex
        
    // Content area will take up remaining space with padding
    flexBox.items.add(juce::FlexItem(contentComponent)
        .withWidth(effectiveArea.getWidth() - 12.0f)  // Account for rounded corners
        .withMargin(juce::FlexItem::Margin(0, 6, 6, 6))  // Top, right, bottom, left margins
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
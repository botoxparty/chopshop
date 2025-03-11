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
    // Use the full bounds including title area
    auto bounds = getLocalBounds().toFloat();
    
    // Define colors from our purple theme
    const auto primaryPurple = juce::Colour(0xFF4A148C);    // Deep purple
    const auto accentPurple = juce::Colour(0xFF9C27B0);     // Bright purple
    const auto lightPurple = juce::Colour(0xFFE1BEE7);      // Light purple

    // Fill background
    g.setColour(juce::Colour(0xFF2A2A2A));
    g.fillRoundedRectangle(bounds, 6.0f);

    // Create depth effect with multiple layers
    for (int i = 4; i > 0; --i)
    {
        float alpha = 0.05f * i;
        auto layerBounds = bounds.reduced(i * 0.5f);
        g.setColour(juce::Colours::black.withAlpha(alpha));
        g.drawRoundedRectangle(layerBounds, 6.0f, 0.5f);
    }

    // Draw main border
    g.setColour(accentPurple);
    g.drawRoundedRectangle(bounds, 6.0f, 1.5f);

    // Inner highlight
    auto innerBounds = bounds.reduced(1.0f);
    g.setColour(lightPurple.withAlpha(0.1f));
    g.drawRoundedRectangle(innerBounds, 5.0f, 0.8f);

    // Add subtle top highlight
    juce::Path topHighlight;
    topHighlight.startNewSubPath(bounds.getX() + 6.0f, bounds.getY() + 0.5f);
    topHighlight.lineTo(bounds.getRight() - 6.0f, bounds.getY() + 0.5f);
    g.setColour(juce::Colours::white.withAlpha(0.15f));
    g.strokePath(topHighlight, juce::PathStrokeType(1.0f));

    // Add subtle left highlight
    juce::Path leftHighlight;
    leftHighlight.startNewSubPath(bounds.getX() + 0.5f, bounds.getY() + 6.0f);
    leftHighlight.lineTo(bounds.getX() + 0.5f, bounds.getBottom() - 6.0f);
    g.setColour(juce::Colours::white.withAlpha(0.1f));
    g.strokePath(leftHighlight, juce::PathStrokeType(1.0f));
}

void BaseEffectComponent::drawScrew(juce::Graphics& g, float x, float y)
{
    const float screwSize = 10.0f;
    const auto accentPurple = juce::Colour(0xFF9C27B0);
    
    // Enhanced shadow with larger radius
    for (int i = 4; i > 0; --i)
    {
        float offset = i * 0.7f;
        g.setColour(juce::Colours::black.withAlpha(0.2f / i));
        g.fillEllipse(x - screwSize/2 + offset, y - screwSize/2 + offset, screwSize, screwSize);
    }
    
    // Sophisticated metallic gradient for screw base
    juce::ColourGradient screwGradient(
        juce::Colours::grey.brighter(0.4f),
        x - screwSize/2, y - screwSize/2,
        juce::Colours::grey.darker(0.6f),
        x + screwSize/2, y + screwSize/2,
        true
    );
    
    // Add more color stops for a more realistic metallic look
    screwGradient.addColour(0.3f, juce::Colours::grey.brighter(0.2f));
    screwGradient.addColour(0.5f, juce::Colours::grey);
    screwGradient.addColour(0.7f, juce::Colours::grey.darker(0.3f));
    
    g.setGradientFill(screwGradient);
    g.fillEllipse(x - screwSize/2, y - screwSize/2, screwSize, screwSize);
    
    // Add metallic ring with gradient
    juce::ColourGradient ringGradient(
        juce::Colours::white.withAlpha(0.4f),
        x - screwSize/2, y - screwSize/2,
        juce::Colours::white.withAlpha(0.1f),
        x + screwSize/2, y + screwSize/2,
        true
    );
    g.setGradientFill(ringGradient);
    g.drawEllipse(x - screwSize/2, y - screwSize/2, screwSize, screwSize, 0.8f);
    
    // Enhanced screw slot with better depth effect
    g.setColour(juce::Colours::black.withAlpha(0.8f));
    const float slotLength = screwSize * 0.7f;
    const float slotWidth = 1.8f;
    g.drawLine(x - slotLength/2, y, x + slotLength/2, y, slotWidth);
    
    // Sophisticated highlight on slot
    g.setColour(juce::Colours::white.withAlpha(0.3f));
    g.drawLine(x - slotLength/2, y - 0.7f, x + slotLength/2, y - 0.7f, 0.5f);
    
    // Purple accent glow
    for (int i = 0; i < 3; ++i)
    {
        g.setColour(accentPurple.withAlpha((0.1f - i * 0.03f)));
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
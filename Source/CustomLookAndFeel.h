#pragma once
#include <JuceHeader.h>

class CustomLookAndFeel : public juce::LookAndFeel_V4
{
public:
    CustomLookAndFeel()
    {
        // Set default colors
        setColour(juce::ResizableWindow::backgroundColourId, juce::Colour(0xFF1A1A1A));
        setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF2D2D2D));
        setColour(juce::TextButton::textColourOffId, juce::Colours::white);
        setColour(juce::Slider::thumbColourId, juce::Colour(0xFFE85D5D));
        setColour(juce::Slider::trackColourId, juce::Colour(0xFF3D3D3D));
        setColour(juce::Label::textColourId, juce::Colours::white);
        
        // Set default font
        setDefaultSansSerifTypefaceName("Arial");
    }

    void drawButtonBackground(juce::Graphics& g, juce::Button& button, const juce::Colour& backgroundColour,
                            bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
    {
        auto bounds = button.getLocalBounds().toFloat().reduced(0.5f, 0.5f);
        auto baseColour = backgroundColour.withMultipliedSaturation(button.hasKeyboardFocus(true) ? 1.3f : 0.9f)
                                        .withMultipliedAlpha(button.isEnabled() ? 1.0f : 0.5f);

        if (shouldDrawButtonAsDown || shouldDrawButtonAsHighlighted)
            baseColour = baseColour.contrasting(shouldDrawButtonAsDown ? 0.2f : 0.1f);

        g.setColour(baseColour);
        g.fillRoundedRectangle(bounds, 4.0f);
    }
}; 
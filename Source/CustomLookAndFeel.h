#pragma once
#include <JuceHeader.h>

class CustomLookAndFeel : public juce::LookAndFeel_V4
{
public:
    CustomLookAndFeel()
    {
        // Set this instance as the default look and feel
        juce::LookAndFeel::setDefaultLookAndFeel(this);

        // Override the default typeface for all fonts
        setDefaultSansSerifTypeface(getCustomFont().getTypefacePtr());

        // Xbox-inspired colors with chopped and screwed aesthetic
        setColour(juce::ResizableWindow::backgroundColourId, juce::Colour(0xFF0A0A0A)); // Dark background
        setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF1A1A1A));          // Darker button base
        setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xFF00FF87));        // Neon green when active
        setColour(juce::TextButton::textColourOffId, juce::Colour(0xFF00FF87));         // Neon green text
        setColour(juce::TextButton::textColourOnId, juce::Colour(0xFF000000));          // Black text on active
        setColour(juce::Slider::thumbColourId, juce::Colour(0xFFBF00FF));               // Neon purple thumb
        setColour(juce::Slider::trackColourId, juce::Colour(0xFF404040));               // Dark track
        setColour(juce::Label::textColourId, juce::Colour(0xFF00FF87));                 // Neon green text
    }

    void drawButtonBackground(juce::Graphics &g, juce::Button &button, const juce::Colour &backgroundColour,
                              bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
    {
        auto bounds = button.getLocalBounds().toFloat().reduced(0.5f, 0.5f);
        auto baseColour = backgroundColour;

        if (shouldDrawButtonAsDown || shouldDrawButtonAsHighlighted)
            baseColour = juce::Colour(0xFF00FF87); // Neon green when highlighted

        // Add glow effect
        if (button.isEnabled())
        {
            for (int i = 0; i < 3; ++i)
            {
                auto glow = bounds.expanded(i * 0.8f);
                g.setColour(baseColour.withAlpha(0.1f - i * 0.03f));
                g.drawRoundedRectangle(glow, 4.0f, 1.0f);
            }
        }

        g.setColour(baseColour);
        g.fillRoundedRectangle(bounds, 4.0f);
    }

    void drawLinearSlider(juce::Graphics &g, int x, int y, int width, int height,
                          float sliderPos, float minSliderPos, float maxSliderPos,
                          const juce::Slider::SliderStyle style, juce::Slider &slider) override
    {
        // Add glow effect to slider
        auto thumbColour = findColour(juce::Slider::thumbColourId);
        auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat();

        // Draw track with glow
        g.setColour(findColour(juce::Slider::trackColourId));
        g.fillRoundedRectangle(bounds.withHeight(4.0f).withY(height * 0.5f - 2.0f), 2.0f);

        // Draw thumb with glow
        auto thumbWidth = 12.0f;
        auto thumbRect = juce::Rectangle<float>(sliderPos - thumbWidth * 0.5f, height * 0.5f - thumbWidth * 0.5f,
                                                thumbWidth, thumbWidth);

        // Glow effect
        for (int i = 0; i < 3; ++i)
        {
            auto glowBounds = thumbRect.expanded(i * 2.0f);
            g.setColour(thumbColour.withAlpha(0.2f - i * 0.05f));
            g.fillEllipse(glowBounds);
        }

        g.setColour(thumbColour);
        g.fillEllipse(thumbRect);
    }

    static const juce::Font getCustomFont()
    {
        static auto typeface = juce::Typeface::createSystemTypefaceFor(BinaryData::AudiowideRegular_ttf, 
                                                                      BinaryData::AudiowideRegular_ttfSize);
        return juce::Font(typeface);
    }

    juce::Typeface::Ptr getTypefaceForFont(const juce::Font& f) override
    {
        return getCustomFont().getTypefacePtr();
    }
};
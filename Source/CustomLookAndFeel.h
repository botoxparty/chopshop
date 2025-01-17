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

        // Matrix-inspired colors
        const auto matrixGreen = juce::Colour(0xFF00FF41);      // Bright matrix green
        const auto darkGreen = juce::Colour(0xFF003B00);        // Dark green for backgrounds
        const auto black = juce::Colour(0xFF000000);            // Pure black
        const auto darkerGreen = juce::Colour(0xFF002200);      // Darker green for contrast

        setColour(juce::ResizableWindow::backgroundColourId, black);
        setColour(juce::TextButton::buttonColourId, darkGreen);
        setColour(juce::TextButton::buttonOnColourId, matrixGreen);
        setColour(juce::TextButton::textColourOffId, matrixGreen);
        setColour(juce::TextButton::textColourOnId, black);
        setColour(juce::Slider::thumbColourId, matrixGreen);
        setColour(juce::Slider::trackColourId, darkerGreen);
        setColour(juce::Label::textColourId, matrixGreen);
    }

    void drawButtonBackground(juce::Graphics& g, juce::Button& button, 
                            const juce::Colour& backgroundColour,
                            bool shouldDrawButtonAsHighlighted, 
                            bool shouldDrawButtonAsDown) override
    {
        auto bounds = button.getLocalBounds().toFloat().reduced(0.5f, 0.5f);
        auto baseColour = backgroundColour;
        const auto matrixGreen = juce::Colour(0xFF00FF41);

        if (shouldDrawButtonAsDown)
            baseColour = matrixGreen;
        else if (shouldDrawButtonAsHighlighted)
            baseColour = baseColour.brighter(0.3f);

        // Matrix-style glow effect
        if (button.isEnabled())
        {
            for (int i = 0; i < 4; ++i)
            {
                auto glow = bounds.expanded(i * 1.0f);
                g.setColour(matrixGreen.withAlpha(0.15f - i * 0.03f));
                g.drawRoundedRectangle(glow, 3.0f, 1.0f);
            }
        }

        g.setColour(baseColour);
        g.fillRoundedRectangle(bounds, 3.0f);

        // Add subtle scanline effect
        if (button.isEnabled())
        {
            g.setColour(juce::Colours::black.withAlpha(0.1f));
            for (int y = 2; y < bounds.getHeight(); y += 4)
                g.drawHorizontalLine(y, bounds.getX(), bounds.getRight());
        }
    }

    void drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                         float sliderPos, float minSliderPos, float maxSliderPos,
                         const juce::Slider::SliderStyle style, juce::Slider& slider) override
    {
        const auto matrixGreen = juce::Colour(0xFF00FF41);
        auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat();
        auto thumbWidth = 12.0f;

        // Draw track with glow
        auto trackBounds = bounds.withHeight(4.0f).withY(height * 0.5f - 2.0f);
        
        // Track glow
        for (int i = 0; i < 3; ++i)
        {
            g.setColour(matrixGreen.withAlpha(0.1f - i * 0.03f));
            g.fillRoundedRectangle(trackBounds.expanded(i * 1.0f), 2.0f);
        }

        g.setColour(findColour(juce::Slider::trackColourId));
        g.fillRoundedRectangle(trackBounds, 2.0f);

        // Draw thumb with matrix-style glow
        auto thumbRect = juce::Rectangle<float>(sliderPos - thumbWidth * 0.5f,
                                              height * 0.5f - thumbWidth * 0.5f,
                                              thumbWidth, thumbWidth);

        // Thumb glow
        for (int i = 0; i < 4; ++i)
        {
            auto glowBounds = thumbRect.expanded(i * 1.5f);
            g.setColour(matrixGreen.withAlpha(0.2f - i * 0.04f));
            g.fillEllipse(glowBounds);
        }

        g.setColour(matrixGreen);
        g.fillEllipse(thumbRect);
    }

    static const juce::Font getCustomFont()
    {
        static auto typeface = juce::Typeface::createSystemTypefaceFor(
            BinaryData::AudiowideRegular_ttf, 
            BinaryData::AudiowideRegular_ttfSize);
        return juce::Font(typeface);
    }

    juce::Typeface::Ptr getTypefaceForFont(const juce::Font& f) override
    {
        return getCustomFont().getTypefacePtr();
    }
};
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
        if (slider.getName() == "Crossfader")
        {
            // Main background
            auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat();
            g.setColour(juce::Colours::black);
            g.fillRoundedRectangle(bounds, 2.0f);
            
            // Draw groove
            auto grooveWidth = width * 0.9f;
            auto grooveHeight = 8.0f;
            auto grooveBounds = juce::Rectangle<float>((width - grooveWidth) * 0.5f, (height - grooveHeight) * 0.5f,
                                                       grooveWidth, grooveHeight);
            
            // Groove shadow
            g.setColour(juce::Colours::black);
            g.fillRoundedRectangle(grooveBounds.translated(0, 1), 3.0f);
            
            // Groove base
            g.setColour(juce::Colours::darkgrey.darker());
            g.fillRoundedRectangle(grooveBounds, 3.0f);

            // Center marker
            float centerX = bounds.getCentreX();
            g.setColour(juce::Colours::white.withAlpha(0.3f));
            g.drawVerticalLine(static_cast<int>(centerX), grooveBounds.getY(), grooveBounds.getBottom());

            // Draw fader handle
            float handleWidth = 40.0f;
            float handleHeight = height * 0.8f;
            
            auto thumbBounds = juce::Rectangle<float>(sliderPos - handleWidth * 0.5f,
                                                      (height - handleHeight) * 0.5f,
                                                      handleWidth, handleHeight);
                                                  
            // Handle shadow
            g.setColour(juce::Colours::black.withAlpha(0.5f));
            g.fillRoundedRectangle(thumbBounds.translated(2, 2), 4.0f);
            
            // Handle base
            g.setGradientFill(juce::ColourGradient(juce::Colours::white.darker(), thumbBounds.getX(), thumbBounds.getY(),
                                                    juce::Colours::white.darker(0.2f), thumbBounds.getRight(), thumbBounds.getBottom(),
                                                    false));
            g.fillRoundedRectangle(thumbBounds, 4.0f);
            
            // Handle detail lines
            g.setColour(juce::Colours::black.withAlpha(0.3f));
            float lineSpacing = 4.0f;
            int numLines = 5;
            float startY = thumbBounds.getCentreY() - (numLines - 1) * lineSpacing * 0.5f;
            
            for (int i = 0; i < numLines; ++i)
            {
                float y = startY + i * lineSpacing;
                g.drawHorizontalLine(static_cast<int>(y), 
                                   thumbBounds.getX() + 5.0f, 
                                   thumbBounds.getRight() - 5.0f);
            }
        }
        else
        {
            // Default slider drawing for other sliders
            const auto matrixGreen = juce::Colour(0xFF00FF41);
            auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat();
            auto thumbWidth = 12.0f;
            
            auto trackBounds = bounds.withHeight(4.0f).withY(height * 0.5f - 2.0f);
            
            for (int i = 0; i < 3; ++i)
            {
                g.setColour(matrixGreen.withAlpha(0.1f - i * 0.03f));
                g.fillRoundedRectangle(trackBounds.expanded(i * 1.0f), 2.0f);
            }

            g.setColour(findColour(juce::Slider::trackColourId));
            g.fillRoundedRectangle(trackBounds, 2.0f);

            auto thumbRect = juce::Rectangle<float>(sliderPos - thumbWidth * 0.5f,
                                                    height * 0.5f - thumbWidth * 0.5f,
                                                    thumbWidth, thumbWidth);

            for (int i = 0; i < 4; ++i)
            {
                auto glowBounds = thumbRect.expanded(i * 1.5f);
                g.setColour(matrixGreen.withAlpha(0.2f - i * 0.04f));
                g.fillEllipse(glowBounds);
            }

            g.setColour(matrixGreen);
            g.fillEllipse(thumbRect);
        }
    }

    void drawLinearSliderBackground(juce::Graphics& g, int x, int y, int width, int height,
                                  float /*sliderPos*/, float /*minSliderPos*/, float /*maxSliderPos*/,
                                  const juce::Slider::SliderStyle /*style*/, juce::Slider& slider) override
    {
        if (slider.getName() == "Crossfader")
        {
            auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat();
            
            // Draw main background
            g.setColour(juce::Colours::darkgrey.darker());
            g.fillRoundedRectangle(bounds, 2.0f);
            
            // Draw center line
            g.setColour(juce::Colours::white.withAlpha(0.3f));
            float centerX = bounds.getCentreX();
            g.drawVerticalLine(static_cast<int>(centerX), bounds.getY(), bounds.getBottom());
            
            // Draw markers
            g.setColour(juce::Colours::white.withAlpha(0.5f));
            float markerWidth = 2.0f;
            float markerHeight = bounds.getHeight() * 0.3f;
            
            // Left marker
            g.fillRect(bounds.getX() + 2, bounds.getCentreY() - markerHeight/2, markerWidth, markerHeight);
            
            // Right marker
            g.fillRect(bounds.getRight() - 4, bounds.getCentreY() - markerHeight/2, markerWidth, markerHeight);
        }
    }

    void drawLinearSliderThumb(juce::Graphics& g, int x, int y, int width, int height,
                              float sliderPos, float minSliderPos, float maxSliderPos,
                              const juce::Slider::SliderStyle style, juce::Slider& slider) override
    {
        if (slider.getName() == "Crossfader")
        {
            auto thumbWidth = 20.0f;
            auto thumbHeight = height * 0.8f;
            
            juce::Rectangle<float> thumbRect(sliderPos - thumbWidth * 0.5f,
                                             y + (height - thumbHeight) * 0.5f,
                                             thumbWidth,
                                             thumbHeight);
            
            // Draw thumb shadow
            g.setColour(juce::Colours::black.withAlpha(0.3f));
            g.fillRoundedRectangle(thumbRect.translated(1, 1), 3.0f);
            
            // Draw thumb body
            g.setColour(juce::Colours::lightgrey);
            g.fillRoundedRectangle(thumbRect, 3.0f);
            
            // Draw thumb grip lines
            g.setColour(juce::Colours::darkgrey);
            float lineSpacing = 3.0f;
            float lineWidth = 1.0f;
            float startX = thumbRect.getCentreX() - lineWidth;
            for (int i = -2; i <= 2; ++i)
            {
                float y = thumbRect.getCentreY() + i * lineSpacing;
                g.fillRect(startX, y, lineWidth * 2.0f, 1.0f);
            }
        }
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
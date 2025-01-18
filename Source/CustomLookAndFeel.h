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

        // Wireframe-inspired colors
        const auto wireColor = juce::Colour(0xFF00FF41);      // Bright matrix green
        const auto darkWire = juce::Colour(0xFF003B00);       // Dark green for backgrounds
        const auto black = juce::Colour(0xFF000000);          // Pure black
        
        setColour(juce::ResizableWindow::backgroundColourId, black);
        setColour(juce::TextButton::buttonColourId, black);
        setColour(juce::TextButton::buttonOnColourId, black);
        setColour(juce::TextButton::textColourOffId, wireColor);
        setColour(juce::TextButton::textColourOnId, wireColor);
        setColour(juce::Slider::thumbColourId, wireColor);
        setColour(juce::Slider::trackColourId, wireColor.withAlpha(0.3f));
        setColour(juce::Label::textColourId, wireColor);
    }

    void drawButtonBackground(juce::Graphics& g, juce::Button& button, 
                            const juce::Colour& backgroundColour,
                            bool shouldDrawButtonAsHighlighted, 
                            bool shouldDrawButtonAsDown) override
    {
        auto bounds = button.getLocalBounds().toFloat().reduced(0.5f, 0.5f);
        const auto wireColor = juce::Colour(0xFF00FF41);

        // Draw wireframe rectangle
        g.setColour(wireColor.withAlpha(shouldDrawButtonAsDown ? 0.8f : 
                                       shouldDrawButtonAsHighlighted ? 0.6f : 0.4f));
        
        // Main outline
        g.drawRect(bounds, 1.0f);
        
        // Corner details
        float cornerSize = 4.0f;
        // Top left
        g.drawLine(bounds.getX(), bounds.getY(), bounds.getX() + cornerSize, bounds.getY(), 1.0f);
        g.drawLine(bounds.getX(), bounds.getY(), bounds.getX(), bounds.getY() + cornerSize, 1.0f);
        // Top right
        g.drawLine(bounds.getRight() - cornerSize, bounds.getY(), bounds.getRight(), bounds.getY(), 1.0f);
        g.drawLine(bounds.getRight(), bounds.getY(), bounds.getRight(), bounds.getY() + cornerSize, 1.0f);
        // Bottom left
        g.drawLine(bounds.getX(), bounds.getBottom() - cornerSize, bounds.getX(), bounds.getBottom(), 1.0f);
        g.drawLine(bounds.getX(), bounds.getBottom(), bounds.getX() + cornerSize, bounds.getBottom(), 1.0f);
        // Bottom right
        g.drawLine(bounds.getRight(), bounds.getBottom() - cornerSize, bounds.getRight(), bounds.getBottom(), 1.0f);
        g.drawLine(bounds.getRight() - cornerSize, bounds.getBottom(), bounds.getRight(), bounds.getBottom(), 1.0f);
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

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                         float sliderPosProportional, float rotaryStartAngle,
                         float rotaryEndAngle, juce::Slider& slider) override
    {
        const auto matrixGreen = juce::Colour(0xFF00FF41);
        auto bounds = juce::Rectangle<float>(x, y, width, height);
        auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.4f;
        auto centreX = bounds.getCentreX();
        auto centreY = bounds.getCentreY();
        auto angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);
        
        // Draw outer glow rings
        for (int i = 3; i > 0; --i)
        {
            g.setColour(matrixGreen.withAlpha(0.1f * i));
            g.drawEllipse(centreX - radius - i * 2, 
                         centreY - radius - i * 2,
                         (radius + i * 2) * 2,
                         (radius + i * 2) * 2, 
                         1.0f);
        }
        
        // Draw background circle
        g.setColour(matrixGreen.withAlpha(0.1f));
        g.fillEllipse(centreX - radius, centreY - radius, radius * 2, radius * 2);
        
        // Draw value arc
        juce::Path valueArc;
        valueArc.addArc(centreX - radius, centreY - radius, radius * 2, radius * 2,
                        rotaryStartAngle, angle, true);
        g.setColour(matrixGreen);
        g.strokePath(valueArc, juce::PathStrokeType(2.0f));
        
        // Draw pointer
        juce::Path pointer;
        auto pointerLength = radius * 0.8f;
        auto pointerThickness = 2.0f;
        pointer.addRectangle(-pointerThickness * 0.5f, -radius, pointerThickness, pointerLength);
        pointer.applyTransform(juce::AffineTransform::rotation(angle).translated(centreX, centreY));
        g.setColour(matrixGreen);
        g.fillPath(pointer);
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
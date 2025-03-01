#pragma once

#include "BinaryData.h"
#include <juce_core/juce_core.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>

class CustomLookAndFeel : public juce::LookAndFeel_V4
{
public:
    CustomLookAndFeel()
    {
        // Set this instance as the default look and feel
        juce::LookAndFeel::setDefaultLookAndFeel (this);

        // Override the default typeface for all fonts
        setDefaultSansSerifTypeface (getCustomFont().getTypefacePtr());

        // Winamp-inspired colors
        const auto displayColor = juce::Colour (0xFF00FF41); // Classic Winamp green
        // const auto metalGrey = juce::Colour(0xFF2A2A2A);      // Dark metallic
        const auto metalLight = juce::Colour (0xFF3D3D3D); // Light metallic
        const auto accentColor = juce::Colour (0xFF484848); // Accent grey
        const auto backgroundColor = juce::Colour (0xFF000000); // Background grey

        setColour (juce::ResizableWindow::backgroundColourId, backgroundColor);
        setColour (juce::TextButton::buttonColourId, metalLight);
        setColour (juce::TextButton::buttonOnColourId, accentColor);
        setColour (juce::TextButton::textColourOffId, displayColor);
        setColour (juce::TextButton::textColourOnId, displayColor);
        setColour (juce::Slider::thumbColourId, metalLight);
        setColour (juce::Slider::trackColourId, displayColor.withAlpha (0.5f));
        setColour (juce::Label::textColourId, displayColor);
    }

    void drawButtonBackground (juce::Graphics& g, juce::Button& button, [[maybe_unused]] const juce::Colour& backgroundColour, [[maybe_unused]] bool shouldDrawButtonAsHighlighted, [[maybe_unused]] bool shouldDrawButtonAsDown) override
    {
        auto bounds = button.getLocalBounds().toFloat().reduced (0.5f, 0.5f);

        // Metallic gradient background
        juce::ColourGradient gradient (
            juce::Colour (0xFF3D3D3D),
            bounds.getTopLeft(),
            juce::Colour (0xFF2A2A2A),
            bounds.getBottomRight(),
            false);

        g.setGradientFill (gradient);
        g.fillRoundedRectangle (bounds, 2.0f);

        // Add metallic edge effect
        if (shouldDrawButtonAsDown)
        {
            g.setColour (juce::Colours::black.withAlpha (0.3f));
            g.drawRoundedRectangle (bounds, 2.0f, 1.0f);
        }
        else
        {
            // Top-left highlight
            g.setColour (juce::Colours::white.withAlpha (0.1f));
            g.drawLine (bounds.getX(), bounds.getY(), bounds.getRight(), bounds.getY(), 1.0f);
            g.drawLine (bounds.getX(), bounds.getY(), bounds.getX(), bounds.getBottom(), 1.0f);

            // Bottom-right shadow
            g.setColour (juce::Colours::black.withAlpha (0.2f));
            g.drawLine (bounds.getX(), bounds.getBottom(), bounds.getRight(), bounds.getBottom(), 1.0f);
            g.drawLine (bounds.getRight(), bounds.getY(), bounds.getRight(), bounds.getBottom(), 1.0f);
        }
    }

    void drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height, float sliderPos, [[maybe_unused]] float minSliderPos, [[maybe_unused]] float maxSliderPos, [[maybe_unused]] const juce::Slider::SliderStyle style, juce::Slider& slider) override
    {
        if (slider.getName() == "Crossfader")
        {
            // Calculate groove dimensions and position
            auto grooveWidth = width * 0.8f;
            auto grooveHeight = 4.0f; // Reduced from 8.0f for a slimmer look
            auto grooveBounds = juce::Rectangle<float> (
                x + (width - grooveWidth) * 0.5f,
                y + (height - grooveHeight) * 0.5f,
                grooveWidth,
                grooveHeight);

            // Draw groove with glow effect
            const auto matrixGreen = juce::Colour (0xFF00FF41);

            // Base groove - back to black
            g.setColour (juce::Colours::black);
            g.fillRoundedRectangle (grooveBounds, 2.0f);

            // Groove glow layers
            for (int i = 0; i < 3; ++i)
            {
                g.setColour (matrixGreen.withAlpha (0.1f - i * 0.03f));
                g.drawRoundedRectangle (grooveBounds.expanded (i * 1.0f), 2.0f, 1.0f);
            }

            // Center marker
            float centerX = x + width * 0.5f;
            g.setColour (matrixGreen.withAlpha (0.3f));
            g.drawVerticalLine (static_cast<int> (centerX),
                grooveBounds.getY() - 2,
                grooveBounds.getBottom() + 2);

            // Draw fader handle
            float handleWidth = 30.0f;
            float handleHeight = height * 0.8f;

            // Calculate handle position with limits to prevent clipping
            float minX = x + handleWidth * 0.5f;
            float maxX = x + width - handleWidth * 0.5f;
            float limitedSliderPos = juce::jlimit (minX, maxX, sliderPos);

            auto thumbBounds = juce::Rectangle<float> (limitedSliderPos - handleWidth * 0.5f,
                y + (height - handleHeight) * 0.5f,
                handleWidth,
                handleHeight);

            // Handle shadow
            g.setColour (juce::Colours::black.withAlpha (0.5f));
            g.fillRoundedRectangle (thumbBounds.translated (2, 2), 4.0f);

            // Handle base
            g.setGradientFill (juce::ColourGradient (
                juce::Colour (0xFF3D3D3D), // Light metallic
                thumbBounds.getTopLeft(),
                juce::Colour (0xFF2A2A2A), // Dark metallic
                thumbBounds.getBottomRight(),
                false));
            g.fillRoundedRectangle (thumbBounds, 4.0f);

            // Handle detail lines
            g.setColour (juce::Colours::black.withAlpha (0.3f));
            float lineSpacing = 4.0f;
            int numLines = 5;
            float startY = thumbBounds.getCentreY() - (numLines - 1) * lineSpacing * 0.5f;

            for (int i = 0; i < numLines; ++i)
            {
                float y = startY + i * lineSpacing;
                g.drawHorizontalLine (static_cast<int> (y),
                    thumbBounds.getX() + 5.0f,
                    thumbBounds.getRight() - 5.0f);
            }
        }
        else
        {
            // Default slider drawing for other sliders
            const auto matrixGreen = juce::Colour (0xFF00FF41);
            auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat();
            auto thumbWidth = 12.0f;

            auto trackBounds = bounds.withHeight (4.0f).withY (height * 0.5f - 2.0f);

            for (int i = 0; i < 3; ++i)
            {
                g.setColour (matrixGreen.withAlpha (0.1f - i * 0.03f));
                g.fillRoundedRectangle (trackBounds.expanded (i * 1.0f), 2.0f);
            }

            g.setColour (findColour (juce::Slider::trackColourId));
            g.fillRoundedRectangle (trackBounds, 2.0f);

            auto thumbRect = juce::Rectangle<float> (sliderPos - thumbWidth * 0.5f,
                height * 0.5f - thumbWidth * 0.5f,
                thumbWidth,
                thumbWidth);

            for (int i = 0; i < 4; ++i)
            {
                auto glowBounds = thumbRect.expanded (i * 1.5f);
                g.setColour (matrixGreen.withAlpha (0.2f - i * 0.04f));
                g.fillEllipse (glowBounds);
            }

            g.setColour (matrixGreen);
            g.fillEllipse (thumbRect);
        }
    }

    void drawLinearSliderBackground (juce::Graphics& g, int x, int y, int width, int height, float /*sliderPos*/, float /*minSliderPos*/, float /*maxSliderPos*/, const juce::Slider::SliderStyle /*style*/, juce::Slider& slider) override
    {
        if (slider.getName() == "Crossfader")
        {
            auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat();

            // Draw only the center line
            const auto displayColor = juce::Colour (0xFF00FF41); // Winamp green
            g.setColour (displayColor.withAlpha (0.3f));
            float centerX = bounds.getCentreX();
            g.drawVerticalLine (static_cast<int> (centerX), bounds.getY() + 4, bounds.getBottom() - 4);
        }
    }

    void drawLinearSliderThumb (juce::Graphics& g, int x, int y, int width, int height, float sliderPos, float minSliderPos, float maxSliderPos, const juce::Slider::SliderStyle style, juce::Slider& slider) override
    {
        if (slider.getName() == "Crossfader")
        {
            auto thumbWidth = 20.0f;
            auto thumbHeight = height * 0.8f;

            juce::Rectangle<float> thumbRect (sliderPos - thumbWidth * 0.5f,
                y + (height - thumbHeight) * 0.5f,
                thumbWidth,
                thumbHeight);

            // Draw thumb shadow
            g.setColour (juce::Colours::black.withAlpha (0.3f));
            g.fillRoundedRectangle (thumbRect.translated (1, 1), 3.0f);

            // Draw thumb body
            g.setColour (juce::Colours::lightgrey);
            g.fillRoundedRectangle (thumbRect, 3.0f);

            // Draw thumb grip lines
            g.setColour (juce::Colours::darkgrey);
            float lineSpacing = 3.0f;
            float lineWidth = 1.0f;
            float startX = thumbRect.getCentreX() - lineWidth;
            for (int i = -2; i <= 2; ++i)
            {
                float lineY = thumbRect.getCentreY() + i * lineSpacing;
                g.fillRect (startX, lineY, lineWidth * 2.0f, 1.0f);
            }
        }
    }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, float sliderPosProportional, float rotaryStartAngle, float rotaryEndAngle, juce::Slider& slider) override
    {
        const auto matrixGreen = juce::Colour (0xFF00FF41);
        auto bounds = juce::Rectangle<float> (x, y, width, height);
        auto radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.35f;
        auto centreX = bounds.getCentreX();
        auto centreY = bounds.getCentreY();
        auto angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

        // Draw outer glow rings
        for (int i = 3; i > 0; --i)
        {
            g.setColour (matrixGreen.withAlpha (0.1f * i));
            g.drawEllipse (centreX - radius - i * 2,
                centreY - radius - i * 2,
                (radius + i * 2) * 2,
                (radius + i * 2) * 2,
                1.0f);
        }

        // Draw background circle
        g.setColour (matrixGreen.withAlpha (0.1f));
        g.fillEllipse (centreX - radius, centreY - radius, radius * 2, radius * 2);

        // Draw value arc
        juce::Path valueArc;
        valueArc.addArc (centreX - radius, centreY - radius, radius * 2, radius * 2, rotaryStartAngle, angle, true);
        g.setColour (matrixGreen);
        g.strokePath (valueArc, juce::PathStrokeType (2.0f));

        // Draw pointer
        juce::Path pointer;
        auto pointerLength = radius * 0.8f;
        auto pointerThickness = 2.0f;
        pointer.addRectangle (-pointerThickness * 0.5f, -radius, pointerThickness, pointerLength);
        pointer.applyTransform (juce::AffineTransform::rotation (angle).translated (centreX, centreY));
        g.setColour (matrixGreen);
        g.fillPath (pointer);
    }

    void drawComboBox (juce::Graphics& g, int width, int height, bool isButtonDown, int buttonX, int buttonY, int buttonW, int buttonH, juce::ComboBox& box) override
    {
        const auto matrixGreen = juce::Colour (0xFF00FF41);
        const auto cornerSize = 3.0f;

        // Draw main background
        g.setColour (juce::Colours::black);
        g.fillRoundedRectangle (0, 0, width, height, cornerSize);

        // Draw border with glow effect
        for (int i = 0; i < 3; ++i)
        {
            g.setColour (matrixGreen.withAlpha (0.1f - i * 0.03f));
            g.drawRoundedRectangle (0.5f + i, 0.5f + i, width - 1 - 2 * i, height - 1 - 2 * i, cornerSize, 1.0f);
        }

        // Draw arrow
        juce::Path path;
        const float arrowSize = 10.0f;
        const float x = buttonX + buttonW * 0.5f;
        const float y = buttonY + buttonH * 0.5f;

        path.addTriangle (x - arrowSize * 0.5f, y - arrowSize * 0.25f, x + arrowSize * 0.5f, y - arrowSize * 0.25f, x, y + arrowSize * 0.25f);

        g.setColour (matrixGreen);
        g.fillPath (path);
    }

    void drawPopupMenuItem (juce::Graphics& g, const juce::Rectangle<int>& area, bool isSeparator, bool isActive, bool isHighlighted, bool isTicked, bool hasSubMenu, const juce::String& text, const juce::String& shortcutKeyText, const juce::Drawable* icon, const juce::Colour* textColour) override
    {
        const auto matrixGreen = juce::Colour (0xFF00FF41);

        if (isHighlighted && isActive)
        {
            g.setColour (matrixGreen.withAlpha (0.2f));
            g.fillRect (area);
        }

        g.setColour (isActive ? matrixGreen : matrixGreen.withAlpha (0.5f));
        g.setFont (getCustomFont().withHeight (14.0f));

        juce::Rectangle<int> textArea = area.reduced (2);
        g.drawFittedText (text, textArea, juce::Justification::centredLeft, 1);
    }

    juce::PopupMenu::Options getOptionsForComboBoxPopupMenu (juce::ComboBox& box, juce::Label& label) override
    {
        return juce::PopupMenu::Options()
            .withTargetComponent (&box)
            .withItemThatMustBeVisible (box.getSelectedId())
            .withMinimumWidth (box.getWidth())
            .withMaximumNumColumns (1)
            .withStandardItemHeight (24);
    }

    static const juce::Font getCustomFont()
    {
        static auto typeface = juce::Typeface::createSystemTypefaceFor (
            BinaryData::AudiowideRegular_ttf,
            BinaryData::AudiowideRegular_ttfSize);
        return juce::Font (juce::FontOptions (typeface));
    }

    static const juce::Font getMonospaceFont()
    {
        static auto typeface = juce::Typeface::createSystemTypefaceFor (
            BinaryData::JetBrainsMonoVariableFont_wght_ttf,
            BinaryData::JetBrainsMonoVariableFont_wght_ttfSize);
        return juce::Font (juce::FontOptions (typeface));
    }

    juce::Typeface::Ptr getTypefaceForFont (const juce::Font& f) override
    {
        return getCustomFont().getTypefacePtr();
    }

    void drawDocumentWindowTitleBar (juce::DocumentWindow& window, juce::Graphics& g, int w, int h, int titleSpaceX, int titleSpaceW, const juce::Image* icon, bool drawTitleTextOnLeft) override
    {
        const auto matrixGreen = juce::Colour (0xFF00FF41);
        const auto metalGrey = juce::Colour (0xFF2A2A2A);
        const auto metalLight = juce::Colour (0xFF3D3D3D);

        // Draw title bar background with metallic gradient
        juce::ColourGradient gradient (
            metalLight,
            0.0f,
            0.0f,
            metalGrey,
            0.0f,
            (float) h,
            false);
        g.setGradientFill (gradient);
        g.fillAll();

        // Draw title text
        g.setColour (matrixGreen);
        g.setFont (getCustomFont().withHeight (16.0f));

        const juce::String title = window.getName();
        const int titleWidth = w - 100; // Leave space for buttons
        g.drawText (title, (w - titleWidth) / 2, 0, titleWidth, h, juce::Justification::centred, true);

        // Draw metallic edge effects
        g.setColour (juce::Colours::white.withAlpha (0.1f));
        g.drawHorizontalLine (0, 0.0f, (float) w);
        g.setColour (juce::Colours::black.withAlpha (0.2f));
        g.drawHorizontalLine (h - 1, 0.0f, (float) w);
    }
};
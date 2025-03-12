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

        // Modern flat color palette with more subtle colors
        const auto primary = juce::Colour(0xFF505050);      // Medium gray
        const auto primaryVariant = juce::Colour(0xFF404040); // Darker gray
        const auto secondary = juce::Colour(0xFF707070);    // Light gray accent
        const auto background = juce::Colour(0xFF121212);   // Dark background
        const auto surface = juce::Colour(0xFF1E1E1E);      // Surface color
        const auto trackBg = juce::Colour(0xFF2A2A2A);     // Track background - slightly lighter than surface
        const auto textPrimary = juce::Colours::white;      // White text
        const auto textSecondary = juce::Colours::white.withAlpha(0.6f); // Secondary text

        setColour(juce::ResizableWindow::backgroundColourId, background);
        setColour(juce::TextButton::buttonColourId, primary);
        setColour(juce::TextButton::buttonOnColourId, primaryVariant);
        setColour(juce::TextButton::textColourOffId, textPrimary);
        setColour(juce::TextButton::textColourOnId, textPrimary);
        setColour(juce::Slider::thumbColourId, secondary);
        setColour(juce::Slider::trackColourId, trackBg);
        setColour(juce::Label::textColourId, textPrimary);
        setColour(juce::PopupMenu::backgroundColourId, surface);
        setColour(juce::PopupMenu::textColourId, textPrimary);
        setColour(juce::PopupMenu::highlightedBackgroundColourId, primary);
        setColour(juce::PopupMenu::highlightedTextColourId, textPrimary);
        setColour(juce::ComboBox::backgroundColourId, surface);
        setColour(juce::ComboBox::textColourId, textPrimary);
        setColour(juce::ComboBox::outlineColourId, primary);
    }

    void drawButtonBackground (juce::Graphics& g, juce::Button& button, const juce::Colour& backgroundColour, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
    {
        auto bounds = button.getLocalBounds().toFloat().reduced(0.5f, 0.5f);
        const float cornerRadius = 4.0f;

        // Use flat colors with state-based modifications
        auto baseColour = backgroundColour;
        
        if (shouldDrawButtonAsDown)
            baseColour = baseColour.darker(0.2f);
        else if (shouldDrawButtonAsHighlighted)
            baseColour = baseColour.brighter(0.1f);

        // Main fill
        g.setColour(baseColour);
        g.fillRoundedRectangle(bounds, cornerRadius);

        // Subtle bottom shadow when not pressed
        if (!shouldDrawButtonAsDown) {
            g.setColour(juce::Colours::black.withAlpha(0.2f));
            g.drawHorizontalLine(bounds.getBottom() + 1, bounds.getX(), bounds.getRight());
        }
    }

    void drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height, float sliderPos, float minSliderPos, float maxSliderPos, const juce::Slider::SliderStyle style, juce::Slider& slider) override
    {
        if (slider.getName() == "Crossfader")
        {
            // Calculate groove dimensions
            auto grooveWidth = width * 0.8f;
            auto grooveHeight = 3.0f;  // Slightly thinner
            auto grooveBounds = juce::Rectangle<float> (
                x + (width - grooveWidth) * 0.5f,
                y + (height - grooveHeight) * 0.5f,
                grooveWidth,
                grooveHeight);

            // Draw groove with subtle gradient
            juce::ColourGradient grooveGradient(
                findColour(juce::Slider::trackColourId).brighter(0.1f),
                grooveBounds.getTopLeft(),
                findColour(juce::Slider::trackColourId).darker(0.1f),
                grooveBounds.getBottomLeft(),
                false);
            g.setGradientFill(grooveGradient);
            g.fillRoundedRectangle(grooveBounds, 1.5f);

            // Center marker
            float centerX = x + width * 0.5f;
            g.setColour(findColour(juce::Slider::thumbColourId).withAlpha(0.3f));
            g.drawVerticalLine(static_cast<int>(centerX), grooveBounds.getY(), grooveBounds.getBottom());

            // Draw fader handle
            float handleWidth = 24.0f;  // Slightly narrower
            float handleHeight = height * 0.7f;  // Slightly shorter

            float minX = x + handleWidth * 0.5f;
            float maxX = x + width - handleWidth * 0.5f;
            float limitedSliderPos = juce::jlimit(minX, maxX, sliderPos);

            auto thumbBounds = juce::Rectangle<float>(
                limitedSliderPos - handleWidth * 0.5f,
                y + (height - handleHeight) * 0.5f,
                handleWidth,
                handleHeight);

            // Simple shadow
            g.setColour(juce::Colours::black.withAlpha(0.2f));
            g.fillRoundedRectangle(thumbBounds.translated(0, 1), 3.0f);

            // Handle with subtle gradient
            juce::ColourGradient handleGradient(
                findColour(juce::Slider::thumbColourId).brighter(0.1f),
                thumbBounds.getTopLeft(),
                findColour(juce::Slider::thumbColourId).darker(0.1f),
                thumbBounds.getBottomLeft(),
                false);
            g.setGradientFill(handleGradient);
            g.fillRoundedRectangle(thumbBounds, 3.0f);

            // Handle detail lines
            g.setColour(juce::Colours::black.withAlpha(0.2f));
            float lineSpacing = 4.0f;
            int numLines = 3;
            float startY = thumbBounds.getCentreY() - (numLines - 1) * lineSpacing * 0.5f;

            for (int i = 0; i < numLines; ++i)
            {
                float lineY = startY + i * lineSpacing;
                g.drawHorizontalLine(static_cast<int>(lineY),
                    thumbBounds.getX() + 5.0f,
                    thumbBounds.getRight() - 5.0f);
            }
        }
        else
        {
            // Default slider drawing
            auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat();
            
            // Make thumb more rectangular
            const float thumbWidth = 16.0f;
            const float thumbHeight = 10.0f;

            // Track - make it more visible
            const float trackHeight = 4.0f;
            auto trackBounds = bounds.withHeight(trackHeight).withY(height * 0.5f - trackHeight * 0.5f);
            
            // Draw track background with subtle gradient
            juce::ColourGradient trackGradient(
                findColour(juce::Slider::trackColourId).brighter(0.1f),
                trackBounds.getTopLeft(),
                findColour(juce::Slider::trackColourId).darker(0.1f),
                trackBounds.getBottomLeft(),
                false);
            g.setGradientFill(trackGradient);
            g.fillRoundedRectangle(trackBounds, 2.0f);
            
            // Draw filled portion of track
            if (style == juce::Slider::LinearHorizontal)
            {
                auto filledTrack = trackBounds.withWidth(sliderPos - x);
                g.setColour(findColour(juce::Slider::thumbColourId));
                g.fillRoundedRectangle(filledTrack, 2.0f);
            }

            // Thumb
            auto thumbRect = juce::Rectangle<float>(
                sliderPos - thumbWidth * 0.5f,
                height * 0.5f - thumbHeight * 0.5f,
                thumbWidth,
                thumbHeight);

            // Simple shadow
            g.setColour(juce::Colours::black.withAlpha(0.2f));
            g.fillRoundedRectangle(thumbRect.translated(0, 1), 2.0f);

            // Thumb with subtle gradient
            juce::ColourGradient thumbGradient(
                findColour(juce::Slider::thumbColourId).brighter(0.1f),
                thumbRect.getTopLeft(),
                findColour(juce::Slider::thumbColourId).darker(0.1f),
                thumbRect.getBottomLeft(),
                false);
            g.setGradientFill(thumbGradient);
            g.fillRoundedRectangle(thumbRect, 2.0f);
            
            // Thumb border
            g.setColour(findColour(juce::Slider::thumbColourId).darker(0.2f));
            g.drawRoundedRectangle(thumbRect, 2.0f, 1.0f);
        }
    }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, float sliderPosProportional, float rotaryStartAngle, float rotaryEndAngle, juce::Slider& slider) override
    {
        auto bounds = juce::Rectangle<float>(x, y, width, height);
        auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.35f;
        auto centreX = bounds.getCentreX();
        auto centreY = bounds.getCentreY();
        auto angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

        // Colors
        const auto thumbColor = findColour(juce::Slider::thumbColourId);
        const auto trackColor = findColour(juce::Slider::trackColourId);

        // Draw background
        auto dialBounds = juce::Rectangle<float>(centreX - radius, centreY - radius, radius * 2, radius * 2);
        g.setColour(trackColor);
        g.fillEllipse(dialBounds);

        // Draw track
        g.setColour(thumbColor.withAlpha(0.3f));
        auto lineW = juce::jmin(4.0f, radius * 0.1f);
        auto arcRadius = radius - lineW * 1.5f;

        juce::Path backgroundArc;
        backgroundArc.addCentredArc(centreX, centreY, arcRadius, arcRadius,
                                  0.0f, rotaryStartAngle, rotaryEndAngle, true);
        g.strokePath(backgroundArc, juce::PathStrokeType(lineW));

        // Draw value arc
        if (rotaryStartAngle < angle)
        {
            g.setColour(thumbColor);
            juce::Path valueArc;
            valueArc.addCentredArc(centreX, centreY, arcRadius, arcRadius,
                                 0.0f, rotaryStartAngle, angle, true);
            g.strokePath(valueArc, juce::PathStrokeType(lineW));
        }

        // Draw pointer
        juce::Path p;
        auto pointerLength = radius * 0.8f;
        auto pointerThickness = 2.0f;
        p.addRectangle(-pointerThickness * 0.5f, -radius + lineW * 2,
                      pointerThickness, pointerLength);
        p.applyTransform(juce::AffineTransform::rotation(angle).translated(centreX, centreY));
        g.setColour(thumbColor);
        g.fillPath(p);

        // Draw center dot
        auto dotRadius = radius * 0.1f;
        g.setColour(thumbColor);
        g.fillEllipse(centreX - dotRadius, centreY - dotRadius, dotRadius * 2, dotRadius * 2);
    }

    void drawComboBox (juce::Graphics& g, int width, int height, bool isButtonDown, int buttonX, int buttonY, int buttonW, int buttonH, juce::ComboBox& box) override
    {
        const float cornerSize = 4.0f;

        // Draw main background
        g.setColour(findColour(juce::ComboBox::backgroundColourId));
        g.fillRoundedRectangle(0, 0, width, height, cornerSize);

        // Draw border
        g.setColour(findColour(juce::ComboBox::outlineColourId));
        g.drawRoundedRectangle(0.5f, 0.5f, width - 1.0f, height - 1.0f, cornerSize, 1.0f);

        // Draw arrow
        const float arrowSize = 10.0f;
        const float x = buttonX + buttonW * 0.5f;
        const float y = buttonY + buttonH * 0.5f;

        juce::Path path;
        path.addTriangle(x - arrowSize * 0.5f, y - arrowSize * 0.25f,
                        x + arrowSize * 0.5f, y - arrowSize * 0.25f,
                        x, y + arrowSize * 0.25f);

        g.setColour(findColour(juce::ComboBox::textColourId));
        g.fillPath(path);
    }

    void drawPopupMenuItem (juce::Graphics& g, const juce::Rectangle<int>& area, [[maybe_unused]] bool isSeparator, bool isActive, bool isHighlighted, [[maybe_unused]] bool isTicked, [[maybe_unused]] bool hasSubMenu, const juce::String& text, [[maybe_unused]] const juce::String& shortcutKeyText, [[maybe_unused]] const juce::Drawable* icon, [[maybe_unused]] const juce::Colour* textColour) override
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

    juce::PopupMenu::Options getOptionsForComboBoxPopupMenu (juce::ComboBox& box, [[maybe_unused]] juce::Label& label) override
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

    juce::Typeface::Ptr getTypefaceForFont ([[maybe_unused]] const juce::Font& f) override
    {
        return getCustomFont().getTypefacePtr();
    }

    void drawDocumentWindowTitleBar (juce::DocumentWindow& window, juce::Graphics& g, int w, int h, [[maybe_unused]] int titleSpaceX, [[maybe_unused]] int titleSpaceW, [[maybe_unused]] const juce::Image* icon, [[maybe_unused]] bool drawTitleTextOnLeft) override
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

    void drawAlertBox (juce::Graphics& g, juce::AlertWindow& alert, const juce::Rectangle<int>& textArea, juce::TextLayout& textLayout) override
    {
        const float cornerSize = 4.0f;
        const auto background = juce::Colour(0xFF121212);   // Dark background
        const auto surface = juce::Colour(0xFF1E1E1E);      // Surface color
        const auto matrixGreen = juce::Colour(0xFF00FF41);  // Accent color
        
        auto bounds = alert.getLocalBounds().toFloat().reduced(0.5f, 0.5f);
        
        // Draw main background
        g.setColour(background);
        g.fillRoundedRectangle(bounds, cornerSize);
        
        // Draw subtle border
        g.setColour(surface.brighter(0.1f));
        g.drawRoundedRectangle(bounds, cornerSize, 1.0f);
        
        // Draw header area with gradient
        auto headerBounds = bounds.withHeight(30.0f);
        juce::ColourGradient headerGradient(
            surface.brighter(0.1f),
            headerBounds.getTopLeft(),
            surface,
            headerBounds.getBottomLeft(),
            false);
        g.setGradientFill(headerGradient);
        g.fillRoundedRectangle(headerBounds, cornerSize);
        
        // Draw message text (includes title)
        g.setColour(juce::Colours::white);
        textLayout.draw(g, juce::Rectangle<float>(textArea.toFloat()));
    }
};
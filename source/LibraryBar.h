#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "CustomLookAndFeel.h"

class LibraryBar : public juce::Component
{
public:
    LibraryBar()
    {
        // Style the show library button
        showLibraryButton.setButtonText("Show Library");
        showLibraryButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF505050));      // Medium gray
        showLibraryButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
        addAndMakeVisible(showLibraryButton);
        
        // Style the track label with modern dark theme
        currentTrackLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        currentTrackLabel.setColour(juce::Label::backgroundColourId, juce::Colour(0xFF1E1E1E));      // Surface color
        currentTrackLabel.setJustificationType(juce::Justification::centredLeft);
        currentTrackLabel.setFont(CustomLookAndFeel::getMonospaceFont().withHeight(14.0f));
        currentTrackLabel.setText("No Track Loaded", juce::dontSendNotification);
        currentTrackLabel.setBorderSize(juce::BorderSize<int>(1));
        addAndMakeVisible(currentTrackLabel);
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        auto reducedBounds = bounds.reduced(5);
        
        // Layout using manual positioning
        auto labelBounds = reducedBounds.withTrimmedRight(110); // Space for button
        currentTrackLabel.setBounds(labelBounds);
        
        auto buttonBounds = reducedBounds.removeFromRight(100);
        showLibraryButton.setBounds(buttonBounds);
    }

    void paint(juce::Graphics& g) override
    {
        // Fill background with dark theme color
        g.fillAll(juce::Colour(0xFF121212));   // Dark background
        
        // Draw subtle border around the track label
        auto labelBounds = currentTrackLabel.getBounds();
        g.setColour(juce::Colour(0xFF2A2A2A));     // Track background - slightly lighter
        g.drawRect(labelBounds, 1);
    }

    void setCurrentTrackName(const juce::String& name)
    {
        currentTrackLabel.setText(name, juce::dontSendNotification);
    }

    juce::TextButton& getShowLibraryButton() { return showLibraryButton; }

private:
    juce::TextButton showLibraryButton;
    juce::Label currentTrackLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LibraryBar)
}; 
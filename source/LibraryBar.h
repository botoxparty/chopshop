#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "CustomLookAndFeel.h"
#include "Utilities.h"

class LibraryBar : public juce::Component
{
public:
    LibraryBar(tracktion::engine::Engine& e) : engine(e)
    {
        // Style the show library button
        showLibraryButton.setButtonText("LIBRARY");
        showLibraryButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF505050));      // Medium gray
        showLibraryButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
        addAndMakeVisible(showLibraryButton);
        
        // Style the game controller button
        controllerButton.setButtonText(juce::String::fromUTF8("🎮"));
        controllerButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF505050));      // Medium gray
        controllerButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
        addAndMakeVisible(controllerButton);

        // Style the audio settings button
        audioSettingsButton.setButtonText(juce::String::fromUTF8("🔊"));
        audioSettingsButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF505050));      // Medium gray
        audioSettingsButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
        audioSettingsButton.onClick = [this] { EngineHelpers::showAudioDeviceSettings(engine); };
        addAndMakeVisible(audioSettingsButton);
        
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
        auto libraryButtonWidth = 70;  // Fixed width for library button
        auto emojiButtonsWidth = 80;   // Space for two emoji buttons (35px each + 10px gap)
        
        // Position track label on the left
        auto labelBounds = reducedBounds.withTrimmedRight(libraryButtonWidth + emojiButtonsWidth + 10);
        currentTrackLabel.setBounds(labelBounds);
        
        // Get the right side area for all buttons
        auto rightArea = reducedBounds.removeFromRight(libraryButtonWidth + emojiButtonsWidth);
        
        // Position library button next to the track label
        auto libraryBounds = rightArea.removeFromLeft(libraryButtonWidth);
        showLibraryButton.setBounds(libraryBounds);
        
        // Add a small gap between library button and emoji buttons
        rightArea.removeFromLeft(10);
        
        // Position emoji buttons adjacent to each other
        auto buttonWidth = 35;
        controllerButton.setBounds(rightArea.removeFromLeft(buttonWidth));
        audioSettingsButton.setBounds(rightArea.removeFromLeft(buttonWidth));
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
    juce::TextButton& getControllerButton() { return controllerButton; }

private:
    tracktion::engine::Engine& engine;
    juce::TextButton showLibraryButton;
    juce::TextButton controllerButton;
    juce::TextButton audioSettingsButton;
    juce::Label currentTrackLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LibraryBar)
}; 
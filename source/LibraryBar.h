#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "CustomLookAndFeel.h"
class LibraryBar : public juce::Component
{
public:
    LibraryBar()
    {
        showLibraryButton.setButtonText("Show Library");
        addAndMakeVisible(showLibraryButton);
        
        // Style the track label to look like a screen display
        currentTrackLabel.setColour(juce::Label::textColourId, juce::Colours::lime);
        currentTrackLabel.setColour(juce::Label::backgroundColourId, juce::Colours::darkgrey.darker(0.8f));
        currentTrackLabel.setJustificationType(juce::Justification::centredLeft);
        currentTrackLabel.setFont(CustomLookAndFeel::getMonospaceFont());
        currentTrackLabel.setText("No Track Loaded", juce::dontSendNotification);
        currentTrackLabel.setBorderSize(juce::BorderSize<int>(2));
        addAndMakeVisible(currentTrackLabel);
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        DBG("LibraryBar bounds: " + juce::String(bounds.getWidth()) + "x" + juce::String(bounds.getHeight()));
        
        // Set minimum height for the component
        auto buttonHeight = 30;
        auto reducedBounds = bounds.reduced(5);
        
        // Layout using manual positioning first to ensure components have size
        auto labelBounds = reducedBounds.withTrimmedRight(110); // Space for button
        currentTrackLabel.setBounds(labelBounds);
        
        auto buttonBounds = reducedBounds.removeFromRight(100);
        showLibraryButton.setBounds(buttonBounds);
        
        DBG("Label bounds: " + juce::String(labelBounds.getWidth()) + "x" + juce::String(labelBounds.getHeight()));
        DBG("Button bounds: " + juce::String(buttonBounds.getWidth()) + "x" + juce::String(buttonBounds.getHeight()));
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colours::black);
        
        // Draw a subtle border around the track label to enhance the screen effect
        auto labelBounds = currentTrackLabel.getBounds().expanded(2);
        g.setColour(juce::Colours::grey.darker(0.5f));
        g.drawRect(labelBounds, 2);
        
        // Add a subtle inner shadow effect
        g.setColour(juce::Colours::black.withAlpha(0.3f));
        g.drawRect(labelBounds.reduced(2), 1);
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
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

class LibraryBar : public juce::Component
{
public:
    LibraryBar()
    {
        showLibraryButton.setButtonText("Show Library");
        addAndMakeVisible(showLibraryButton);
        
        currentTrackLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        currentTrackLabel.setJustificationType(juce::Justification::centredLeft);
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
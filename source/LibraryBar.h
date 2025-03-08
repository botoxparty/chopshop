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
        auto bounds = getLocalBounds().reduced(5);
        showLibraryButton.setBounds(bounds.removeFromRight(100));
        currentTrackLabel.setBounds(bounds);
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colours::black);
        g.setColour(juce::Colours::darkgrey);
        g.drawRect(getLocalBounds(), 1);
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
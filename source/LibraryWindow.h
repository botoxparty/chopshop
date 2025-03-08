#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "LibraryComponent.h"

class LibraryWindow : public juce::DocumentWindow
{
public:
    LibraryWindow(tracktion::engine::Engine& engine)
        : DocumentWindow("ChopShop Library", 
                       juce::Colours::black,
                       DocumentWindow::allButtons)
    {
        libraryComponent = std::make_unique<LibraryComponent>(engine);
        setContentOwned(libraryComponent.get(), true);
        setResizable(true, true);
        centreWithSize(600, 400);
        
        // Add to desktop and make visible
        setUsingNativeTitleBar(true);
        addToDesktop();    // Explicitly add to desktop
        setVisible(true);
        toFront(true);     // Bring to front
    }

    ~LibraryWindow() override = default;

    void closeButtonPressed() override
    {
        setVisible(false);
    }

    LibraryComponent* getLibraryComponent() { return libraryComponent.get(); }

private:
    std::unique_ptr<LibraryComponent> libraryComponent;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LibraryWindow)
}; 
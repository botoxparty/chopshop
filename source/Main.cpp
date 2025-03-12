/*
  ==============================================================================

    This file contains the basic startup code for a JUCE application.

  ==============================================================================
*/


#include <juce_core/juce_core.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <melatonin_inspector/melatonin_inspector.h>
#include <tracktion_engine/tracktion_engine.h>

#include "MainComponent.h"
#include "CustomLookAndFeel.h"

//==============================================================================
class ChopShopApplication  : public juce::JUCEApplication
{
public:
    //==============================================================================
    ChopShopApplication() {}

    const juce::String getApplicationName() override       { return ProjectInfo::projectName; }
    const juce::String getApplicationVersion() override    { return ProjectInfo::versionString; }
    bool moreThanOneInstanceAllowed() override             { return true; }

    //==============================================================================
    void initialise ([[maybe_unused]] const juce::String& commandLine) override
    {
        // This method is where you should put your application's initialisation code..

        Process::setPriority(Process::HighPriority);

        mainWindow.reset (new MainWindow (getApplicationName()));
    }

    void shutdown() override
    {
        // Add your application's shutdown code here..

        // First clear the content
        if (mainWindow != nullptr)
            mainWindow->setContentOwned(nullptr, true);
        
        // Then destroy the window
        mainWindow = nullptr; // (deletes our window)
    }

    //==============================================================================
    void systemRequestedQuit() override
    {
        // Check if there are any unsaved changes
        if (auto* mainComp = dynamic_cast<MainComponent*>(mainWindow->getContentComponent()))
        {
            if (auto* edit = mainComp->getEdit())
            {
                if (edit->hasChangedSinceSaved())
                {
                    auto options = juce::MessageBoxOptions::makeOptionsYesNoCancel(
                        juce::MessageBoxIconType::QuestionIcon,
                        "Save Changes?",
                        "Do you want to save the changes to \"" + edit->getName() + "\" before quitting?",
                        "Save",
                        "Discard",
                        "Cancel"
                    );

                    juce::AlertWindow::showAsync(options, [this](int result) {
                        if (result == 1) // Save
                        {
                            // Save current edit
                            if (auto* mainComp = dynamic_cast<MainComponent*>(mainWindow->getContentComponent()))
                            {
                                if (auto* edit = mainComp->getEdit())
                                {
                                    tracktion::engine::EditFileOperations(*edit).save(true, false, false);
                                    quit();
                                }
                            }
                        }
                        else if (result == 2) // Discard
                        {
                            quit();
                        }
                        // If result == 0 (Cancel), do nothing and keep app running
                    });
                    return;
                }
            }
        }

        // If no unsaved changes, quit immediately
        quit();
    }

    void anotherInstanceStarted ([[maybe_unused]] const juce::String& commandLine) override
    {
        // When another instance of the app is launched while this one is running,
        // this method is invoked, and the commandLine parameter tells you what
        // the other instance's command-line arguments were.
    }

    //==============================================================================
    /*
        This class implements the desktop window that contains an instance of
        our MainComponent class.
    */
    class MainWindow    : public juce::DocumentWindow
    {
    public:
        MainWindow (juce::String name)
            : DocumentWindow (name,
                              juce::Colours::black,
                              DocumentWindow::allButtons)
        {
            setUsingNativeTitleBar (true);
            setContentOwned (new MainComponent(), true);

            #if JUCE_DEBUG
                inspector = std::make_unique<melatonin::Inspector>(*this);
                inspector->setVisible(true);
            #endif

            #if JUCE_IOS || JUCE_ANDROID
                setFullScreen (true);
            #else
                setResizable (true, true);
                setResizeLimits(800, 500, 10000, 10000);
                centreWithSize (getWidth(), getHeight());
            #endif

            setVisible (true);
        }

        ~MainWindow() override
        {
            setLookAndFeel(nullptr);
        }

        void closeButtonPressed() override
        {
            // This is called when the user tries to close this window. Here, we'll just
            // ask the app to quit when this happens, but you can change this to do
            // whatever you need.
            JUCEApplication::getInstance()->systemRequestedQuit();
        }

        /* Note: Be careful if you override any DocumentWindow methods - the base
           class uses a lot of them, so by overriding you might break its functionality.
           It's best to do all your work in your content component instead, but if
           you really have to override any DocumentWindow methods, make sure your
           subclass also calls the superclass's method.
        */

    private:
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainWindow)
        #if JUCE_DEBUG
            std::unique_ptr<melatonin::Inspector> inspector;
        #endif
    };

private:
    std::unique_ptr<MainWindow> mainWindow;
};

//==============================================================================
// This macro generates the main() routine that launches the app.
START_JUCE_APPLICATION (ChopShopApplication)

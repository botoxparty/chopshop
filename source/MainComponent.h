#pragma once


#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <tracktion_engine/tracktion_engine.h>

namespace ProjectInfo
{
    const char* const  projectName    = "ChopShop";
    const char* const  companyName    = "Pounding Systems";
    const char* const  versionString  = "1.0.0";
    const int          versionNumber  = 0x10000;
}


#include "Utilities.h"
#include "CustomLookAndFeel.h"
#include "ReverbComponent.h"
#include "GamepadManager.h"
#include "FlangerComponent.h"
#include "LibraryComponent.h"
#include "VinylBrakeComponent.h"
#include "DelayComponent.h"
#include "OscilloscopePlugin.h"
#include "ChopComponent.h"
#include "ScrewComponent.h"
#include "ControllerMappingComponent.h"
#include "PhaserComponent.h"
#include "Plugins/FlangerPlugin.h"
#include "Plugins/AutoDelayPlugin.h"
#include "Plugins/AutoPhaserPlugin.h"
#include "ScratchComponent.h"
#include "ScratchPlugin.h"
#include "TransportComponent.h"



// Add this line to enable console output
#define JUCE_DEBUG 1

//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/
class MainComponent : public juce::Component,
                      public juce::Timer,
                      public GamepadManager::Listener,
                      public juce::ChangeListener,
                      public tracktion::engine::OscilloscopePlugin::Listener,
                      public juce::ApplicationCommandTarget
{
public:
    //==============================================================================
    MainComponent();
    ~MainComponent() override;

    //==============================================================================
    void paint(juce::Graphics &) override;
    void resized() override;

    // GameControllerListener overrides
    void gamepadButtonPressed(int buttonId) override;
    void gamepadButtonReleased(int buttonId) override;
    void gamepadAxisMoved(int axisId, float newValue) override;
    // Toggles playback state and updates UI
    void play();
    void stop();
    void updateTempo();

    void timerCallback() override
    {
        // Check if we're shutting down
        if (chopComponent == nullptr)
        {
            stopTimer();
            return;
        }
        
        // Only manipulate the crossfader if we're handling a chop release
        if (chopReleaseDelay > 0)
        {
            stopTimer();
            float currentPosition = chopComponent->getCrossfaderValue();
            chopComponent->setCrossfaderValue(currentPosition <= 0.5f ? 1.0f : 0.0f);
            chopReleaseDelay = 0;
        }
    }

    void changeListenerCallback(juce::ChangeBroadcaster*) override
    {
        // This will be called when the transport state changes
    }

    void oscilloscopePluginInitialised() override
    {
        // Create and add the component on the message thread
        juce::MessageManager::callAsync([this]()
        {
            if (auto* oscPlugin = dynamic_cast<tracktion::engine::OscilloscopePlugin*>(oscilloscopePlugin.get()))
            {
                oscilloscopeComponent.reset(oscPlugin->createControlPanel());
                if (oscilloscopeComponent != nullptr)
                {
                    DBG("Created oscilloscope component after initialization");
                    addAndMakeVisible(*oscilloscopeComponent);
                    resized();
                }
            }
        });
    }

    // Command IDs
    enum CommandIDs
    {
        DeleteSelectedRegion = 1001
    };

    // Command handler
    juce::ApplicationCommandTarget* getNextCommandTarget() override { return nullptr; }
    void getAllCommands(juce::Array<juce::CommandID>& commands) override;
    void getCommandInfo(juce::CommandID commandID, juce::ApplicationCommandInfo& result) override;
    bool perform(const juce::ApplicationCommandTarget::InvocationInfo& info) override;

    void setupAudioGraph();

private:
    //==============================================================================
    tracktion::engine::Engine engine{ProjectInfo::projectName};
    std::unique_ptr<tracktion::engine::Edit> edit;
    std::unique_ptr<CustomLookAndFeel> customLookAndFeel;
    juce::TextButton audioSettingsButton{"Audio Settings"};

    double baseTempo = 120.0;
    double trackOffset = 0.0;

    enum class PlayState
    {
        Stopped,
        Playing,
        Paused
    };

    PlayState playState{PlayState::Stopped};

    void updateCrossfader();

    void armTrack(int trackIndex, bool arm);
    void startRecording();
    void stopRecording();

    double chopStartTime = 0.0;
    double chopReleaseDelay = 0.0;

    // GameController member variables
    GamepadManager* gamepadManager = nullptr;

    std::unique_ptr<ReverbComponent> reverbComponent;
    std::unique_ptr<FlangerComponent> flangerComponent;
    std::unique_ptr<LibraryComponent> libraryComponent;
    std::unique_ptr<VinylBrakeComponent> vinylBrakeComponent;
    std::unique_ptr<DelayComponent> delayComponent;
    std::unique_ptr<ChopComponent> chopComponent;
    std::unique_ptr<ScrewComponent> screwComponent;
    std::unique_ptr<PhaserComponent> phaserComponent;
    std::unique_ptr<ScratchComponent> scratchComponent;
    std::unique_ptr<Component> oscilloscopeComponent;
    tracktion::engine::Plugin::Ptr oscilloscopePlugin;

    std::unique_ptr<ControllerMappingComponent> controllerMappingComponent;

    void initialiseTracks();

    void setupVinylBrakeComponent();
    void setupChopComponent();
    void setupLibraryComponent();
    void setupOscilloscopeComponent();
    void setupScrewComponent();
    void setupScratchComponent();

    void createPluginRack();

    void releaseResources();

    std::unique_ptr<juce::ApplicationCommandManager> commandManager;

    std::unique_ptr<TransportComponent> transportComponent;

    // Change to handle Edits instead of Files
    void handleEditSelection(std::unique_ptr<tracktion::engine::Edit> newEdit);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
#pragma once


#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <tracktion_engine/tracktion_engine.h>

// Project info that was previously in JuceHeader.h
namespace ProjectInfo
{
    const char* const  projectName    = "ChopShop";
    const char* const  companyName    = "";
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
#include "ControlBarComponent.h"
#include "Thumbnail.h"



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
    void loadAudioFile();
    void updateTempo();
    tracktion::engine::WaveAudioClip::Ptr getClip(int trackIndex);

    void timerCallback() override
    {
        // Check if we're shutting down
        if (chopComponent == nullptr)
        {
            stopTimer();
            return;
        }
        
        updatePositionLabel();
        
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

    // Add these required methods from ApplicationCommandTarget
    juce::ApplicationCommandTarget* getNextCommandTarget() override
    {
        return chopComponent.get();
    }
    
    void getAllCommands([[maybe_unused]] juce::Array<juce::CommandID>& commands) override
    {
        // Add any commands your main component handles
    }
    
    void getCommandInfo([[maybe_unused]] juce::CommandID commandID, [[maybe_unused]] juce::ApplicationCommandInfo& result) override
    {
        // Provide info for your commands
    }
    
    bool perform([[maybe_unused]] const juce::ApplicationCommandTarget::InvocationInfo& info) override
    {
        return false; // Return true if you handle the command
    }

    void setupAudioGraph();

private:
    //==============================================================================
    tracktion::engine::Engine engine{ProjectInfo::projectName};
    tracktion::engine::Edit edit{engine, tracktion::engine::Edit::forEditing};
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

    juce::TextButton saveButton{"Save"};
    juce::TextButton recordButton{"Record"};

    void handleFileSelection(const juce::File &file);

    void updateCrossfader();
    void setTrackVolume(int trackIndex, float volume);

    std::unique_ptr<Thumbnail> thumbnail;

    void armTrack(int trackIndex, bool arm);
    void startRecording();
    void stopRecording();

    bool isTempoPercentageActive(double percentage) const;

    double chopStartTime = 0.0;
    double chopReleaseDelay = 0.0;

    te::VolumeAndPanPlugin *volumeAndPan1 = nullptr;
    te::VolumeAndPanPlugin *volumeAndPan2 = nullptr;

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
    bool isTrackLoaded()
    {
        if (auto track = EngineHelpers::getOrInsertAudioTrackAt(edit, 0))
            return !track->getClips().isEmpty();
        return false;
    }

    void updateButtonStates()
    {
        bool trackLoaded = isTrackLoaded();
        recordButton.setEnabled(trackLoaded);
    }

    void updatePositionLabel();

    std::unique_ptr<Component> oscilloscopeComponent;

    // Add a member to hold the plugin reference
    tracktion::engine::Plugin::Ptr oscilloscopePlugin;

    std::unique_ptr<ControllerMappingComponent> controllerMappingComponent;

    void createVinylBrakeComponent();

    void createPluginRack();

    void releaseResources();

    std::unique_ptr<ControlBarComponent> controlBarComponent;

    // Add this line to declare the command manager
    std::unique_ptr<juce::ApplicationCommandManager> commandManager;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
#pragma once


#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <tracktion_engine/tracktion_engine.h>
#include <moonbase_JUCEClient/moonbase_JUCEClient.h>
#include <juce_audio_utils/juce_audio_utils.h>

namespace ProjectInfo
{
    const char* const  projectName    = "ChopShop";
    const char* const  companyName    = "Pounding Systems";
    const char* const  versionString  = "1.0.0";
    const int          versionNumber  = 0x10000;
}

#define ANIMATE_COMPANY_LOGO 1

#include "Utilities.h"
#include "CustomLookAndFeel.h"
#include "ReverbComponent.h"
#include "GamepadManager.h"
#include "FlangerComponent.h"
#include "LibraryWindow.h"
#include "LibraryBar.h"
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
#include "Plugins/ScratchPlugin.h"
#include "ScratchComponent.h"
#include "TransportComponent.h"
#include "ControllerMappingComponent.h"



// Add this line to enable console output
#define JUCE_DEBUG 1

//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/
class CompanyLogo : public juce::Component,
                    private juce::Timer
{
public:
    CompanyLogo()
    {
        logo = juce::Drawable::createFromImageData(BinaryData::PoundingSystemsLogo_png,
                                                  BinaryData::PoundingSystemsLogo_pngSize);

        #if ANIMATE_COMPANY_LOGO
            jitterX.reset(15);
            jitterY.reset(15);
            startTimerHz(30);
        #endif
    }

private:
    std::unique_ptr<juce::Drawable> logo;
    
    void paint(juce::Graphics& g) override
    {
        const auto width = getWidth();
        const auto height = getHeight();
        auto area = getLocalBounds().toFloat().reduced(height * 0.1f);
        
        #if ANIMATE_COMPANY_LOGO
            const auto currentJitterX = jitterX.getNextValue();
            const auto currentJitterY = jitterY.getNextValue();
            area = area.translated(width * currentJitterX, height * currentJitterY);
        #endif
        
        if (logo != nullptr)
            logo->drawWithin(g, area, juce::RectanglePlacement::centred, 1.0f);
    }
    
    void timerCallback() override
    {
        const auto jitterRange = 0.1f;
        jitterX.setTargetValue(juce::jmap(random.nextFloat(), 0.f, 1.f, -jitterRange, jitterRange));
        jitterY.setTargetValue(juce::jmap(random.nextFloat(), 0.f, 1.f, -jitterRange, jitterRange));
        repaint();
    }

    juce::LinearSmoothedValue<float> jitterX{0.f};
    juce::LinearSmoothedValue<float> jitterY{0.f};
    juce::Random random;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CompanyLogo)
};

//==============================================================================
namespace CommandIDs
{
    static const int DeleteSelectedRegion = 0x2001;
    static const int SaveProject = 0x2002;
    static const int Undo = 0x2003;
    static const int Redo = 0x2004;
    static const int chopEffect = 0x2005;
}

class MainComponent : public juce::AudioAppComponent,
                      public juce::Timer,
                      public GamepadManager::Listener,
                      public juce::ChangeListener,
                      public tracktion::engine::OscilloscopePlugin::Listener,
                      public juce::ApplicationCommandTarget,
                      public juce::MenuBarModel
{
public:
    //==============================================================================
    MainComponent();
    ~MainComponent() override;

    tracktion::engine::Edit* getEdit() { return edit.get(); }

    // AudioAppComponent methods
    void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;
    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) override;
    void releaseResources() override;

    //==============================================================================
    void paint(juce::Graphics &) override;
    void resized() override;

    // MenuBarModel implementation
    juce::StringArray getMenuBarNames() override;
    juce::PopupMenu getMenuForIndex(int topLevelMenuIndex, const juce::String& menuName) override;
    void menuItemSelected(int menuItemID, int topLevelMenuIndex) override;

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
            // float currentPosition = chopComponent->getCrossfaderValue();
            // chopComponent->setCrossfaderValue(currentPosition <= 0.5f ? 1.0f : 0.0f);
            chopReleaseDelay = 0;
        }

        // Update command states for undo/redo
        if (commandManager != nullptr && edit != nullptr)
            commandManager->commandStatusChanged();
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
    
    std::unique_ptr<juce::MenuBarComponent> menuBar;

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
    std::unique_ptr<LibraryBar> libraryBar;
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

    void gamepadTouchpadMoved(float x, float y, bool touched) override;
    void showControllerMappingWindow();

    std::unique_ptr<juce::ApplicationCommandManager> commandManager;

    std::unique_ptr<TransportComponent> transportComponent;

    // Change to handle Edits instead of Files
    void handleEditSelection(std::unique_ptr<tracktion::engine::Edit> newEdit);
    void loadNewEdit(std::unique_ptr<tracktion::engine::Edit> newEdit);

    // Moonbase API member
    MOONBASE_DECLARE_LICENSING_USING_JUCE_PROJECTINFO;
    
    // Moonbase Activation UI member
    MOONBASE_DECLARE_AND_INIT_ACTIVATION_UI_SAME_PARENT;
    
    juce::TextButton showActivationUiButton{"Show Activation UI"};

    std::unique_ptr<ControllerMappingWindow> controllerMappingWindow;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
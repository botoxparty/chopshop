#pragma once

#include <JuceHeader.h>
#include "Utilities.h"
#include "CustomLookAndFeel.h"
#include "ReverbComponent.h"
#include "GamepadManager.h"
#include "FlangerComponent.h"
#include "LibraryComponent.h"
#include "VinylBrakeComponent.h"
#include "DelayComponent.h"
#include <aubio/aubio.h>
#include "minibpm.h"
#include "OscilloscopePlugin.h"
#include "ChopComponent.h"
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
                      public tracktion_engine::OscilloscopePlugin::Listener
{
public:
    //==============================================================================
    MainComponent();
    ~MainComponent() override;

    //==============================================================================
    void paint(juce::Graphics &) override;
    void resized() override;

    // GameControllerListener overrides
    void gamepadButtonPressed(int buttonId);
    void gamepadButtonReleased(int buttonId);
    void gamepadAxisMoved(int axisId, float newValue);
    // Toggles playback state and updates UI
    void play();
    void stop();
    void loadAudioFile();
    void updateTempo();
    tracktion_engine::WaveAudioClip::Ptr getClip(int trackIndex);

    void mouseDown(const juce::MouseEvent &event) override
    {
        if (event.eventComponent == &chopButton)
        {
            chopStartTime = juce::Time::getMillisecondCounterHiRes();
            // Switch crossfader to opposite position
            float currentPosition = crossfaderSlider.getValue();
            crossfaderSlider.setValue(currentPosition <= 0.5f ? 1.0f : 0.0f, juce::sendNotification);
        }
    }

    void mouseUp(const juce::MouseEvent &event) override
    {
        if (event.eventComponent == &chopButton)
        {
            double elapsedTime = juce::Time::getMillisecondCounterHiRes() - chopStartTime;
            double minimumTime = chopComponent->getChopDurationInMs(tempoSlider.getValue());

            if (elapsedTime >= minimumTime)
            {
                float currentPosition = crossfaderSlider.getValue();
                crossfaderSlider.setValue(currentPosition <= 0.5f ? 1.0f : 0.0f, juce::sendNotification);
            }
            else
            {
                chopReleaseDelay = minimumTime - elapsedTime;
                startTimer(static_cast<int>(chopReleaseDelay));
            }
        }
    }

    void timerCallback() override
    {
        updatePositionLabel();
        // Timer has finished, perform the delayed crossfade
        stopTimer();
        float currentPosition = crossfaderSlider.getValue();
        crossfaderSlider.setValue(currentPosition <= 0.5f ? 1.0f : 0.0f, juce::sendNotification);
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
            if (auto* oscPlugin = dynamic_cast<tracktion_engine::OscilloscopePlugin*>(oscilloscopePlugin.get()))
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

private:
    //==============================================================================
    tracktion_engine::Engine engine{ProjectInfo::projectName};
    tracktion_engine::Edit edit{engine, tracktion_engine::Edit::forEditing};
    juce::Slider tempoSlider;
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

    juce::TextButton openButton{"Browse"};
    juce::TextButton saveButton{"Save"};
    juce::TextButton playButton{"Play"};
    juce::TextButton stopButton{"Stop"};
    juce::TextButton chopButton{"Chop"};

    void handleFileSelection(const juce::File &file);

    juce::Slider crossfaderSlider;
    void updateCrossfader();
    void setTrackVolume(int trackIndex, float volume);

    std::unique_ptr<Thumbnail> thumbnail;

    juce::TextButton recordButton{"Record"};

    void armTrack(int trackIndex, bool arm);
    void startRecording();
    void stopRecording();

    juce::TextButton tempo70Button{"70%"};
    juce::TextButton tempo75Button{"75%"};
    juce::TextButton tempo80Button{"80%"};
    juce::TextButton tempo85Button{"85%"};
    juce::TextButton tempo100Button{"100%"};

    void setTempoPercentage(double percentage);
    bool isTempoPercentageActive(double percentage) const;
    void updateTempoButtonStates();


    double chopStartTime = 0.0;
    double chopReleaseDelay = 0.0;

    juce::Label currentTrackLabel{"Track Label", "No Track Loaded"};

    te::VolumeAndPanPlugin *volumeAndPan1 = nullptr;
    te::VolumeAndPanPlugin *volumeAndPan2 = nullptr;

    // GameController member variables
    std::unique_ptr<GamepadManager> gamepadManager;

    std::unique_ptr<ReverbComponent> reverbComponent;
    std::unique_ptr<FlangerComponent> flangerComponent;
    std::unique_ptr<LibraryComponent> libraryComponent;
    std::unique_ptr<VinylBrakeComponent> vinylBrakeComponent;
    std::unique_ptr<DelayComponent> delayComponent;
    std::unique_ptr<ChopComponent> chopComponent;

    bool isTrackLoaded()
    {
        if (auto track = EngineHelpers::getOrInsertAudioTrackAt(edit, 0))
            return !track->getClips().isEmpty();
        return false;
    }

    void updateButtonStates()
    {
        bool trackLoaded = isTrackLoaded();
        playButton.setEnabled(trackLoaded);
        stopButton.setEnabled(trackLoaded);
        recordButton.setEnabled(trackLoaded);
    }

    juce::Label positionLabel{"Position Label", "00:00:00.000 | 1|1|000"};

    void updatePositionLabel();

    std::unique_ptr<Component> oscilloscopeComponent;

    // Add a member to hold the plugin reference
    std::shared_ptr<tracktion_engine::Plugin> oscilloscopePlugin;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
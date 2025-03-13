#pragma once

#include <tracktion_engine/tracktion_engine.h>

#include "Utilities.h"

class ChopPlugin : public tracktion::engine::Plugin,
                  private juce::Timer
{
public:
    static const char* getPluginName() { return NEEDS_TRANS("Chop"); }
    static constexpr const char* xmlTypeName = "chop";

    static tracktion::engine::Plugin::Ptr create(tracktion::engine::Edit& ed)
    {
        DBG("Creating ChopPlugin...");
        auto v = juce::ValueTree(tracktion::engine::IDs::PLUGIN);
        v.setProperty(tracktion::engine::IDs::type, xmlTypeName, nullptr);
        
        auto plugin = new ChopPlugin(tracktion::engine::PluginCreationInfo(ed, v, true));
        DBG("ChopPlugin created, initializing...");
        plugin->initialiseFully();
        DBG("ChopPlugin initialization complete");
        return plugin;
    }

    ChopPlugin(tracktion::engine::PluginCreationInfo info) : Plugin(info)
    {
        DBG("ChopPlugin constructor start");
        if(remapOnTempoChange == false)
        {
            DBG("Remapping on tempo change on Plugin is disabled.");
        }
        
        // Start the timer to check clip states
        startTimerHz(30); // Check 30 times per second
        
        DBG("ChopPlugin constructor complete");
    }

    ~ChopPlugin() override
    {
        stopTimer();
        notifyListenersOfDeletion();
    }

    juce::String getName() const override { return TRANS("Chop"); }
    juce::String getPluginType() override { return xmlTypeName; }
    juce::String getShortName(int) override { return getName(); }
    juce::String getSelectableDescription() override { return TRANS("Chop Plugin"); }

    void initialise(const tracktion::engine::PluginInitialisationInfo& info) override 
    { 
        sampleRate = info.sampleRate;
        blockSize = info.blockSizeSamples;
        updateTrackVolumes();
    }
    
    void deinitialise() override {}
    
    void applyToBuffer(const tracktion::engine::PluginRenderContext& fc) override 
    {
        // This plugin doesn't process audio directly
        // Instead it controls the volume of two tracks via their VolumeAndPanPlugins
    }
    
    void reset() override 
    {
        updateTrackVolumes();
    }

    bool takesAudioInput() override                  { return false; }
    bool takesMidiInput() override                   { return false; }
    bool producesAudioWhenNoAudioInput() override    { return false; }
    bool canBeAddedToClip() override                 { return false; }
    bool canBeAddedToRack() override                 { return true; }

    void restorePluginStateFromValueTree(const juce::ValueTree& v) override
    {
        Plugin::restorePluginStateFromValueTree(v);
        updateTrackVolumes();
    }
    
    // Get the current volume levels for UI feedback
    float getTrack1Volume() const { return track1Volume; }
    float getTrack2Volume() const { return track2Volume; }

    void setTrackVolumes(float track1Vol, float track2Vol)
    {
        track1Volume = juce::jlimit(0.0f, 1.0f, track1Vol);
        track2Volume = juce::jlimit(0.0f, 1.0f, track2Vol);
        updateTrackVolumes();
    }

    void timerCallback() override
    {
        auto& transport = edit.getTransport();
        bool isTransportPlaying = transport.isPlaying();

        // Check if any clips are playing on the chop track
        bool isChopTrackPlaying = false;
        if (!isTransportPlaying) {
            return;
        }
        if (auto chopTrack = EngineHelpers::getChopTrack(edit))
        {
            DBG("ChopPlugin::timerCallback - Chop track found");
            DBG("ChopPlugin::timerCallback - Chop track clips: " + juce::String(chopTrack->getClips().size()));
            for (auto clip : chopTrack->getClips())
            {
                auto currentPosition = transport.getPosition();
                auto clipPosition = clip->getPosition();

                DBG("ChopPlugin::timerCallback - Clip position: " + juce::String(clipPosition.getStart().inSeconds()) + " - " + juce::String(clipPosition.getEnd().inSeconds()));
                DBG("ChopPlugin::timerCallback - Current position: " + juce::String(currentPosition.inSeconds()));

                bool isClipPlaying = currentPosition >= clipPosition.getStart() 
                                        && currentPosition < clipPosition.getEnd();
                if (isClipPlaying)
                {
                    isChopTrackPlaying = true;
                    break;
                }
            }
        }
        
        // Set track volumes based on chop track state
        setTrackVolumes(isChopTrackPlaying ? 0.0f : 1.0f,  // Track 1 volume
                       isChopTrackPlaying ? 1.0f : 0.0f);  // Track 2 volume
    }

    void updateTrackVolumes()
    {
        // Apply volumes to tracks
        if (auto track1 = EngineHelpers::getAudioTrack(edit, 0))
        {
            auto volPanPlugin = EngineHelpers::getPlugin(*track1, tracktion::engine::VolumeAndPanPlugin::xmlTypeName);

            if(volPanPlugin != nullptr)
            {
                if (auto volumeAndPan = dynamic_cast<tracktion::engine::VolumeAndPanPlugin*>(volPanPlugin.get()))
                {
                    volumeAndPan->volParam->setParameter(track1Volume, juce::dontSendNotification);
                }
                else
                {
                    DBG("ChopPlugin::updateTrackVolumes - Failed to cast VolumeAndPanPlugin for Track 1");
                }
            }
            else
            {
                DBG("ChopPlugin::updateTrackVolumes - Failed to get VolumeAndPanPlugin for Track 1");
            }
        }

        if (auto track2 = EngineHelpers::getAudioTrack(edit, 1))
        {
            auto volPanPlugin = EngineHelpers::getPlugin(*track2, tracktion::engine::VolumeAndPanPlugin::xmlTypeName);
            
            if(volPanPlugin != nullptr)
            {
                if (auto volumeAndPan = dynamic_cast<tracktion::engine::VolumeAndPanPlugin*>(volPanPlugin.get()))
                {
                    volumeAndPan->volParam->setParameter(track2Volume, juce::dontSendNotification);
                }
                else
                {
                    DBG("ChopPlugin::updateTrackVolumes - Failed to cast VolumeAndPanPlugin for Track 2");
                }
            }
            else
            {
                DBG("ChopPlugin::updateTrackVolumes - Failed to get VolumeAndPanPlugin for Track 2");
            }
        }
    }

private:
    double sampleRate = 48000.0;
    int blockSize = 512;
    float track1Volume = 1.0f;
    float track2Volume = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChopPlugin)
}; 
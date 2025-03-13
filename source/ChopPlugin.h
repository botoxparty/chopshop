#pragma once

#include "tracktion_engine/tracktion_engine.h"
#include "Utilities.h"

//==============================================================================
class ChopPlugin : public tracktion::engine::Plugin,
                  private juce::Timer
{
public:
    ChopPlugin(tracktion::engine::PluginCreationInfo info) : Plugin(info)
    {
        // Start the timer to check clip states
        startTimerHz(30); // Check 30 times per second

        chopTrack = EngineHelpers::getChopTrack(edit);
        DBG("ChopPlugin::ChopPlugin - Chop track: " + chopTrack->getName());
    }
    
    ~ChopPlugin() override
    {
        notifyListenersOfDeletion();
        stopTimer();
    }
    
    //==============================================================================
    static const char* getPluginName() { return "Chop Plugin"; }
    static const char* xmlTypeName;
    
    juce::String getName() const override { return TRANS("Chop Plugin"); }
    juce::String getPluginType() override { return xmlTypeName; }
    juce::String getSelectableDescription() override { return TRANS("Chop Plugin"); }
    bool canBeAddedToRack() override { return true; }
    tracktion::engine::AudioTrack::Ptr chopTrack;
    void timerCallback() override
    {
        updateTrackVolumes();
    }

    void updateTrackVolumes()
    {
        
        // Check if any clips are playing on the chop track
        bool isChopTrackPlaying = false;
        if (chopTrack != nullptr)
        {
            for (auto clip : chopTrack->getClips())
            {
                if (auto launchHandle = clip->getLaunchHandle())
                {
                    if (launchHandle->getPlayingStatus() == tracktion::engine::LaunchHandle::PlayState::playing)
                    {
                        DBG("ChopPlugin::updateTrackVolumes - Chop track is playing");
                        isChopTrackPlaying = true;
                        break;
                    }
                }
            }
        }
        
        // Apply volumes to tracks based on whether chop track is playing
        if (auto track1 = EngineHelpers::getAudioTrack(edit, 0))
        {
            auto volPanPlugin = EngineHelpers::getPlugin(*track1, tracktion::engine::VolumeAndPanPlugin::xmlTypeName);

            if(volPanPlugin != nullptr)
            {
                if (auto volumeAndPan = dynamic_cast<tracktion::engine::VolumeAndPanPlugin*>(volPanPlugin.get()))
                {
                    const float volume = isChopTrackPlaying ? 0.0f : 1.0f;
                    volumeAndPan->volParam->setParameter(volume, juce::dontSendNotification);
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
                    const float volume = isChopTrackPlaying ? 1.0f : 0.0f;
                    volumeAndPan->volParam->setParameter(volume, juce::dontSendNotification);
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
}; 
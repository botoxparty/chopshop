#pragma once

#include <tracktion_engine/tracktion_engine.h>

#include "Utilities.h"

namespace ChopPluginIDs
{
    struct IDs
    {
        static const juce::Identifier crossfader;
    };
}

class ChopPlugin : public tracktion::engine::Plugin,
                   public tracktion::engine::AutomatableParameter::Listener
{
public:
    static const char* getPluginName() { return NEEDS_TRANS("Chop"); }
    static constexpr const char* xmlTypeName = "chop";

    static tracktion::engine::Plugin::Ptr create(tracktion::engine::Edit& ed)
    {
        DBG("Creating ChopPlugin...");
        auto v = juce::ValueTree(tracktion::engine::IDs::PLUGIN);
        v.setProperty(tracktion::engine::IDs::type, xmlTypeName, nullptr);
        v.setProperty(ChopPluginIDs::IDs::crossfader, 0.0f, nullptr);
        
        auto plugin = new ChopPlugin(tracktion::engine::PluginCreationInfo(ed, v, true));
        DBG("ChopPlugin created, initializing...");
        plugin->initialiseFully();
        DBG("ChopPlugin initialization complete");
        return plugin;
    }

    ChopPlugin(tracktion::engine::PluginCreationInfo info) : Plugin(info)
    {
        DBG("ChopPlugin constructor start");
        auto um = getUndoManager();

        try {
            crossfaderParam = addParam("crossfader", TRANS("Crossfader"), { 0.0f, 1.0f },
                [](float value) { return juce::String((int)(100.0f * value)) + "%"; },
                [](const juce::String& s) { return s.getFloatValue() / 100.0f; });

            if (crossfaderParam == nullptr) {
                DBG("Failed to create crossfader parameter!");
                return;
            }

            crossfaderValue.referTo(state, ChopPluginIDs::IDs::crossfader, um, 0.0f);
            crossfaderParam->attachToCurrentValue(crossfaderValue);

            // Add value listener to crossfader parameter
            crossfaderParam->addListener(this);

            // Initialize track volumes
            updateTrackVolumes();
            
            DBG("ChopPlugin constructor complete");
        }
        catch (const std::exception& e) {
            DBG("Exception in ChopPlugin constructor: " + juce::String(e.what()));
        }
    }

    ~ChopPlugin() override
    {
        if (crossfaderParam != nullptr)
            crossfaderParam->removeListener(this);
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
    }
    
    void deinitialise() override {}
    
    void applyToBuffer(const tracktion::engine::PluginRenderContext& fc) override 
    {
        // This plugin doesn't process audio directly
        // Instead it controls the volume of two tracks via their VolumeAndPanPlugins
    }
    
    void reset() override {}

    bool takesAudioInput() override                  { return false; }
    bool takesMidiInput() override                   { return false; }
    bool producesAudioWhenNoAudioInput() override    { return false; }
    bool canBeAddedToClip() override                 { return false; }
    bool canBeAddedToRack() override                 { return true; }

    void restorePluginStateFromValueTree(const juce::ValueTree& v) override
    {
        Plugin::restorePluginStateFromValueTree(v);
        
        if (crossfaderParam != nullptr) {
            crossfaderParam->updateFromAttachedValue();
            updateTrackVolumes();
        }
    }

    tracktion::engine::AutomatableParameter::Ptr crossfaderParam;
    juce::CachedValue<float> crossfaderValue;

    void setCrossfader(float value) 
    { 
        crossfaderParam->setParameter(juce::jlimit(0.0f, 1.0f, value), juce::sendNotification); 
        updateTrackVolumes();
    }
    
    float getCrossfader() { return crossfaderParam->getCurrentValue(); }
    
    // Get the current volume levels for UI feedback
    float getTrack1Volume() const 
    { 
        const float position = crossfaderParam->getCurrentValue();
        return std::sqrt(1.0f - position);
    }
    
    float getTrack2Volume() const 
    { 
        const float position = crossfaderParam->getCurrentValue();
        return std::sqrt(position);
    }

    void updateTrackVolumes()
    {
        const float position = crossfaderParam->getCurrentValue();
        
        // Simple linear crossfade with slight curve for smoother transition
        // When position is 0, track1 is full volume and track2 is silent
        // When position is 1, track1 is silent and track2 is full volume
        float gainTrack1 = std::sqrt(1.0f - position); // Square root for smoother curve
        float gainTrack2 = std::sqrt(position);

        DBG("ChopPlugin::updateTrackVolumes - Crossfader position: " + juce::String(position));
        DBG("ChopPlugin::updateTrackVolumes - Track1 gain: " + juce::String(gainTrack1));
        DBG("ChopPlugin::updateTrackVolumes - Track2 gain: " + juce::String(gainTrack2));

        // Apply volumes to tracks
        if (auto track1 = EngineHelpers::getAudioTrack(edit, 0))
        {
            if (auto volumeAndPan = dynamic_cast<tracktion::engine::VolumeAndPanPlugin*>(EngineHelpers::getPlugin(*track1, tracktion::engine::VolumeAndPanPlugin::xmlTypeName).get()))
            {
                volumeAndPan->volParam->setParameter(gainTrack1, juce::sendNotification);
                DBG("ChopPlugin::updateTrackVolumes - Track 1 volume set to: " + juce::String(gainTrack1));
            }
        }

        if (auto track2 = EngineHelpers::getAudioTrack(edit, 1))
        {
            if (auto volumeAndPan = dynamic_cast<tracktion::engine::VolumeAndPanPlugin*>(EngineHelpers::getPlugin(*track2, tracktion::engine::VolumeAndPanPlugin::xmlTypeName).get()))
            {
                volumeAndPan->volParam->setParameter(gainTrack2, juce::sendNotification);
                DBG("ChopPlugin::updateTrackVolumes - Track 2 volume set to: " + juce::String(gainTrack2));
            }
        }
    }

    // Add value listener interface
    void valueTreePropertyChanged(juce::ValueTree&, const juce::Identifier&) override
    {
        Plugin::valueTreePropertyChanged(state, ChopPluginIDs::IDs::crossfader);
        updateTrackVolumes();
    }

    void currentValueChanged(tracktion::engine::AutomatableParameter&) override { updateTrackVolumes(); }
    void curveHasChanged(tracktion::engine::AutomatableParameter&) override { updateTrackVolumes(); }

private:

    double sampleRate = 44100.0;
    int blockSize = 512;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChopPlugin)
}; 
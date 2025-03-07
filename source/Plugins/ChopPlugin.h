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
        float gainTrack1 = std::cos(position * juce::MathConstants<float>::halfPi);
        return gainTrack1 <= 0.0f ? -60.0f : juce::Decibels::gainToDecibels(gainTrack1);
    }
    
    float getTrack2Volume() const 
    { 
        const float position = crossfaderParam->getCurrentValue();
        float gainTrack2 = std::sin(position * juce::MathConstants<float>::halfPi);
        return gainTrack2 <= 0.0f ? -60.0f : juce::Decibels::gainToDecibels(gainTrack2);
    }

    void updateTrackVolumes()
    {
        const float position = crossfaderParam->getCurrentValue();
        const float minDB = -60.0f; // Effectively silent

        // Calculate volume curves that give equal power at center position
        float gainTrack1 = std::cos(position * juce::MathConstants<float>::halfPi);
        float gainTrack2 = std::sin(position * juce::MathConstants<float>::halfPi);

        // Convert linear gains to dB
        float gainDB1 = gainTrack1 <= 0.0f ? minDB : juce::Decibels::gainToDecibels(gainTrack1);
        float gainDB2 = gainTrack2 <= 0.0f ? minDB : juce::Decibels::gainToDecibels(gainTrack2);

        DBG("Gain DB1: " + juce::String(gainDB1));
        DBG("Gain DB2: " + juce::String(gainDB2));

        // Apply volumes to tracks
        if (auto track1 = EngineHelpers::getAudioTrack(edit, 0))
        {
            DBG("Track 1 found");
            if (auto volumeAndPan = dynamic_cast<tracktion::engine::VolumeAndPanPlugin*>(track1->pluginList.getPluginsOfType<tracktion::engine::VolumeAndPanPlugin>().getFirst()))
            {
                DBG("Volume and pan plugin found");
                volumeAndPan->setVolumeDb(gainDB1);
            }
        }

        if (auto track2 = EngineHelpers::getAudioTrack(edit, 1))
        {
            if (auto volumeAndPan = dynamic_cast<tracktion::engine::VolumeAndPanPlugin*>(track2->pluginList.getPluginsOfType<tracktion::engine::VolumeAndPanPlugin>().getFirst()))
            {
                volumeAndPan->setVolumeDb(gainDB2);
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
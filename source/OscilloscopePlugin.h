/*
    ,--.                     ,--.     ,--.  ,--.
  ,-'  '-.,--.--.,--,--.,---.|  |,-.,-'  '-.`--' ,---. ,--,--,      Copyright 2024
  '-.  .-'|  .--' ,-.  | .--'|     /'-.  .-',--.| .-. ||      \   Tracktion Software
    |  |  |  |  \ '-'  \ `--.|  \  \  |  |  |  |' '-' '|  ||  |       Corporation
    `---' `--'   `--`--'`---'`--'`--' `---' `--' `---' `--''--'    www.tracktion.com

    Tracktion Engine uses a GPL/commercial licence - see LICENCE.md for details.
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include <tracktion_engine/tracktion_engine.h>
#include "Osc2D.h"

namespace tracktion { inline namespace engine
{

/** */
class OscilloscopePlugin   : public Plugin
{
public:
    OscilloscopePlugin (PluginCreationInfo);
    ~OscilloscopePlugin() override;

    static const char* getPluginName()                  { return NEEDS_TRANS("Oscilloscope"); }
    static juce::ValueTree create();

    //==============================================================================
    static const char* xmlTypeName;

    bool canBeAddedToFolderTrack() override             { return true; }
    juce::String getName() const override               { return textTitle.get().isNotEmpty() ? textTitle : TRANS("Oscilloscope"); }
    juce::String getPluginType() override               { return xmlTypeName; }
    void initialise (const PluginInitialisationInfo&) override;
    void deinitialise() override;
    void applyToBuffer (const PluginRenderContext&) override;
    int getNumOutputChannelsGivenInputs (int numInputChannels) override     { return numInputChannels; }
    bool producesAudioWhenNoAudioInput() override       { return false; }
    juce::String getSelectableDescription() override    { return TRANS("Oscilloscope Plugin"); }

    Component* createControlPanel();
    
    std::unique_ptr<RingBuffer<GLfloat>> getOscilloscopeBuffer() { return std::move(oscilloscopeBuffer); }

    juce::CachedValue<juce::String> textTitle, textBody;

    // Add listener interface
    class Listener
    {
    public:
        virtual ~Listener() = default;
        virtual void oscilloscopePluginInitialised() = 0;
    };

    void addListener(Listener* listener) { listeners.add(listener); }
    void removeListener(Listener* listener) { listeners.remove(listener); }

private:
    static constexpr int BUFFER_SIZE = 1024;
    std::unique_ptr<RingBuffer<GLfloat>> oscilloscopeBuffer;
    std::unique_ptr<Oscilloscope2D> oscilloscope;
    juce::ListenerList<Listener> listeners;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OscilloscopePlugin)
};

}} // namespace tracktion { inline namespace engine

/*
    ,--.                     ,--.     ,--.  ,--.
  ,-'  '-.,--.--.,--,--.,---.|  |,-.,-'  '-.`--' ,---. ,--,--,      Copyright 2024
  '-.  .-'|  .--' ,-.  | .--'|     /'-.  .-',--.| .-. ||      \   Tracktion Software
    |  |  |  |  \ '-'  \ `--.|  \  \  |  |  |  |' '-' '|  ||  |       Corporation
    `---' `--'   `--`--'`---'`--'`--' `---' `--' `---' `--''--'    www.tracktion.com

    Tracktion Engine uses a GPL/commercial licence - see LICENCE.md for details.
*/

#include "OscilloscopePlugin.h"

namespace tracktion { inline namespace engine
{

const char* OscilloscopePlugin::xmlTypeName ("text");

OscilloscopePlugin::OscilloscopePlugin (PluginCreationInfo info)  : Plugin (info)
{
    auto um = getUndoManager();

    textTitle.referTo (state, IDs::title, um);
    textBody.referTo (state, IDs::body, um);
}

OscilloscopePlugin::~OscilloscopePlugin()
{
    notifyListenersOfDeletion();
}

juce::ValueTree OscilloscopePlugin::create()
{
    return createValueTree (IDs::PLUGIN,
                            IDs::type, xmlTypeName);
}

}} // namespace tracktion { inline namespace engine

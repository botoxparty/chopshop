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

const char* OscilloscopePlugin::xmlTypeName ("oscilloscope");

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

void OscilloscopePlugin::initialise(const PluginInitialisationInfo&)
{
    oscilloscopeBuffer = std::make_unique<RingBuffer<GLfloat>>(2, BUFFER_SIZE);
}

void OscilloscopePlugin::deinitialise()
{
    oscilloscopeBuffer.reset();
}

void OscilloscopePlugin::applyToBuffer(const PluginRenderContext& rc)
{
    if (rc.bufferNumSamples > 0 && oscilloscopeBuffer != nullptr)
    {
        // Convert audio buffer to GLfloat buffer
        AudioBuffer<GLfloat> tempBuffer(rc.destBuffer->getNumChannels(), rc.bufferNumSamples);
        
        for (int ch = 0; ch < rc.destBuffer->getNumChannels(); ++ch)
            FloatVectorOperations::copy(tempBuffer.getWritePointer(ch), 
                                      rc.destBuffer->getReadPointer(ch, rc.bufferStartSample), 
                                      rc.bufferNumSamples);

        oscilloscopeBuffer->writeSamples(tempBuffer, 0, rc.bufferNumSamples);
    }
}

Component* OscilloscopePlugin::createControlPanel()
{
    DBG("Creating control panel...");
    
    if (oscilloscopeBuffer != nullptr)
    {
        DBG("Oscilloscope buffer exists");
        auto* osc = new Oscilloscope2D(oscilloscopeBuffer.get());
        
        if (osc != nullptr)
        {
            DBG("Created Oscilloscope2D");
            osc->start();
            return osc;
        }
        else
        {
            DBG("Failed to create Oscilloscope2D");
        }
    }
    else
    {
        DBG("Oscilloscope buffer is null");
    }
    
    return nullptr;
}

}} // namespace tracktion { inline namespace engine

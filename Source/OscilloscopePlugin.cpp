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

void OscilloscopePlugin::initialise(const PluginInitialisationInfo& info)
{
    DBG("OscilloscopePlugin::initialise called");
    DBG("  Sample rate: " << info.sampleRate);
    DBG("  Block size: " << info.blockSizeSamples);
    // DBG("  Num channels: " << info.numInputChannels);
    
    // Use provided block size or fallback to a reasonable default if zero
    const int blockSize = info.blockSizeSamples > 0 ? info.blockSizeSamples : 1024;
    const int bufferSize = blockSize * 10;
    DBG("Creating oscilloscope buffer with size: " << bufferSize);
    oscilloscopeBuffer = std::make_unique<RingBuffer<GLfloat>>(2, bufferSize);

    // Notify listeners that initialization is complete
    listeners.call(&Listener::oscilloscopePluginInitialised);
}

void OscilloscopePlugin::deinitialise()
{
    oscilloscopeBuffer.reset();
}

void OscilloscopePlugin::applyToBuffer(const PluginRenderContext& rc)
{
    if (rc.bufferNumSamples > 0 && oscilloscopeBuffer != nullptr)
    {
        // Only process if we actually have audio data
        if (!rc.destBuffer->hasBeenCleared())
        {
            // Convert audio buffer to GLfloat buffer
            AudioBuffer<GLfloat> tempBuffer(rc.destBuffer->getNumChannels(), rc.bufferNumSamples);
            
            for (int ch = 0; ch < rc.destBuffer->getNumChannels(); ++ch)
            {
                // Clear the temp buffer first
                tempBuffer.clear(ch, 0, rc.bufferNumSamples);
                
                // Copy only if we have valid audio data
                if (rc.destBuffer->getRMSLevel(ch, rc.bufferStartSample, rc.bufferNumSamples) > 0.0f)
                {
                    FloatVectorOperations::copy(
                        tempBuffer.getWritePointer(ch), 
                        rc.destBuffer->getReadPointer(ch, rc.bufferStartSample), 
                        rc.bufferNumSamples
                    );
                }
            }

            oscilloscopeBuffer->writeSamples(tempBuffer, 0, rc.bufferNumSamples);
        }
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

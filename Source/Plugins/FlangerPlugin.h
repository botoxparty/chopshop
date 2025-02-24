#pragma once

#include <tracktion_engine/tracktion_engine.h>

using namespace tracktion::engine;

class FlangerPlugin : public ChorusPlugin
{
public:
    FlangerPlugin(PluginCreationInfo info) : ChorusPlugin(info)
    {
        depthParam = addParam("depth", TRANS("Depth"), {0.0f, 10.0f}, [](float value)
                              { return juce::String(value, 1) + " ms"; }, [](const juce::String &s)
                              { return s.getFloatValue(); });

        speedParam = addParam("speed", TRANS("Speed"), {0.0f, 10.0f}, [](float value)
                              { return juce::String(value, 1) + " Hz"; }, [](const juce::String &s)
                              { return s.getFloatValue(); });

        widthParam = addParam("width", TRANS("Width"), {0.0f, 1.0f}, [](float value)
                              { return juce::String((int)(100.0f * value)) + "%"; }, [](const juce::String &s)
                              { return s.getFloatValue(); });

        mixParam = addParam("mix", TRANS("Mix"), {0.0f, 1.0f}, [](float value)
                            { return juce::String((int)(100.0f * value)) + "%"; }, [](const juce::String &s)
                            { return s.getFloatValue(); });
        auto um = getUndoManager();

        depthMs.referTo(state, IDs::depthMs, um, 3.0f);
        speedHz.referTo(state, IDs::speedHz, um, 1.0f);
        width.referTo(state, IDs::width, um, 0.5f);
        mixProportion.referTo(state, IDs::mixProportion, um, 0.5f);

        // Attach parameters to their values
        depthParam->attachToCurrentValue(depthMs);
        speedParam->attachToCurrentValue(speedHz);
        widthParam->attachToCurrentValue(width);
        mixParam->attachToCurrentValue(mixProportion);
    }

    ~FlangerPlugin() override
    {
        notifyListenersOfDeletion();
    }

    static const char *getPluginName() { return NEEDS_TRANS("Flanger"); }
    static constexpr const char *xmlTypeName = "flanger";

    juce::String getName() const override { return TRANS("Flanger"); }
    juce::String getPluginType() override { return xmlTypeName; }
    juce::String getShortName(int) override { return getName(); }
    juce::String getSelectableDescription() override { return TRANS("Flanger Plugin"); }

    void initialise(const PluginInitialisationInfo& info) override 
    { 
        ChorusPlugin::initialise(info); 
    }

    void restorePluginStateFromValueTree(const juce::ValueTree& v) override 
    { 
        ChorusPlugin::restorePluginStateFromValueTree(v); 
    }

    AutomatableParameter::Ptr depthParam, speedParam,
        widthParam, mixParam;

    void setDepth(float value) { depthParam->setParameter(juce::jlimit(0.0f, 10.0f, value), juce::sendNotification); }
    float getDepth() { return depthParam->getCurrentValue(); }

    void setSpeed(float value) { speedParam->setParameter(juce::jlimit(0.0f, 10.0f, value), juce::sendNotification); }
    float getSpeed() { return speedParam->getCurrentValue(); }

    void setWidth(float value) { widthParam->setParameter(juce::jlimit(0.0f, 1.0f, value), juce::sendNotification); }
    float getWidth() { return widthParam->getCurrentValue(); }

    void setMix(float value) { mixParam->setParameter(juce::jlimit(0.0f, 1.0f, value), juce::sendNotification); }
    float getMix() { return mixParam->getCurrentValue(); }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FlangerPlugin)
};

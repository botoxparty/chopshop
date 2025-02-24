#pragma once

#include <tracktion_engine/tracktion_engine.h>

using namespace tracktion::engine;

class AutoDelayPlugin : public DelayPlugin
{
public:
    AutoDelayPlugin(PluginCreationInfo info)
        : DelayPlugin(info)
    {
        lengthMs = addParam("length", TRANS("Length"), { 0.0f, 1000.0f },
                         [] (float value) { return juce::String(value, 1) + " ms"; },
                         [] (const juce::String& s) { return s.getFloatValue(); });

        auto um = getUndoManager();
        length.referTo(state, IDs::length, um, 0.0f);
        lengthMs->attachToCurrentValue(length);
    }

    ~AutoDelayPlugin() override
    {
        notifyListenersOfDeletion();
    }

    static const char* getPluginName()                  { return NEEDS_TRANS("Auto Delay"); }
    static constexpr const char* xmlTypeName = "auto-delay";

    juce::String getName() const override               { return TRANS("Auto Delay"); }
    juce::String getPluginType() override              { return xmlTypeName; }
    juce::String getShortName(int) override            { return getName(); }
    juce::String getSelectableDescription() override   { return TRANS("Auto Delay Plugin"); }

    void setLength(float value)    { lengthMs->setParameter(juce::jlimit(0.0f, 1000.0f, value), juce::sendNotification); }
    float getLength()              { return lengthMs->getCurrentValue(); }

    AutomatableParameter::Ptr lengthMs;

private:
    juce::CachedValue<float> length;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AutoDelayPlugin)
};

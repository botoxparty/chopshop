#pragma once

#include <tracktion_engine/tracktion_engine.h>

using namespace tracktion::engine;

class AutoPhaserPlugin : public PhaserPlugin
{
public:
  AutoPhaserPlugin(PluginCreationInfo info)
      : PhaserPlugin(info)
  {
    depthParam = addParam("depth", TRANS("Depth"), {0.0f, 10.0f}, [](float value)
                          { return juce::String(value); }, [](const juce::String &s)
                          { return s.getFloatValue(); });

    rateParam = addParam("rate", TRANS("Rate"), {0.0f, 10.0f}, [](float value)
                         { return juce::String(value); }, [](const juce::String &s)
                         { return s.getFloatValue(); });

    feedbackGainParam = addParam("feedback", TRANS("Feedback"), {0.0f, 1.0f}, [](float value)
                                 { return juce::String(value); }, [](const juce::String &s)
                                 { return s.getFloatValue(); });

    depthParam->attachToCurrentValue(depth);
    rateParam->attachToCurrentValue(rate);
    feedbackGainParam->attachToCurrentValue(feedbackGain);
  }

  ~AutoPhaserPlugin() override
  {
    notifyListenersOfDeletion();
    depthParam->detachFromCurrentValue();
    rateParam->detachFromCurrentValue();
    feedbackGainParam->detachFromCurrentValue();
  }

  static const char *getPluginName() { return NEEDS_TRANS("Auto Phaser"); }
  static constexpr const char *xmlTypeName = "auto-phaser";

  juce::String getName() const override { return TRANS("Phaser"); }
  juce::String getPluginType() override { return xmlTypeName; }
  juce::String getShortName(int) override { return getName(); }
  juce::String getSelectableDescription() override { return TRANS("Auto Phaser Plugin"); }

  AutomatableParameter::Ptr depthParam, rateParam, feedbackGainParam;
};

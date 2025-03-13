#pragma once

#include "BaseEffectComponent.h"
#include "ChopTrackLane.h"

// Add this near the top of your file, outside the class definition
namespace CommandIDs
{
    static const int chopEffect = 1;
}

class ChopComponent : public BaseEffectComponent, 
                      public juce::ApplicationCommandTarget,
                      public juce::ApplicationCommandManagerListener,
                      public juce::Timer
{
public:
    explicit ChopComponent(tracktion::engine::Edit&);
    void resized() override;
    
    std::function<double()> getTempoCallback;

    double getChopDurationInBeats() const;
    ~ChopComponent() override;
    
    // ApplicationCommandTarget implementation
    juce::ApplicationCommandTarget* getNextCommandTarget() override;
    void getAllCommands(juce::Array<juce::CommandID>& commands) override;
    void getCommandInfo(juce::CommandID commandID, juce::ApplicationCommandInfo& result) override;
    bool perform(const juce::ApplicationCommandTarget::InvocationInfo& info) override;
    void setCommandManager(juce::ApplicationCommandManager* manager);

    // Required method from ApplicationCommandManagerListener
    void applicationCommandInvoked([[maybe_unused]] const juce::ApplicationCommandTarget::InvocationInfo& info) override {}
    void applicationCommandListChanged() override {}

    // Timer callback
    void timerCallback() override;

    void handleChopButtonPressed();
    void handleChopButtonReleased();

private:
    juce::Label durationLabel;
    juce::ComboBox chopDurationComboBox;
    juce::TextButton chopButton;
    juce::ApplicationCommandManager* commandManager = nullptr;
    tracktion::engine::AudioTrack::Ptr chopTrack;

    double chopStartTime = 0.0;
    double chopReleaseDelay = 0.0;



    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChopComponent)
}; 
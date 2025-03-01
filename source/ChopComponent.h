#pragma once

#include "BaseEffectComponent.h"

// Add this near the top of your file, outside the class definition
namespace CommandIDs
{
    static const int chopEffect = 1;
}

class ChopComponent : public BaseEffectComponent, 
                      public juce::ApplicationCommandTarget,
                      public juce::ApplicationCommandManagerListener
{
public:
    explicit ChopComponent(tracktion::engine::Edit&);
    void resized() override;
    
    std::function<void()> onChopButtonPressed;
    std::function<void()> onChopButtonReleased;
    std::function<void(float)> onCrossfaderValueChanged;

    double getChopDurationInMs(double currentTempo) const;
    float getCrossfaderValue() const { return static_cast<float>(crossfaderSlider.getValue()); }
    void setCrossfaderValue(float value) { crossfaderSlider.setValue(value, juce::sendNotification); }

    ~ChopComponent() override;
    
    // ApplicationCommandTarget implementation
    juce::ApplicationCommandTarget* getNextCommandTarget() override;
    void getAllCommands(juce::Array<juce::CommandID>& commands) override;
    void getCommandInfo(juce::CommandID commandID, juce::ApplicationCommandInfo& result) override;
    bool perform(const juce::ApplicationCommandTarget::InvocationInfo& info) override;
    void setCommandManager(juce::ApplicationCommandManager* manager);

    // Required method from ApplicationCommandManagerListener
    void applicationCommandInvoked([[maybe_unused]] const juce::ApplicationCommandTarget::InvocationInfo& info) override {}
    
    // Required method from ApplicationCommandManagerListener
    void applicationCommandListChanged() override {}

private:
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    juce::TextButton chopButton{"Chop"};
    juce::ComboBox chopDurationComboBox;
    juce::Label durationLabel;
    juce::Slider crossfaderSlider;
    juce::Label crossfaderLabel;

    // Change from std::unique_ptr to a raw pointer
    juce::ApplicationCommandManager* commandManager = nullptr;
    
    // Add this instance variable to track space key state for this instance
    bool wasSpaceDown = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChopComponent)
}; 
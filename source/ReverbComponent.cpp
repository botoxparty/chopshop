#include "ReverbComponent.h"

ReverbComponent::ReverbComponent(tracktion::engine::Edit& edit)
    : BaseEffectComponent(edit)
{
    setMixParameterId("wet level"); // Set the correct parameter ID for reverb
    reverbWetSlider.setComponentID("wet level");
    titleLabel.setText("Reverb", juce::dontSendNotification);
    // Configure labels
    roomSizeLabel.setText("Room Size", juce::dontSendNotification);
    wetLabel.setText("Wet Level", juce::dontSendNotification);
    
    roomSizeLabel.setJustificationType(juce::Justification::centred);
    wetLabel.setJustificationType(juce::Justification::centred);
    
    // Configure sliders
    reverbRoomSizeSlider.setTextValueSuffix("");
    reverbRoomSizeSlider.setNumDecimalPlacesToDisplay(2);
    reverbRoomSizeSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    reverbRoomSizeSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 15);
    
    reverbWetSlider.setTextValueSuffix("");
    reverbWetSlider.setNumDecimalPlacesToDisplay(2);
    reverbWetSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    reverbWetSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 15);
    
    addAndMakeVisible(roomSizeLabel);
    addAndMakeVisible(wetLabel);
    addAndMakeVisible(reverbRoomSizeSlider);
    addAndMakeVisible(reverbWetSlider);

    // Create and setup plugin
    plugin = createPlugin(tracktion::engine::ReverbPlugin::xmlTypeName);
    
    if (plugin != nullptr)
    {
        if (auto roomSizeParam = plugin->getAutomatableParameterByID("room size"))
            bindSliderToParameter(reverbRoomSizeSlider, *roomSizeParam);
            
        if (auto wetParam = plugin->getAutomatableParameterByID("wet level"))
            bindSliderToParameter(reverbWetSlider, *wetParam);
    }

    mixRamp.onValueChange = [this](float value) {
        reverbWetSlider.setValue(value, juce::sendNotification);
    };
}

void ReverbComponent::resized()
{
    auto bounds = getEffectiveArea();
    BaseEffectComponent::resized(); // This will handle the title label
    
    // Create a grid layout
    juce::Grid grid;
    grid.rowGap = juce::Grid::Px(4);
    grid.columnGap = juce::Grid::Px(4);
    
    using Track = juce::Grid::TrackInfo;
    using Fr = juce::Grid::Fr;
    
    grid.templateRows = { Track(Fr(1)), Track(Fr(2)) };    // Label row, Dial row
    grid.templateColumns = { Track(Fr(1)), Track(Fr(1)) };
    
    // Add items to grid
    grid.items = {
        juce::GridItem(roomSizeLabel),
        juce::GridItem(wetLabel),
        juce::GridItem(reverbRoomSizeSlider).withSize(60, 60).withJustifySelf(juce::GridItem::JustifySelf::center),
        juce::GridItem(reverbWetSlider).withSize(60, 60).withJustifySelf(juce::GridItem::JustifySelf::center)
    };
    
    grid.performLayout(bounds.toNearestInt());
}

void ReverbComponent::rampMixLevel(bool rampUp)
{
    if (rampUp)
    {
        storedMixValue = reverbWetSlider.getValue();
        mixRamp.startRamp(1.0);
    }
    else
    {
        mixRamp.startRamp(storedMixValue);
    }
}

void ReverbComponent::restoreMixLevel()
{
    rampMixLevel(false);
} 
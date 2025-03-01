/*
  ==============================================================================

    FlangerComponent.cpp
    Created: 17 Jan 2025 10:12:43pm
    Author:  Adam Hammad

  ==============================================================================
*/

#include "FlangerComponent.h"

FlangerComponent::FlangerComponent(tracktion::engine::Edit &edit)
    : BaseEffectComponent(edit)
{
    setMixParameterId("mix"); // Set the correct parameter ID for flanger
    mixSlider.setComponentID("mix");
    titleLabel.setText("Flanger", juce::dontSendNotification);
    // Configure labels
    depthLabel.setText("Depth", juce::dontSendNotification);
    speedLabel.setText("Speed", juce::dontSendNotification);
    widthLabel.setText("Width", juce::dontSendNotification);
    mixLabel.setText("Mix", juce::dontSendNotification);

    depthLabel.setJustificationType(juce::Justification::centred);
    speedLabel.setJustificationType(juce::Justification::centred);
    widthLabel.setJustificationType(juce::Justification::centred);
    mixLabel.setJustificationType(juce::Justification::centred);

    // Configure sliders as rotary dials
    depthSlider.setTextValueSuffix("");
    depthSlider.setNumDecimalPlacesToDisplay(2);
    depthSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    depthSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 15);

    speedSlider.setTextValueSuffix(" Hz");
    speedSlider.setNumDecimalPlacesToDisplay(2);
    speedSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    speedSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 15);

    widthSlider.setTextValueSuffix("");
    widthSlider.setNumDecimalPlacesToDisplay(2);
    widthSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    widthSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 15);

    mixSlider.setTextValueSuffix("%");
    mixSlider.setNumDecimalPlacesToDisplay(0);
    mixSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    mixSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 15);

    depthSlider.setDoubleClickReturnValue(true, 0.0);
    speedSlider.setDoubleClickReturnValue(true, 0.0);
    widthSlider.setDoubleClickReturnValue(true, 0.0);
    mixSlider.setDoubleClickReturnValue(true, 0.0);
    
    addAndMakeVisible(depthLabel);
    addAndMakeVisible(speedLabel);
    addAndMakeVisible(widthLabel);
    addAndMakeVisible(mixLabel);
    addAndMakeVisible(depthSlider);
    addAndMakeVisible(speedSlider);
    addAndMakeVisible(widthSlider);
    addAndMakeVisible(mixSlider);

    // Create and setup plugin
    plugin = createPlugin(FlangerPlugin::xmlTypeName);

    if (plugin != nullptr)
    {
               // Initialize all parameters to zero first
        for (auto param : plugin->getAutomatableParameters())
        {
            param->setParameter(0.0f, juce::sendNotification);
        }

        if (auto depthParam = plugin->getAutomatableParameterByID("depth"))
            bindSliderToParameter(depthSlider, *depthParam);
        else
            DBG("Depth parameter not found");

        if (auto speedParam = plugin->getAutomatableParameterByID("speed"))
            bindSliderToParameter(speedSlider, *speedParam);
        else
            DBG("Speed parameter not found");

        if (auto widthParam = plugin->getAutomatableParameterByID("width"))
            bindSliderToParameter(widthSlider, *widthParam);
        else
            DBG("Width parameter not found");

        if (auto mixParam = plugin->getAutomatableParameterByID("mix"))
        {   
            mixParam->setParameter(0.7f, juce::sendNotification);
            bindSliderToParameter(mixSlider, *mixParam);
        }
        else
            DBG("Mix parameter not found");
    }

    // Add to constructor after other slider setup:
    mixRamp.onValueChange = [this](float value) {
        mixSlider.setValue(value, juce::sendNotification);
    };
}

void FlangerComponent::resized()
{
    auto bounds = getEffectiveArea();
    BaseEffectComponent::resized();

    // Create a grid layout
    juce::Grid grid;
    grid.rowGap = juce::Grid::Px(4);
    grid.columnGap = juce::Grid::Px(4);

    using Track = juce::Grid::TrackInfo;
    using Fr = juce::Grid::Fr;

    grid.templateRows = {Track(Fr(1)), Track(Fr(2))}; // Label row, Dial row
    grid.templateColumns = {Track(Fr(1)), Track(Fr(1)), Track(Fr(1)), Track(Fr(1))};

    // Add items to grid
    grid.items = {
        juce::GridItem(depthLabel),
        juce::GridItem(speedLabel),
        juce::GridItem(widthLabel),
        juce::GridItem(mixLabel),
        juce::GridItem(depthSlider).withSize(60, 60).withJustifySelf(juce::GridItem::JustifySelf::center),
        juce::GridItem(speedSlider).withSize(60, 60).withJustifySelf(juce::GridItem::JustifySelf::center),
        juce::GridItem(widthSlider).withSize(60, 60).withJustifySelf(juce::GridItem::JustifySelf::center),
        juce::GridItem(mixSlider).withSize(60, 60).withJustifySelf(juce::GridItem::JustifySelf::center)};

    grid.performLayout(bounds.toNearestInt());
}

void FlangerComponent::setDepth(float value)
{
    depthSlider.setValue(value, juce::sendNotification);
}

void FlangerComponent::setSpeed(float value)
{
    speedSlider.setValue(value, juce::sendNotification);
}

void FlangerComponent::setWidth(float value)
{
    widthSlider.setValue(value, juce::sendNotification);
}

void FlangerComponent::setMix(float value)
{
    mixSlider.setValue(value, juce::sendNotification);
}

void FlangerComponent::rampMixLevel(bool rampUp)
{
    mixRamp.startRamp(rampUp ? 1.0f : 0.0f, 100);
}

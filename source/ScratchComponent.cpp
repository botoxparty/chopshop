#include "ScratchComponent.h"
#include "ScratchPlugin.h"

ScratchComponent::ScratchComponent(tracktion::engine::Edit& e) : BaseEffectComponent(e)
{
    // Create the scratch plugin
    plugin = EngineHelpers::getPluginFromRack(edit, ScratchPlugin::xmlTypeName);
    setMixParameterId("mix");

    // Create and setup the scratch slider
    scratchSlider = std::make_unique<SpringSlider>();
    scratchSlider->setSliderStyle(juce::Slider::SliderStyle::LinearHorizontal);
    scratchSlider->setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    scratchSlider->setRange(-1.0, 1.0);
    scratchSlider->setValue(0.0);
    scratchSlider->setComponentID("scratch");
    addAndMakeVisible(scratchSlider.get());

    // Create and setup the mix slider
    mixSlider = std::make_unique<juce::Slider>();
    mixSlider->setSliderStyle(juce::Slider::SliderStyle::LinearHorizontal);
    mixSlider->setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    mixSlider->setRange(0.0, 1.0);
    mixSlider->setValue(1.0);
    mixSlider->setComponentID("mix");
    addAndMakeVisible(mixSlider.get());

    // Bind sliders to parameters
    if (auto* scratchPlugin = dynamic_cast<ScratchPlugin*>(plugin.get()))
    {
        bindSliderToParameter(*scratchSlider, *scratchPlugin->scratchParam);
        bindSliderToParameter(*mixSlider, *scratchPlugin->mixParam);
    }
}

void ScratchComponent::resized()
{
    auto bounds = getLocalBounds().reduced(10);
    auto sliderHeight = bounds.getHeight() / 2;
    
    scratchSlider->setBounds(bounds.removeFromTop(sliderHeight));
    mixSlider->setBounds(bounds);
}

void ScratchComponent::paint(juce::Graphics& g)
{
    BaseEffectComponent::paint(g);
    
    g.setColour(juce::Colours::white);
    g.setFont(14.0f);
    
    auto bounds = getLocalBounds().reduced(10);
    auto sliderHeight = bounds.getHeight() / 2;
    
    g.drawText("Scratch", bounds.removeFromTop(sliderHeight), juce::Justification::centredLeft);
    g.drawText("Mix", bounds, juce::Justification::centredLeft);
}

void ScratchComponent::setScratchValue(double value)
{
    if (auto* scratchPlugin = dynamic_cast<ScratchPlugin*>(plugin.get()))
    {
        if (auto param = scratchPlugin->scratchParam)
        {
            param->setParameter(value, juce::sendNotification);
            scratchSlider->setValue(value, juce::sendNotification);
        }
    }
}

ScratchComponent::~ScratchComponent()
{
    // Clean up any resources if needed
}
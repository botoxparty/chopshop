#include "ScratchComponent.h"
#include "Plugins/ScratchPlugin.h"

ScratchComponent::ScratchComponent(tracktion::engine::Edit& e) : BaseEffectComponent(e)
{
    // Create the scratch plugin
    plugin = EngineHelpers::getPluginFromRack(edit, ScratchPlugin::xmlTypeName);
    setMixParameterId("mix");
    
    titleLabel.setText("Scratch", juce::dontSendNotification);

    // Create and setup the scratch slider
    scratchSlider = std::make_unique<SpringSlider>();
    scratchSlider->setSliderStyle(juce::Slider::SliderStyle::LinearHorizontal);
    scratchSlider->setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    scratchSlider->setRange(-1.0, 1.0);
    scratchSlider->setValue(0.0);
    scratchSlider->setComponentID("scratch");
    addAndMakeVisible(scratchSlider.get());

    // Bind sliders to parameters
    if (auto* scratchPlugin = dynamic_cast<ScratchPlugin*>(plugin.get()))
    {
        // Special binding for scratch slider to preserve spring behavior
        scratchSlider->setRange(scratchPlugin->scratchParam->getValueRange().getStart(), 
                               scratchPlugin->scratchParam->getValueRange().getEnd(), 
                               0.01);
        scratchSlider->setValue(scratchPlugin->scratchParam->getCurrentValue(), juce::dontSendNotification);
        
        scratchSlider->onValueChange = [scratchPlugin, this](){ 
            scratchPlugin->scratchParam->setParameter(static_cast<float>(scratchSlider->getValue()), 
                                                    juce::sendNotification); 
        };
        
        scratchSlider->onDragStart = [scratchPlugin](){ 
            scratchPlugin->scratchParam->parameterChangeGestureBegin(); 
        };
        
        scratchSlider->addParameterBinding([scratchPlugin](){ 
            scratchPlugin->scratchParam->parameterChangeGestureEnd(); 
        });
    }
}

void ScratchComponent::resized()
{
    auto bounds = getLocalBounds().reduced(10);
    auto sliderHeight = bounds.getHeight() / 2;
    
    scratchSlider->setBounds(bounds.removeFromTop(sliderHeight));
}

void ScratchComponent::paint(juce::Graphics& g)
{
    BaseEffectComponent::paint(g);
    
    g.setColour(juce::Colours::white);
    g.setFont(14.0f);
    
    auto bounds = getLocalBounds().reduced(10);
    auto sliderHeight = bounds.getHeight() / 2;
    
    g.drawText("Scratch", bounds.removeFromTop(sliderHeight), juce::Justification::centredLeft);
}

ScratchComponent::~ScratchComponent()
{
    // Clean up any resources if needed
}
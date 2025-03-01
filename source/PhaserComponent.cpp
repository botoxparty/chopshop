#include "PhaserComponent.h"

PhaserComponent::PhaserComponent(tracktion::engine::Edit& edit)
    : BaseEffectComponent(edit)
{
    titleLabel.setText("Phaser", juce::dontSendNotification);
    
    // Configure labels
    depthLabel.setText("Depth", juce::dontSendNotification);
    rateLabel.setText("Rate", juce::dontSendNotification);
    feedbackLabel.setText("Feedback", juce::dontSendNotification);
    
    depthLabel.setJustificationType(juce::Justification::centred);
    rateLabel.setJustificationType(juce::Justification::centred);
    feedbackLabel.setJustificationType(juce::Justification::centred);
    
    // Configure sliders based on the actual plugin parameters
    // Depth: initialize to 0.0f
    depthSlider.setRange(0.0, 10.0, 0.1);
    depthSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    depthSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 15);
    
    // Rate: initialize to 0.0f
    rateSlider.setRange(0.0, 10.0, 0.01);
    rateSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    rateSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 15);
    
    // Feedback: initialize to 0.0f
    feedbackSlider.setRange(0.0, 0.99, 0.01);
    feedbackSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    feedbackSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 15);
    
    depthSlider.setDoubleClickReturnValue(true, 0.0);
    rateSlider.setDoubleClickReturnValue(true, 0.0);
    feedbackSlider.setDoubleClickReturnValue(true, 0.0);

    addAndMakeVisible(depthLabel);
    addAndMakeVisible(rateLabel);
    addAndMakeVisible(feedbackLabel);
    addAndMakeVisible(depthSlider);
    addAndMakeVisible(rateSlider);
    addAndMakeVisible(feedbackSlider);

    // Create and setup plugin
    plugin = createPlugin(AutoPhaserPlugin::xmlTypeName);
    
    if (plugin != nullptr)
    {
        // Initialize all parameters to zero first
        for (auto param : plugin->getAutomatableParameters())
        {
            param->setParameter(0.0f, juce::sendNotification);
        }

        // Use the correct parameter IDs from the plugin
        if (auto depthParam = plugin->getAutomatableParameterByID("depth"))
        {
            bindSliderToParameter(depthSlider, *depthParam);
        }
        
        if (auto rateParam = plugin->getAutomatableParameterByID("rate"))
        {
            bindSliderToParameter(rateSlider, *rateParam);
        }
        
        if (auto feedbackParam = plugin->getAutomatableParameterByID("feedback"))
        {
            bindSliderToParameter(feedbackSlider, *feedbackParam);
        }

    }
}

void PhaserComponent::resized()
{
    auto bounds = getEffectiveArea();
    BaseEffectComponent::resized();
    
    // Create a grid layout
    juce::Grid grid;
    grid.rowGap = juce::Grid::Px(4);
    grid.columnGap = juce::Grid::Px(4);
    
    using Track = juce::Grid::TrackInfo;
    using Fr = juce::Grid::Fr;
    
    grid.templateRows = { Track(Fr(1)), Track(Fr(2)) };    // Label row, Dial row
    grid.templateColumns = { Track(Fr(1)), Track(Fr(1)), Track(Fr(1)) };  // Changed to 3 columns
    
    // Add items to grid
    grid.items = {
        juce::GridItem(depthLabel),
        juce::GridItem(rateLabel),
        juce::GridItem(feedbackLabel),
        juce::GridItem(depthSlider).withSize(60, 60).withJustifySelf(juce::GridItem::JustifySelf::center),
        juce::GridItem(rateSlider).withSize(60, 60).withJustifySelf(juce::GridItem::JustifySelf::center),
        juce::GridItem(feedbackSlider).withSize(60, 60).withJustifySelf(juce::GridItem::JustifySelf::center)
    };
    
    grid.performLayout(bounds.toNearestInt());
}

void PhaserComponent::setDepth(float value)
{
    depthSlider.setValue(value * 10.0f, juce::sendNotification);
}

void PhaserComponent::setRate(float value)
{
    rateSlider.setValue(value * 10.0f, juce::sendNotification);
}

void PhaserComponent::setFeedback(float value)
{
    feedbackSlider.setValue(value, juce::sendNotification);
} 
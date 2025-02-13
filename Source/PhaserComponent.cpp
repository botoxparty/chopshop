#include "PhaserComponent.h"

PhaserComponent::PhaserComponent(tracktion_engine::Edit& edit)
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
    // Depth: default 5.0f
    depthSlider.setRange(0.0, 10.0, 0.1);
    depthSlider.setValue(5.0, juce::dontSendNotification);
    depthSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    depthSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 15);
    
    // Rate: default 0.4f
    rateSlider.setRange(0.0, 10.0, 0.01);
    rateSlider.setValue(0.4, juce::dontSendNotification);
    rateSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    rateSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 15);
    
    // Feedback: default 0.7f
    feedbackSlider.setRange(0.0, 0.99, 0.01);
    feedbackSlider.setValue(0.7, juce::dontSendNotification);
    feedbackSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    feedbackSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 15);
    
    addAndMakeVisible(depthLabel);
    addAndMakeVisible(rateLabel);
    addAndMakeVisible(feedbackLabel);
    addAndMakeVisible(depthSlider);
    addAndMakeVisible(rateSlider);
    addAndMakeVisible(feedbackSlider);

    // Create and setup plugin
    plugin = createPlugin(tracktion_engine::PhaserPlugin::xmlTypeName);
    
    if (plugin != nullptr)
    {
        // Use the correct parameter IDs from the plugin
        if (auto depthParam = plugin->getAutomatableParameterByID("depth"))
            bindSliderToParameter(depthSlider, *depthParam);
        
        if (auto rateParam = plugin->getAutomatableParameterByID("rate"))
            bindSliderToParameter(rateSlider, *rateParam);
        
        if (auto feedbackParam = plugin->getAutomatableParameterByID("feedback"))
            bindSliderToParameter(feedbackSlider, *feedbackParam);
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
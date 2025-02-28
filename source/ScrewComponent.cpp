#include "ScrewComponent.h"

ScrewComponent::ScrewComponent(tracktion_engine::Edit& edit)
    : BaseEffectComponent(edit)
{
    titleLabel.setText("Screw Controls", juce::dontSendNotification);
    
    // Configure tempo slider
    tempoSlider.setRange(30.0, 220.0, 0.1);
    tempoSlider.setTextValueSuffix(" BPM");
    tempoSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    tempoSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 70, 20);
    
    // Add all components to make them visible
    addAndMakeVisible(tempoSlider);
    addAndMakeVisible(tempo70Button);
    addAndMakeVisible(tempo75Button);
    addAndMakeVisible(tempo80Button);
    addAndMakeVisible(tempo85Button);
    addAndMakeVisible(tempo100Button);
    
    tempoSlider.onValueChange = [this] {
        if (onTempoChanged)
            onTempoChanged(tempoSlider.getValue());
        updateTempoButtonStates();
    };
    
    // Configure tempo buttons
    tempo70Button.onClick = [this] { setTempoPercentage(0.70); };
    tempo75Button.onClick = [this] { setTempoPercentage(0.75); };
    tempo80Button.onClick = [this] { setTempoPercentage(0.80); };
    tempo85Button.onClick = [this] { setTempoPercentage(0.85); };
    tempo100Button.onClick = [this] { setTempoPercentage(1.00); };
    resized();
}

void ScrewComponent::resized()
{
    auto bounds = getEffectiveArea();
    BaseEffectComponent::resized();
    
    // Create a grid layout
    juce::Grid grid;
    grid.rowGap = juce::Grid::Px(4);
    grid.columnGap = juce::Grid::Px(4);
    
    using Track = juce::Grid::TrackInfo;
    using Fr = juce::Grid::Fr;
    
    grid.templateRows = { Track(Fr(1)), Track(Fr(1)) };
    grid.templateColumns = { Track(Fr(1)) };
    
    // Create and setup tempo buttons container
    auto* tempoButtonComponent = new juce::Component();
    addAndMakeVisible(tempoButtonComponent);
    tempoButtonComponent->setBounds(bounds.removeFromTop(30).toNearestInt());
    
    juce::FlexBox tempoButtonBox;
    tempoButtonBox.flexDirection = juce::FlexBox::Direction::row;
    tempoButtonBox.items.add(juce::FlexItem(tempo70Button).withFlex(1.0f).withMargin(2));
    tempoButtonBox.items.add(juce::FlexItem(tempo75Button).withFlex(1.0f).withMargin(2));
    tempoButtonBox.items.add(juce::FlexItem(tempo80Button).withFlex(1.0f).withMargin(2));
    tempoButtonBox.items.add(juce::FlexItem(tempo85Button).withFlex(1.0f).withMargin(2));
    tempoButtonBox.items.add(juce::FlexItem(tempo100Button).withFlex(1.0f).withMargin(2));
    
    tempoButtonBox.performLayout(tempoButtonComponent->getBounds().toFloat());
    
    grid.items = {
        juce::GridItem(*tempoButtonComponent),
        juce::GridItem(tempoSlider)
    };
    
    grid.performLayout(bounds.toNearestInt());
}

void ScrewComponent::setTempo(double tempo, juce::NotificationType notification)
{
    tempoSlider.setValue(tempo, notification);
}

void ScrewComponent::setTempoPercentage(double percentage)
{
    // Calculate the new tempo based on the percentage of base tempo
    double newTempo = baseTempo * percentage;
    
    // Set the tempo slider value which will trigger the onTempoChanged callback
    setTempo(newTempo, juce::sendNotification);
    
    // Call the percentage changed callback if it exists
    if (onTempoPercentageChanged)
        onTempoPercentageChanged(percentage);
        
    updateTempoButtonStates();
}

bool ScrewComponent::isTempoPercentageActive(double percentage) const
{
    const double currentPercentage = tempoSlider.getValue() / baseTempo;
    return std::abs(currentPercentage - percentage) < 0.001;
}

void ScrewComponent::updateTempoButtonStates()
{
    tempo70Button.setToggleState(isTempoPercentageActive(0.70), juce::dontSendNotification);
    tempo75Button.setToggleState(isTempoPercentageActive(0.75), juce::dontSendNotification);
    tempo80Button.setToggleState(isTempoPercentageActive(0.80), juce::dontSendNotification);
    tempo85Button.setToggleState(isTempoPercentageActive(0.85), juce::dontSendNotification);
    tempo100Button.setToggleState(isTempoPercentageActive(1.00), juce::dontSendNotification);
}

void ScrewComponent::setBaseTempo(double tempo)
{
    baseTempo = tempo;
    
    // Set the range to 50% - 110% of the base tempo
    double minTempo = baseTempo * 0.5;
    double maxTempo = baseTempo * 1.1;
    
    // Update the slider range
    tempoSlider.setRange(minTempo, maxTempo, 0.1);
    
    updateTempoButtonStates();
} 
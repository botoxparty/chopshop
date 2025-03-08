#include "PluginAutomationComponent.h"

PluginAutomationComponent::PluginAutomationComponent(tracktion::engine::Edit& e, tracktion::engine::Plugin* p)
    : edit(e)
{
    setPlugin(p);
}

PluginAutomationComponent::~PluginAutomationComponent()
{
}

void PluginAutomationComponent::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
    
    // Draw plugin name if available
    if (plugin != nullptr)
    {
        g.setColour(juce::Colours::white);
        g.setFont(juce::Font(16.0f).boldened());
        g.drawText(plugin->getName(), getLocalBounds().removeFromTop(30), juce::Justification::centred, true);
    }
}

void PluginAutomationComponent::resized()
{
    auto bounds = getLocalBounds();
    
    // Reserve space for plugin name
    bounds.removeFromTop(30);
    
    // Layout automation lanes
    for (auto& laneInfo : automationLanes)
    {
        auto laneBounds = bounds.removeFromTop(laneHeight);
        
        // Position automation lane to take full width
        if (laneInfo.lane != nullptr)
            laneInfo.lane->setBounds(laneBounds);
            
        // Position label as a tag in top left corner of the lane
        if (laneInfo.nameLabel != nullptr)
        {
            const int tagWidth = 100;  // Adjust width as needed
            const int tagHeight = 20;   // Adjust height as needed
            const int margin = 5;       // Margin from the edges
            laneInfo.nameLabel->setBounds(laneBounds.getX() + margin, 
                                        laneBounds.getY() + margin, 
                                        tagWidth, 
                                        tagHeight);
        }
    }
}

void PluginAutomationComponent::setPlugin(tracktion::engine::Plugin* p)
{
    plugin = p;
    updateAutomationLanes();
    resized();
}

void PluginAutomationComponent::setZoomLevel(double newZoomLevel)
{
    zoomLevel = newZoomLevel;
    
    for (auto& laneInfo : automationLanes)
        if (laneInfo.lane != nullptr)
            laneInfo.lane->setZoomLevel(zoomLevel);
}

void PluginAutomationComponent::setScrollPosition(double newScrollPosition)
{
    scrollPosition = newScrollPosition;
    
    for (auto& laneInfo : automationLanes)
        if (laneInfo.lane != nullptr)
            laneInfo.lane->setScrollPosition(scrollPosition);
}


void PluginAutomationComponent::setClip(tracktion::engine::WaveAudioClip* clip)
{
    currentClip = clip;
    for (auto& laneInfo : automationLanes)
        if (laneInfo.lane != nullptr)
            laneInfo.lane->setClip(currentClip);
}

void PluginAutomationComponent::updateAutomationLanes()
{
    // Clear existing lanes
    automationLanes.clear();
    
    if (plugin != nullptr)
    {
        // Get all automatable parameters from the plugin
        auto params = plugin->getAutomatableParameters();
        
        // Create a lane for each parameter
        for (auto* param : params)
        {
            if (param != nullptr)
                createAutomationLaneForParameter(param);
        }
    }
    
    resized();
}

void PluginAutomationComponent::createAutomationLaneForParameter(tracktion::engine::AutomatableParameter* param)
{
    if (param == nullptr)
        return;
        
    AutomationLaneInfo laneInfo;
    
    // Create and setup automation lane
    laneInfo.lane = std::make_unique<AutomationLane>(edit, param);
    laneInfo.lane->setZoomLevel(zoomLevel);
    laneInfo.lane->setScrollPosition(scrollPosition);
    laneInfo.lane->setClip(currentClip);
    addAndMakeVisible(*laneInfo.lane);
    
    // Setup label
    laneInfo.nameLabel->setText(param->getParameterName(), juce::dontSendNotification);
    laneInfo.nameLabel->setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(*laneInfo.nameLabel);
    
    automationLanes.push_back(std::move(laneInfo));
} 
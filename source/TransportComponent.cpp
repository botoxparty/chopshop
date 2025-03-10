#include "TransportComponent.h"

TransportComponent::TransportComponent(tracktion::engine::Edit& e)
    : edit(e),
      transport(e.getTransport()),
      transportBar(e)
{
    // Add transport bar
    addAndMakeVisible(transportBar);

    // Create and add thumbnail component with zoom state
    thumbnailComponent = std::make_unique<ThumbnailComponent>(edit, zoomState);
    addAndMakeVisible(*thumbnailComponent);

    // Add and make visible other components
    addAndMakeVisible(pluginAutomationViewport);
    pluginAutomationViewport.setViewedComponent(&pluginAutomationContainer, false);
    pluginAutomationViewport.setScrollBarsShown(true, false);

    // Create and add crossfader automation lane with zoom state
    crossfaderAutomationLane = std::make_unique<CrossfaderAutomationLane>(edit, zoomState);
    
    // Find the crossfader parameter
    tracktion::engine::AutomatableParameter* crossfaderParam = nullptr;
    if (auto chopPlugin = EngineHelpers::getPluginFromMasterTrack(edit, ChopPlugin::xmlTypeName))
    {
        if (auto* plugin = chopPlugin.get())
        {
            crossfaderParam = plugin->getAutomatableParameterByID("crossfader");
        }
    }
    
    if (crossfaderParam != nullptr)
    {
        crossfaderAutomationLane->setParameter(crossfaderParam);
    }
    
    addAndMakeVisible(*crossfaderAutomationLane);

    // Create and add automation lane for reverb wet parameter with zoom state
    reverbWetAutomationLane = std::make_unique<AutomationLane>(edit, zoomState);
    
    // Find the reverb plugin and create automation component
    if (auto reverbPlugin = EngineHelpers::getPluginFromRack(edit, tracktion::engine::ReverbPlugin::xmlTypeName))
    {
        reverbAutomationComponent = std::make_unique<PluginAutomationComponent>(edit, reverbPlugin.get(), zoomState);
        pluginAutomationContainer.addAndMakeVisible(*reverbAutomationComponent);
    }

    // Find the delay plugin and create automation component
    if (auto delayPlugin = EngineHelpers::getPluginFromRack(edit, AutoDelayPlugin::xmlTypeName))
    {
        delayAutomationComponent = std::make_unique<PluginAutomationComponent>(edit, delayPlugin.get(), zoomState);
        pluginAutomationContainer.addAndMakeVisible(*delayAutomationComponent);
    }

    // Find the phaser plugin and create automation component
    if (auto phaserPlugin = EngineHelpers::getPluginFromRack(edit, AutoPhaserPlugin::xmlTypeName))
    {
        phaserAutomationComponent = std::make_unique<PluginAutomationComponent>(edit, phaserPlugin.get(), zoomState);
        pluginAutomationContainer.addAndMakeVisible(*phaserAutomationComponent);
    }

    // Find the flanger plugin and create automation component
    if (auto flangerPlugin = EngineHelpers::getPluginFromRack(edit, FlangerPlugin::xmlTypeName))
    {
        flangerAutomationComponent = std::make_unique<PluginAutomationComponent>(edit, flangerPlugin.get(), zoomState);
        pluginAutomationContainer.addAndMakeVisible(*flangerAutomationComponent);
    }

    // Register as automation listener
    edit.getAutomationRecordManager().addListener(this);
}

TransportComponent::~TransportComponent()
{
    // Remove automation listener
    edit.getAutomationRecordManager().removeListener(this);
}

void TransportComponent::timerCallback()
{
    // Force thumbnail redraw during playback
    if (transport.isPlaying())
        repaint();
}

void TransportComponent::paint(juce::Graphics& g)
{
    // Background is now handled by child components
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
}

void TransportComponent::resized()
{
    auto bounds = getLocalBounds();
    
    // Constants for layout
    const int controlBarHeight = 26;  // Fixed control bar height
    const int crossfaderHeight = 25;  // Fixed crossfader height
    const int thumbnailHeight = 60;   // Fixed thumbnail height
    
    // Create main FlexBox for vertical layout of all components
    juce::FlexBox mainFlex;
    mainFlex.flexDirection = juce::FlexBox::Direction::column;
    mainFlex.flexWrap = juce::FlexBox::Wrap::noWrap;
    mainFlex.justifyContent = juce::FlexBox::JustifyContent::flexStart;
    mainFlex.alignItems = juce::FlexBox::AlignItems::stretch;
    
    // 1. Add transport bar
    mainFlex.items.add(juce::FlexItem(transportBar).withHeight(controlBarHeight).withMargin(juce::FlexItem::Margin(0, 5, 0, 5)));
    
    // 2. Add thumbnail section
    mainFlex.items.add(juce::FlexItem(*thumbnailComponent).withHeight(thumbnailHeight).withMargin(juce::FlexItem::Margin(0, 0, 0, 0)));
    
    // 3. Add crossfader automation lane
    if (crossfaderAutomationLane != nullptr)
    {
        mainFlex.items.add(juce::FlexItem(*crossfaderAutomationLane).withHeight(crossfaderHeight).withMargin(juce::FlexItem::Margin(1, 0, 1, 0)));
    }
    
    // 4. Add plugin automation viewport
    mainFlex.items.add(juce::FlexItem(pluginAutomationViewport).withFlex(1.0f).withMargin(juce::FlexItem::Margin(0, 0, 0, 0)));
    
    // Perform the main layout
    mainFlex.performLayout(bounds);
    
    // Setup plugin automation container layout
    juce::FlexBox pluginFlex;
    pluginFlex.flexDirection = juce::FlexBox::Direction::column;
    pluginFlex.flexWrap = juce::FlexBox::Wrap::noWrap;
    
    // Add automation components to plugin flex layout
    if (reverbAutomationComponent != nullptr)
        pluginFlex.items.add(juce::FlexItem(*reverbAutomationComponent).withFlex(1.0f).withMargin(juce::FlexItem::Margin(0, 0, 1, 0)));
    
    if (delayAutomationComponent != nullptr)
        pluginFlex.items.add(juce::FlexItem(*delayAutomationComponent).withFlex(1.0f).withMargin(juce::FlexItem::Margin(0, 0, 1, 0)));
    
    if (phaserAutomationComponent != nullptr)
        pluginFlex.items.add(juce::FlexItem(*phaserAutomationComponent).withFlex(1.0f).withMargin(juce::FlexItem::Margin(0, 0, 1, 0)));
    
    if (flangerAutomationComponent != nullptr)
        pluginFlex.items.add(juce::FlexItem(*flangerAutomationComponent).withFlex(1.0f).withMargin(juce::FlexItem::Margin(0, 0, 1, 0)));
    
    // Calculate total height for plugin container
    auto totalHeight = pluginAutomationViewport.getHeight() * 4;
    
    // Set container bounds and perform plugin layout
    pluginAutomationContainer.setBounds(0, 0, pluginAutomationViewport.getWidth(), totalHeight);
    pluginFlex.performLayout(pluginAutomationContainer.getBounds());
}

void TransportComponent::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    repaint();
}

void TransportComponent::mouseDown(const juce::MouseEvent& event)
{
    // Mouse events are now handled by child components
}

void TransportComponent::mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel)
{
    // Mouse wheel events are now handled by child components
}

void TransportComponent::updateThumbnail()
{
    if (thumbnailComponent != nullptr)
        thumbnailComponent->updateThumbnail();
}

void TransportComponent::deleteSelectedChopRegion()
{
    if (crossfaderAutomationLane != nullptr)
    {
        crossfaderAutomationLane->deleteSelectedRegion();
    }
}

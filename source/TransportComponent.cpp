#include "TransportComponent.h"
#include "Utilities.h"
#include "Plugins/ChopPlugin.h"
#include "Plugins/AutoDelayPlugin.h"
#include "Plugins/AutoPhaserPlugin.h"
#include "Plugins/FlangerPlugin.h"

TransportComponent::TransportComponent(tracktion::engine::Edit& e, ZoomState& zs)
    : edit(e),
      transport(e.getTransport()),
      zoomState(zs),
      transportBar(e, zs)
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

    // Set up snap callback
    transportBar.setSnapCallback([this](bool snapEnabled) {
        setSnapEnabled(snapEnabled);
    });

    // Find and create plugin automation components
    if (auto reverbPlugin = EngineHelpers::getPluginFromRack(edit, tracktion::engine::ReverbPlugin::xmlTypeName))
    {
        reverbAutomationComponent = std::make_unique<PluginAutomationComponent>(edit, reverbPlugin.get(), zoomState);
        pluginAutomationContainer.addPluginComponent(reverbAutomationComponent.get());
    }

    if (auto delayPlugin = EngineHelpers::getPluginFromRack(edit, AutoDelayPlugin::xmlTypeName))
    {
        delayAutomationComponent = std::make_unique<PluginAutomationComponent>(edit, delayPlugin.get(), zoomState);
        pluginAutomationContainer.addPluginComponent(delayAutomationComponent.get());
    }

    if (auto phaserPlugin = EngineHelpers::getPluginFromRack(edit, AutoPhaserPlugin::xmlTypeName))
    {
        phaserAutomationComponent = std::make_unique<PluginAutomationComponent>(edit, phaserPlugin.get(), zoomState);
        pluginAutomationContainer.addPluginComponent(phaserAutomationComponent.get());
    }

    if (auto flangerPlugin = EngineHelpers::getPluginFromRack(edit, FlangerPlugin::xmlTypeName))
    {
        flangerAutomationComponent = std::make_unique<PluginAutomationComponent>(edit, flangerPlugin.get(), zoomState);
        pluginAutomationContainer.addPluginComponent(flangerAutomationComponent.get());
    }

    // Initialize layout manager
    // We'll use indices 0-7 for our components (2 indices per component for spacing)
    for (int i = 0; i < 8; ++i)
        itemComponents.push_back(i);

    // Set minimum sizes
    layoutManager.setItemLayout(0, controlBarHeight, controlBarHeight, controlBarHeight);  // Transport bar (fixed)
    layoutManager.setItemLayout(1, 2, 2, 2);  // Spacing
    layoutManager.setItemLayout(2, thumbnailHeight, thumbnailHeight, thumbnailHeight);  // Thumbnail (fixed)
    layoutManager.setItemLayout(3, 2, 2, 2);  // Spacing
    layoutManager.setItemLayout(4, crossfaderHeight, crossfaderHeight, crossfaderHeight);  // Crossfader (fixed)
    layoutManager.setItemLayout(5, 2, 2, 2);  // Spacing
    layoutManager.setItemLayout(6, minPluginHeight, -1.0, -1.0);  // Plugin container (stretches)
    layoutManager.setItemLayout(7, 2, 2, 2);  // Final spacing

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
    layoutItemsWithCurrentBounds();
}

void TransportComponent::layoutItemsWithCurrentBounds()
{
    auto bounds = getLocalBounds();
    auto w = bounds.getWidth();
    
    // Calculate positions for fixed-height components
    transportBar.setBounds(bounds.getX(), bounds.getY(), w, controlBarHeight);
    
    auto y = bounds.getY() + controlBarHeight + 2; // Add spacing
    
    if (thumbnailComponent != nullptr)
    {
        thumbnailComponent->setBounds(bounds.getX(), y, w, thumbnailHeight);
        y += thumbnailHeight + 2; // Add spacing
    }
    
    if (crossfaderAutomationLane != nullptr)
    {
        crossfaderAutomationLane->setBounds(bounds.getX(), y, w, crossfaderHeight);
        y += crossfaderHeight + 2; // Add spacing
    }
    
    // Viewport gets remaining height
    auto remainingHeight = bounds.getBottom() - y;
    pluginAutomationViewport.setBounds(bounds.getX(), y, w, remainingHeight);
    
    // Layout plugin components in container
    auto containerHeight = 0;
    const int spacing = 1;
    
    if (reverbAutomationComponent != nullptr)
    {
        auto height = reverbAutomationComponent->getPreferredHeight();
        reverbAutomationComponent->setBounds(0, containerHeight, w, height);
        containerHeight += height + spacing;
    }
    
    if (delayAutomationComponent != nullptr)
    {
        auto height = delayAutomationComponent->getPreferredHeight();
        delayAutomationComponent->setBounds(0, containerHeight, w, height);
        containerHeight += height + spacing;
    }
    
    if (phaserAutomationComponent != nullptr)
    {
        auto height = phaserAutomationComponent->getPreferredHeight();
        phaserAutomationComponent->setBounds(0, containerHeight, w, height);
        containerHeight += height + spacing;
    }
    
    if (flangerAutomationComponent != nullptr)
    {
        auto height = flangerAutomationComponent->getPreferredHeight();
        flangerAutomationComponent->setBounds(0, containerHeight, w, height);
        containerHeight += height + spacing;
    }
    
    // Update container size
    pluginAutomationContainer.setSize(w, containerHeight);
}

void TransportComponent::updateLayout()
{
    // This method can be called when plugin components are collapsed/expanded
    layoutItemsWithCurrentBounds();
    repaint();
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

void TransportComponent::setSnapEnabled(bool shouldSnap)
{
    if (crossfaderAutomationLane != nullptr)
    {
        crossfaderAutomationLane->setSnapToGrid(shouldSnap);
    }
}

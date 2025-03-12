#pragma once

#include <juce_core/juce_core.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <tracktion_engine/tracktion_engine.h>

#include "Utilities.h"
#include "AutomationLane.h"
#include "CustomLookAndFeel.h"
#include "Plugins/ChopPlugin.h"
#include "Plugins/AutoDelayPlugin.h"
#include "Plugins/AutoPhaserPlugin.h"
#include "Plugins/FlangerPlugin.h"
#include "CrossfaderAutomationLane.h"
#include "PluginAutomationComponent.h"
#include "TransportBar.h"
#include "ThumbnailComponent.h"
#include "ZoomState.h"

class PluginAutomationContainer : public juce::Component,
                                public PluginAutomationComponent::HeightListener
{
public:
    PluginAutomationContainer() = default;
    ~PluginAutomationContainer() override = default;

    void heightChanged() override
    {
        updateContainerBounds();
    }

    void addPluginComponent(PluginAutomationComponent* component)
    {
        if (component != nullptr)
        {
            addAndMakeVisible(component);
            component->addHeightListener(this);
            components.add(component);
            updateContainerBounds();
        }
    }

    void resized() override
    {
        updateContainerBounds();
    }

private:
    void updateContainerBounds()
    {
        auto w = getWidth();
        auto containerHeight = 0;
        const int spacing = 1;

        for (auto* component : components)
        {
            if (component != nullptr)
            {
                auto height = component->getPreferredHeight();
                component->setBounds(0, containerHeight, w, height);
                containerHeight += height + spacing;
            }
        }

        setSize(w, containerHeight);
    }

    juce::Array<PluginAutomationComponent*> components;
};

class TransportComponent : public juce::Component,
                         public juce::Timer,
                         public juce::ChangeListener,
                         public tracktion::engine::AutomationRecordManager::Listener
{
public:
    TransportComponent(tracktion::engine::Edit&);
    ~TransportComponent() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;
    void changeListenerCallback(juce::ChangeBroadcaster*) override;
    
    void mouseDown(const juce::MouseEvent&) override;
    void mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    
    void updateThumbnail();
    void deleteSelectedChopRegion();
    void setSnapEnabled(bool shouldSnap);

    void updatePlayheadPosition();
    void setZoomLevel(double newLevel);
    void setScrollPosition(double newPosition);
    double getMaxScrollPosition() const;

    // Expose zoom state for child components
    ZoomState& getZoomState() { return zoomState; }

private:
    void updateLayout();
    void layoutItemsWithCurrentBounds();

    tracktion::engine::Edit& edit;
    tracktion::engine::TransportControl& transport;
    
    // Transport bar
    TransportBar transportBar;
    
    // Waveform thumbnail
    std::unique_ptr<ThumbnailComponent> thumbnailComponent;
    
    std::unique_ptr<CrossfaderAutomationLane> crossfaderAutomationLane;
    std::unique_ptr<AutomationLane> reverbWetAutomationLane;
    
    // Plugin automation components
    std::unique_ptr<PluginAutomationComponent> reverbAutomationComponent;
    std::unique_ptr<PluginAutomationComponent> delayAutomationComponent;
    std::unique_ptr<PluginAutomationComponent> phaserAutomationComponent;
    std::unique_ptr<PluginAutomationComponent> flangerAutomationComponent;
    
    juce::Viewport pluginAutomationViewport;
    PluginAutomationContainer pluginAutomationContainer;
    
    // Zoom and scroll state
    double zoomLevel = 1.0;
    double scrollPosition = 0.0; // 0.0 to 1.0
    static constexpr double minZoom = 1.0; // Changed from 0.1 to 1.0 (100%)
    static constexpr double maxZoom = 20.0;

    // Current clip reference
    tracktion::engine::WaveAudioClip* currentClip = nullptr;
    
    // Playhead
    std::unique_ptr<juce::DrawableRectangle> playhead;

    // Global zoom state
    ZoomState zoomState;

    // Layout management
    juce::StretchableLayoutManager layoutManager;
    std::vector<int> itemComponents;  // Indices of components in the layout
    
    // Constants for layout
    static constexpr int controlBarHeight = 26;
    static constexpr int crossfaderHeight = 25;
    static constexpr int thumbnailHeight = 60;
    static constexpr int minPluginHeight = 30;  // Minimum height when collapsed

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TransportComponent)
}; 
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

class TransportComponent : public juce::Component,
                         public juce::Timer,
                         public juce::ChangeListener,
                         public tracktion::engine::AutomationRecordManager::Listener
{
public:
    TransportComponent(tracktion::engine::Edit& e);
    ~TransportComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void changeListenerCallback(juce::ChangeBroadcaster*) override;
    void timerCallback() override;
    void mouseDown(juce::MouseEvent const& event) override;
    void mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) override;
    
    void updateThumbnail();

    void deleteSelectedChopRegion();

    void updatePlayheadPosition();
    void setZoomLevel(double newLevel);
    void setScrollPosition(double newPosition);
    double getMaxScrollPosition() const;

private:
    tracktion::engine::Edit& edit;
    tracktion::engine::TransportControl& transport;
    
    // Transport bar
    TransportBar transportBar;
    
    // Waveform thumbnail
    tracktion::engine::SmartThumbnail thumbnail;
    
    std::unique_ptr<CrossfaderAutomationLane> crossfaderAutomationLane;
    std::unique_ptr<AutomationLane> reverbWetAutomationLane;
    
    // Plugin automation components
    std::unique_ptr<PluginAutomationComponent> reverbAutomationComponent;
    std::unique_ptr<PluginAutomationComponent> delayAutomationComponent;
    std::unique_ptr<PluginAutomationComponent> phaserAutomationComponent;
    std::unique_ptr<PluginAutomationComponent> flangerAutomationComponent;
    
    juce::Viewport pluginAutomationViewport;
    juce::Component pluginAutomationContainer;
    
    // Zoom and scroll state
    double zoomLevel = 1.0;
    double scrollPosition = 0.0; // 0.0 to 1.0
    static constexpr double minZoom = 1.0; // Changed from 0.1 to 1.0 (100%)
    static constexpr double maxZoom = 20.0;

    // Current clip reference
    tracktion::engine::WaveAudioClip* currentClip = nullptr;
    
    // Playhead
    std::unique_ptr<juce::DrawableRectangle> playhead;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TransportComponent)
}; 
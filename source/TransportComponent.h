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
    void automationModeChanged() override;

    void deleteSelectedChopRegion();

    void updateTimeDisplay();
    void updatePlayheadPosition();
    void updateTransportState();
    void setZoomLevel(double newLevel);
    void setScrollPosition(double newPosition);
    double getMaxScrollPosition() const;

private:
    tracktion::engine::Edit& edit;
    tracktion::engine::TransportControl& transport;
    
    // Transport controls
    juce::ShapeButton playButton{"Play", juce::Colours::white, juce::Colours::lightgrey, juce::Colours::grey};
    juce::ShapeButton stopButton{"Stop", juce::Colours::white, juce::Colours::lightgrey, juce::Colours::grey};
    juce::TextButton loopButton{"Loop"};
    juce::ShapeButton automationReadButton{"Auto Read", juce::Colours::white, juce::Colours::green.darker(), juce::Colours::green};
    juce::ShapeButton automationWriteButton{"Auto Write", juce::Colours::white, juce::Colours::red, juce::Colours::red};
    
    // Zoom controls
    juce::TextButton zoomInButton{"+"};
    juce::TextButton zoomOutButton{"-"};
    
    // Grid controls
    juce::ComboBox gridSizeComboBox;
    
    // Timeline and position display
    juce::Label timeDisplay;
    std::unique_ptr<juce::DrawableRectangle> playhead;
    
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
    
    // Icon path functions
    static juce::Path getPlayPath()
    {
        juce::Path path;
        path.addTriangle(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.5f);
        return path;
    }

    static juce::Path getPausePath()
    {
        juce::Path path;
        path.addRectangle(0.0f, 0.0f, 0.3f, 1.0f);
        path.addRectangle(0.7f, 0.0f, 0.3f, 1.0f);
        return path;
    }

    static juce::Path getStopPath()
    {
        juce::Path path;
        path.addRectangle(0.0f, 0.0f, 1.0f, 1.0f);
        return path;
    }

    static juce::Path getRecordPath()
    {
        juce::Path recordPath;
        recordPath.addEllipse(0.0f, 0.0f, 100.0f, 100.0f);
        return recordPath;
    }

    static juce::Path getAutomationPath()
    {
        juce::Path path;
        path.startNewSubPath(0.0f, 50.0f);
        
        // Create a smooth sine wave
        for (float x = 0; x <= 100.0f; x += 1.0f)
        {
            float y = 50.0f + 30.0f * std::sin(x * 0.12f);
            path.lineTo(x, y);
        }
        
        return path;
    }
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TransportComponent)
}; 
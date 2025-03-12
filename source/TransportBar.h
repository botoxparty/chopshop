#pragma once

#include <juce_core/juce_core.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <tracktion_engine/tracktion_engine.h>

#include "CustomLookAndFeel.h"
#include "ZoomState.h"

class TransportBar : public juce::Component,
                    public juce::Timer,
                    public juce::ChangeListener,
                    public ZoomStateListener
{
public:
    // Function pointer type for snap callback
    using SnapCallback = std::function<void(bool)>;
    
    TransportBar(tracktion::engine::Edit& e, ZoomState& zs);
    ~TransportBar() override;

    void resized() override;
    void changeListenerCallback(juce::ChangeBroadcaster*) override;
    void timerCallback() override;
    void gridSizeChanged(float newGridSize) override;

    void updateTimeDisplay();
    void updateTransportState();
    
    void setSnapCallback(SnapCallback callback) { onSnapStateChanged = callback; }

private:
    tracktion::engine::Edit& edit;
    tracktion::engine::TransportControl& transport;
    ZoomState& zoomState;
    
    // Transport controls
    juce::ShapeButton playButton{"Play", juce::Colours::white, juce::Colours::lightgrey, juce::Colours::grey};
    juce::ShapeButton stopButton{"Stop", juce::Colours::white, juce::Colours::lightgrey, juce::Colours::grey};
    juce::TextButton loopButton{"Loop"};
    juce::ShapeButton automationReadButton{"Auto Read", juce::Colours::white, juce::Colours::green, juce::Colours::green};
    juce::ShapeButton automationWriteButton{"Auto Write", juce::Colours::white, juce::Colours::red, juce::Colours::red};
    
    // Zoom controls
    juce::TextButton zoomInButton{"+"};
    juce::TextButton zoomOutButton{"-"};
    
    // Snap control
    juce::TextButton snapButton{"SNAP TO GRID"};
    SnapCallback onSnapStateChanged;
    
    // Grid controls
    juce::ComboBox gridSizeComboBox;
    
    // Timeline and position display
    juce::Label timeDisplay;

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

    static juce::Path getAutomationPath()
    {
        juce::Path path;
        path.startNewSubPath(0.0f, 0.5f);
        for (float x = 0.0f; x <= 1.0f; x += 0.01f)
            path.lineTo(x, 0.5f + 0.4f * std::sin(x * juce::MathConstants<float>::twoPi));
        return path;
    }

    static juce::Path getRecordPath()
    {
        juce::Path path;
        path.addEllipse(0.0f, 0.0f, 1.0f, 1.0f);
        return path;
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TransportBar)
}; 
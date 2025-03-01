#pragma once

#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <tracktion_engine/tracktion_engine.h>

//==============================================================================
/**
 * A modern audio visualization component that displays waveforms with playback position
 * and allows for interactive seeking.
 */
class Thumbnail : public juce::Component,
                  private juce::Timer
{
public:
    //==============================================================================
    Thumbnail(tracktion::engine::TransportControl& transportControl);
    ~Thumbnail() override;

    //==============================================================================
    void paint(juce::Graphics& g) override;
    void resized() override;
    
    //==============================================================================
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    
    //==============================================================================
    /** Start the thumbnail's timer to update the cursor position */
    void start();
    
    /** Set the audio file to display */
    void setFile(const tracktion::engine::AudioFile& file);
    
    /** Set the playback speed ratio (for time-stretching visualization) */
    void setSpeedRatio(double ratio);
    
    /** Set beat quantization for playback jumps */
    void setQuantisation(std::optional<int> numBars);
    
    /** Set the waveform color */
    void setWaveformColor(juce::Colour color);
    
    /** Set the cursor color */
    void setCursorColor(juce::Colour color);
    
    /** Set the background color */
    void setBackgroundColor(juce::Colour color);

private:
    //==============================================================================
    tracktion::engine::TransportControl& transport;
    tracktion::engine::SmartThumbnail smartThumbnail;
    
    // Visual elements
    juce::DrawableRectangle cursor;
    juce::DrawableRectangle pendingCursor;
    
    // Timer for cursor updates
    void timerCallback() override;
    
    // Playback control
    void updateCursorPosition();
    std::optional<tracktion::TimePosition> positionToJumpAt;
    std::optional<int> quantisationNumBars;
    
    // Visual settings
    double currentSpeedRatio = 1.0;
    juce::Colour waveformColor = juce::Colours::white;
    juce::Colour cursorColor = juce::Colours::red;
    juce::Colour backgroundColor = juce::Colours::black.withAlpha(0.7f);
    
    // Draw time markers on the waveform
    void drawTimeMarkers(juce::Graphics& g, juce::Rectangle<int> bounds);
    
    // Helper methods for quantization
    static tracktion::TimePosition roundToNearest(tracktion::TimePosition pos, const tracktion::engine::TempoSequence& ts, int quantisationNumBars);
    static tracktion::TimePosition roundUp(tracktion::TimePosition pos, const tracktion::engine::TempoSequence& ts, int quantisationNumBars);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Thumbnail)
};

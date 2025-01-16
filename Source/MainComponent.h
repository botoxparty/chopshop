#pragma once

#include <JuceHeader.h>

//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/
class MainComponent  : public juce::Component
{
public:
    //==============================================================================
    MainComponent();
    ~MainComponent() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

    void play();
    void stop();
    void loadAudioFile();
    void updateTempo();
    tracktion_engine::WaveAudioClip::Ptr getClip(int trackIndex);

private:
    //==============================================================================
    
    tracktion_engine::Engine engine { ProjectInfo::projectName };
    tracktion_engine::Edit edit { engine, tracktion_engine::Edit::forEditing };
    juce::Slider tempoSlider;

    double baseTempo = 120.0;

    enum class PlayState {
        Stopped,
        Playing,
        Paused
    };

    PlayState playState { PlayState::Stopped };

    juce::TextButton openButton { "Open" };
    juce::TextButton saveButton { "Save" };
    juce::TextButton playButton { "Play" };
    juce::TextButton stopButton { "Stop" };

    void handleFileSelection(const juce::File& file);

    juce::Slider crossfaderSlider;
    void updateCrossfader();
    void setTrackVolume(int trackIndex, float volume);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};

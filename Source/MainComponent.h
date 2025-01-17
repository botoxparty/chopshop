#pragma once

#include <JuceHeader.h>
#include "Utilities.h"

// Add this line to enable console output
#define JUCE_DEBUG 1


class DistortionEffect
{
public:
    DistortionEffect() : drive(1.0f) {}
    
    void setDrive(float newDrive) { drive = newDrive; }
    
    void process(float* buffer, int numSamples)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            float input = buffer[i];
            
            // Amplify the input signal by the drive amount
            input *= drive;
            
            // Clip the signal to prevent excessive values
            if (input > 1.0f) input = 1.0f;
            if (input < -1.0f) input = -1.0f;
            
            // Apply waveshaping using tanh function for smooth distortion
            buffer[i] = std::tanh(input);
        }
    }

private:
    float drive;
};

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
    DistortionEffect distortion;
    juce::Slider distortionDriveSlider;
    float distortionDrive = 1.0f;
    tracktion_engine::Engine engine { ProjectInfo::projectName };
    tracktion_engine::Edit edit { engine, tracktion_engine::Edit::forEditing };
    juce::Slider tempoSlider;

    double baseTempo = 120.0;
    double trackOffset = 0.0;

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
    juce::TextButton chopButton { "Chop" };

    void handleFileSelection(const juce::File& file);

    juce::Slider crossfaderSlider;
    void updateCrossfader();
    void setTrackVolume(int trackIndex, float volume);
    void updateDistortion();

    std::unique_ptr<Thumbnail> thumbnail;

    te::Plugin::Ptr distortionPlugin;

    juce::Slider reverbRoomSizeSlider;
    juce::Slider reverbWetSlider;
    te::Plugin::Ptr reverbPlugin;

    juce::TextButton recordButton { "Record" };

    void armTrack(int trackIndex, bool arm);
    void startRecording();
    void stopRecording();

    juce::Label trackOffsetLabel;

    void updateTrackOffsetLabel(double offset);

    juce::Label tempoLabel { "Tempo Label", "BPM" };

    juce::TextButton tempo70Button  { "70%" };
    juce::TextButton tempo75Button  { "75%" };
    juce::TextButton tempo80Button  { "80%" };
    juce::TextButton tempo85Button  { "85%" };
    juce::TextButton tempo100Button { "100%" };

    void setTempoPercentage(double percentage);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};

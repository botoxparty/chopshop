/*
    ,--.                     ,--.     ,--.  ,--.
  ,-'  '-.,--.--.,--,--.,---.|  |,-.,-'  '-.`--' ,---. ,--,--,      Copyright 2018
  '-.  .-'|  .--' ,-.  | .--'|     /'-.  .-',--.| .-. ||      \   Tracktion Software
    |  |  |  |  \ '-'  \ `--.|  \  \  |  |  |  |' '-' '|  ||  |       Corporation
    `---' `--'   `--`--'`---'`--'`--' `---' `--' `---' `--''--'    www.tracktion.com
*/

#pragma once

namespace te = tracktion;
using namespace std::literals;
using namespace juce;

//==============================================================================
namespace Helpers
{
    static inline void addAndMakeVisible (Component& parent, const Array<Component*>& children)
    {
        for (auto c : children)
            parent.addAndMakeVisible (c);
    }

    static inline String getStringOrDefault (const String& stringToTest, const String& stringToReturnIfEmpty)
    {
        return stringToTest.isEmpty() ? stringToReturnIfEmpty : stringToTest;
    }

    static inline File findRecentEdit (const File& dir)
    {
        auto files = dir.findChildFiles (File::findFiles, false, "*.tracktionedit");

        if (files.size() > 0)
        {
            files.sort();
            return files.getLast();
        }

        return {};
    }
}

//==============================================================================
namespace PlayHeadHelpers
{
    // Quick-and-dirty function to format a timecode string
    static inline String timeToTimecodeString (double seconds)
    {
        auto millisecs = roundToInt (seconds * 1000.0);
        auto absMillisecs = std::abs (millisecs);

        return String::formatted ("%02d:%02d:%02d.%03d",
                                  millisecs / 3600000,
                                  (absMillisecs / 60000) % 60,
                                  (absMillisecs / 1000)  % 60,
                                  absMillisecs % 1000);
    }

    // Quick-and-dirty function to format a bars/beats string
    static inline String quarterNotePositionToBarsBeatsString (double quarterNotes, int numerator, int denominator)
    {
        if (numerator == 0 || denominator == 0)
            return "1|1|000";

        auto quarterNotesPerBar = ((double) numerator * 4.0 / (double) denominator);
        auto beats  = (fmod (quarterNotes, quarterNotesPerBar) / quarterNotesPerBar) * numerator;

        auto bar    = ((int) quarterNotes) / quarterNotesPerBar + 1;
        auto beat   = ((int) beats) + 1;
        auto ticks  = ((int) (fmod (beats, 1.0) * 960.0 + 0.5));

        return String::formatted ("%d|%d|%03d", bar, beat, ticks);
    }

    // Returns a textual description of a CurrentPositionInfo
    static inline String getTimecodeDisplay (const AudioPlayHead::CurrentPositionInfo& pos)
    {
        MemoryOutputStream displayText;

        displayText << String (pos.bpm, 2) << " bpm, "
                    << pos.timeSigNumerator << '/' << pos.timeSigDenominator
                    << "  -  " << timeToTimecodeString (pos.timeInSeconds)
                    << "  -  " << quarterNotePositionToBarsBeatsString (pos.ppqPosition,
                                                                        pos.timeSigNumerator,
                                                                        pos.timeSigDenominator);

        if (pos.isRecording)
            displayText << "  (recording)";
        else if (pos.isPlaying)
            displayText << "  (playing)";
        else
            displayText << "  (stopped)";

        return displayText.toString();
    }
}

//==============================================================================
namespace EngineHelpers
{
    inline te::Project::Ptr createTempProject (te::Engine& engine)
    {
        auto file = engine.getTemporaryFileManager().getTempDirectory().getChildFile ("temp_project").withFileExtension (te::projectFileSuffix);
        te::ProjectManager::TempProject tempProject (engine.getProjectManager(), file, true);
        return tempProject.project;
    }

    inline void showAudioDeviceSettings (te::Engine& engine)
    {
        DialogWindow::LaunchOptions o;
        o.dialogTitle = TRANS("Audio Settings");
        o.dialogBackgroundColour = LookAndFeel::getDefaultLookAndFeel().findColour (ResizableWindow::backgroundColourId);
        o.content.setOwned (new AudioDeviceSelectorComponent (engine.getDeviceManager().deviceManager,
                                                              0, 1024, 1, 1024, false, false, true, true));
        o.content->setSize (400, 600);
        o.launchAsync();
    }

    inline void browseForAudioFile (te::Engine& engine, std::function<void (const File&)> fileChosenCallback)
    {
        auto fc = std::make_shared<FileChooser> ("Please select an audio file to load...",
                                                 engine.getPropertyStorage().getDefaultLoadSaveDirectory ("pitchAndTimeExample"),
                                                 engine.getAudioFileFormatManager().readFormatManager.getWildcardForAllFormats());

        fc->launchAsync (FileBrowserComponent::openMode + FileBrowserComponent::canSelectFiles,
                         [fc, &engine, callback = std::move (fileChosenCallback)] (const FileChooser&)
                         {
                             const auto f = fc->getResult();

                             if (f.existsAsFile())
                                 engine.getPropertyStorage().setDefaultLoadSaveDirectory ("pitchAndTimeExample", f.getParentDirectory());

                             callback (f);
                         });
    }

    inline void removeAllClips (te::AudioTrack& track)
    {
        auto clips = track.getClips();

        for (int i = clips.size(); --i >= 0;)
            clips.getUnchecked (i)->removeFromParent();
    }

    inline te::AudioTrack* getAudioTrack (te::Edit& edit, int index)
    {
        edit.ensureNumberOfAudioTracks (index + 1);
        return te::getAudioTracks (edit)[index];
    }

    enum class ReturnToStart { no, yes };

    inline te::Plugin::Ptr getPlugin (te::AudioTrack& track, const String& pluginName)
    {
        for (auto plugin : track.pluginList)
            if (plugin->getPluginType() == pluginName)
                return plugin;
        return {};
    }

    inline te::Plugin::Ptr getPluginFromMasterTrack (te::Edit& edit, const String& pluginName)
    {
        for (auto plugin : edit.getMasterPluginList().getPlugins())
            if (plugin->getPluginType() == pluginName)
                return plugin;
        return {};
    }

    inline tracktion::engine::Plugin::Ptr getPluginFromRack(tracktion::engine::Edit& edit, const juce::String& pluginName)
    {
        if (auto masterTrack = edit.getMasterTrack())
        {
            tracktion::engine::RackInstance::Ptr rackInstance = masterTrack->pluginList.getPluginsOfType<tracktion::engine::RackInstance>()[0];
            tracktion::engine::RackType::Ptr rackType = rackInstance->type;
            juce::Array<tracktion::engine::Plugin*> plugins = rackType->getPlugins();
            for (auto& plugin : plugins)
            {
                DBG("Plugin: " + plugin->getPluginType());
                DBG("Plugin NAME LOOKING FOR: " + pluginName);
                if (plugin->getPluginType() == pluginName)
                {
                    DBG("Loaded plugin: " + plugin->getPluginType());
                    return plugin;
                }
            }
        }

        DBG("Error: No plugin found in rack for " + pluginName);
        return nullptr;
    }

    inline tracktion::engine::Clip* getCurrentClip(tracktion::engine::Edit& edit)
    {
        auto track = EngineHelpers::getAudioTrack(edit, 0);
        return track->getClips()[0];
    }

    inline void togglePlay (te::Edit& edit, ReturnToStart rts = ReturnToStart::no)
    {
        auto& transport = edit.getTransport();

        if (transport.isPlaying())
            transport.stop (false, false);
        else
        {
            if (rts == ReturnToStart::yes)
                transport.playFromStart (true);
            else
                transport.play (false);
        }
    }

    inline void toggleRecord (te::Edit& edit)
    {
        auto& transport = edit.getTransport();

        if (transport.isRecording())
            transport.stop (true, false);
        else
            transport.record (false);
    }

    inline void armTrack (te::AudioTrack& t, bool arm, int position = 0)
    {
        auto& edit = t.edit;
        for (auto instance : edit.getAllInputDevices())
            if (te::isOnTargetTrack (*instance, t, position))
                instance->setRecordingEnabled (t.itemID, arm);
    }

    inline bool isTrackArmed (te::AudioTrack& t, int position = 0)
    {
        auto& edit = t.edit;
        for (auto instance : edit.getAllInputDevices())
            if (te::isOnTargetTrack (*instance, t, position))
                return instance->isRecordingEnabled (t.itemID);

        return false;
    }

    inline bool isInputMonitoringEnabled (te::AudioTrack& t, int position = 0)
    {
        for (auto instance : t.edit.getAllInputDevices())
            if (te::isOnTargetTrack (*instance, t, position))
                return instance->isLivePlayEnabled (t);

        return false;
    }

    inline void enableInputMonitoring (te::AudioTrack& t, bool im, int position = 0)
    {
        if (isInputMonitoringEnabled (t, position) != im)
        {
            for (auto instance : t.edit.getAllInputDevices())
            {
                if (te::isOnTargetTrack (*instance, t, position))
                {
                    if (auto mode = instance->getInputDevice().getMonitorMode();
                        mode == te::InputDevice::MonitorMode::on ||  mode == te::InputDevice::MonitorMode::off)
                    {
                        instance->getInputDevice().setMonitorMode (mode == te::InputDevice::MonitorMode::on
                                                                    ? te::InputDevice::MonitorMode::off
                                                                    : te::InputDevice::MonitorMode::on);
                    }
                }
            }
        }
    }

    inline bool trackHasInput (te::AudioTrack& t, int position = 0)
    {
        auto& edit = t.edit;
        for (auto instance : edit.getAllInputDevices())
            if (te::isOnTargetTrack (*instance, t, position))
                return true;

        return false;
    }

    inline std::unique_ptr<juce::KnownPluginList::PluginTree> createPluginTree (te::Engine& engine)
    {
        auto& list = engine.getPluginManager().knownPluginList;

        if (auto tree = list.createTree (list.getTypes(), KnownPluginList::sortByManufacturer))
            return tree;

        return {};
    }

    inline te::Plugin::Ptr createPlugin(te::Edit& edit, const juce::String& xmlType)
    {
        auto plugin = edit.getPluginCache().createNewPlugin(xmlType, {});
        return plugin;
    }

    inline te::AudioTrack::Ptr getChopTrack(te::Edit& edit)
    {
        for (auto track : edit.getTrackList())
        {
            if (auto audioTrack = dynamic_cast<te::AudioTrack*>(track))
            {
                if (audioTrack->getName() == "Chop Track")
                    return audioTrack;
            }
        }
        return nullptr;
    }
}

//==============================================================================
class FlaggedAsyncUpdater : public AsyncUpdater
{
public:
    //==============================================================================
    void markAndUpdate (bool& flag)     { flag = true; triggerAsyncUpdate(); }

    bool compareAndReset (bool& flag) noexcept
    {
        if (! flag)
            return false;

        flag = false;
        return true;
    }
};


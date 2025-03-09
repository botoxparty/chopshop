/*
  ==============================================================================

    LibraryComponent.cpp
    Created: 17 Jan 2025 11:02:15pm
    Author:  Adam Hammad

  ==============================================================================
*/

#include "LibraryComponent.h"
#include "minibpm.h"

LibraryComponent::LibraryComponent (te::Engine& engineToUse)
    : engine (engineToUse)
{
    // Create or load the library project
    auto projectFile = juce::File::getSpecialLocation (juce::File::userMusicDirectory)
                           .getChildFile ("ChopShop")
                           .getChildFile ("Library.tracktion");

    // Create the directory if it doesn't exist
    auto projectDir = juce::File::getSpecialLocation (juce::File::userMusicDirectory).getChildFile ("ChopShop");
    bool dirCreated = projectDir.createDirectory();
    DBG ("Project directory creation result: " + juce::String (dirCreated ? "Success" : "Failed") + " Path: " + projectDir.getFullPathName());

    // Get or create the library project
    libraryProject = engine.getProjectManager().getProject (projectFile);
    if (libraryProject == nullptr || !libraryProject->isValid())
    {
        DBG ("Attempting to create new project at: " + projectFile.getFullPathName());
        libraryProject = engine.getProjectManager().createNewProject (projectFile);
        if (libraryProject != nullptr)
        {
            libraryProject->createNewProjectId();
            libraryProject->setName ("ChopShop Library");
            libraryProject->setDescription ("Created: " + juce::Time::getCurrentTime().toString (true, false));

            if (libraryProject->save())
            {
                DBG ("Created and saved new ChopShop Library project at: " + projectFile.getFullPathName());
            }
            else
            {
                DBG ("Failed to save project!");
            }
        }
        else
        {
            DBG ("Failed to create new project!");
        }
    }
    else
    {
        DBG ("Loaded existing ChopShop Library project from: " + projectFile.getFullPathName());
        DBG ("Project contains " + juce::String (libraryProject->getNumProjectItems()) + " items");

        // Log the items in the project
        for (int i = 0; i < libraryProject->getNumProjectItems(); ++i)
        {
            auto item = libraryProject->getProjectItemAt (i);
            if (item != nullptr)
            {
                DBG ("  Item " + juce::String (i) + ": " + item->getName() + " (BPM: " + juce::String (item->getNamedProperty ("bpm").getFloatValue(), 1) + ", File: " + item->getSourceFile().getFileName() + ")");
            }
        }
    }

    // Set up add file button
    addFileButton.setColour (juce::TextButton::buttonColourId, black);
    addFileButton.setColour (juce::TextButton::textColourOffId, matrixGreen);
    addFileButton.setColour (juce::TextButton::textColourOnId, matrixGreen);
    addAndMakeVisible (addFileButton);

    // Set up remove file button
    removeFileButton.setColour (juce::TextButton::buttonColourId, black);
    removeFileButton.setColour (juce::TextButton::textColourOffId, matrixGreen);
    removeFileButton.setColour (juce::TextButton::textColourOnId, matrixGreen);
    addAndMakeVisible (removeFileButton);

    // Set up edit BPM button
    editBpmButton.setButtonText ("Edit BPM");
    editBpmButton.setColour (juce::TextButton::buttonColourId, black);
    editBpmButton.setColour (juce::TextButton::textColourOffId, matrixGreen);
    editBpmButton.setColour (juce::TextButton::textColourOnId, matrixGreen);
    editBpmButton.onClick = [this]() {
        auto selectedRow = playlistTable->getSelectedRow();
        if (selectedRow >= 0)
        {
            showBpmEditorWindow (selectedRow);
        }
    };
    addAndMakeVisible (editBpmButton);

    // Set up playlist table
    playlistTable = std::make_unique<juce::TableListBox>();
    playlistTable->setModel (this);
    playlistTable->getHeader().addColumn ("Name", 1, 300);
    playlistTable->getHeader().addColumn ("BPM", 2, 100);
    playlistTable->getHeader().setStretchToFitActive (true);
    playlistTable->setColour (juce::ListBox::backgroundColourId, black);
    playlistTable->setColour (juce::ListBox::outlineColourId, matrixGreen.withAlpha (0.5f));
    playlistTable->setColour (juce::ListBox::textColourId, matrixGreen);
    addAndMakeVisible (playlistTable.get());

    // Enable sorting
    playlistTable->getHeader().setSortColumnId (1, true); // Default sort by name

    // Set up button callbacks
    addFileButton.onClick = [this]() {
        fileChooser = std::make_shared<juce::FileChooser> (
            "Select Audio Files",
            juce::File::getSpecialLocation (juce::File::userMusicDirectory),
            "*.wav;*.mp3;*.aif;*.aiff");

        fileChooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles | juce::FileBrowserComponent::canSelectMultipleItems,
            [this] (const juce::FileChooser& fc) {
                auto results = fc.getResults();
                for (const auto& file : results)
                {
                    if (file.exists())
                    {
                        addToLibrary (file);
                    }
                }
            });
    };

    removeFileButton.onClick = [this]() {
        auto selectedRow = playlistTable->getSelectedRow();
        if (selectedRow >= 0)
        {
            removeFromLibrary (selectedRow);
        }
    };

    // Load existing library
    loadLibrary();
}

LibraryComponent::~LibraryComponent()
{
    // No need to explicitly save as the Project class handles this
}

void LibraryComponent::paint (juce::Graphics& g)
{
    g.fillAll (black);
    g.setColour (matrixGreen.withAlpha (0.5f));
    g.drawRect (getLocalBounds(), 1);
}

void LibraryComponent::resized()
{
    auto bounds = getLocalBounds();
    auto buttonHeight = 30;

    // Playlist table takes all space except bottom button area
    auto buttonArea = bounds.removeFromBottom (buttonHeight);
    playlistTable->setBounds (bounds.reduced (2));

    // Add buttons at the bottom
    addFileButton.setBounds (buttonArea.removeFromLeft (100).reduced (2));
    removeFileButton.setBounds (buttonArea.removeFromLeft (100).reduced (2));
    editBpmButton.setBounds (buttonArea.removeFromLeft (100).reduced (2));
}

// TableListBoxModel implementations
int LibraryComponent::getNumRows()
{
    return libraryProject ? libraryProject->getNumProjectItems() : 0;
}

void LibraryComponent::paintRowBackground (juce::Graphics& g, int rowNumber, int width, int height, bool rowIsSelected)
{
    if (rowIsSelected)
        g.fillAll (matrixGreen.withAlpha (0.3f));
}

void LibraryComponent::paintCell (juce::Graphics& g, int rowNumber, int columnId, int width, int height, bool rowIsSelected)
{
    if (!libraryProject || rowNumber >= libraryProject->getNumProjectItems())
        return;

    auto projectItem = libraryProject->getProjectItemAt (rowNumber);
    if (projectItem == nullptr)
        return;

    g.setColour (matrixGreen);

    if (columnId == 1) // Name column
        g.drawText (projectItem->getName(), 2, 0, width - 4, height, juce::Justification::centredLeft);
    else if (columnId == 2) // BPM column
        g.drawText (juce::String (projectItem->getNamedProperty ("bpm").getFloatValue(), 1), 2, 0, width - 4, height, juce::Justification::centred);
}

void LibraryComponent::cellDoubleClicked (int rowNumber, int columnId, const juce::MouseEvent&)
{
    if (!libraryProject || rowNumber >= libraryProject->getNumProjectItems())
        return;

    auto projectItem = libraryProject->getProjectItemAt (rowNumber);
    if (projectItem != nullptr && onEditSelected)
    {
        if (auto edit = loadEditFromProjectItem (projectItem))
            onEditSelected (std::move (edit));
    }
}

void LibraryComponent::cellClicked (int rowNumber, int columnId, const juce::MouseEvent& event)
{
    if (event.mods.isRightButtonDown() && libraryProject && rowNumber < libraryProject->getNumProjectItems())
    {
        auto projectItem = libraryProject->getProjectItemAt (rowNumber);
        if (projectItem == nullptr)
            return;

        juce::PopupMenu menu;
        menu.addItem (1, "Show in Finder");
        menu.addItem (2, "Remove");

        menu.showMenuAsync (juce::PopupMenu::Options(), [this, rowNumber, projectItem] (int result) {
            if (result == 1) // Show in Finder
            {
                juce::File file (projectItem->getSourceFile());
                if (file.exists())
                    file.revealToUser();
            }
            else if (result == 2) // Remove
            {
                removeFromLibrary (rowNumber);
            }
        });
    }
}

void LibraryComponent::sortOrderChanged (int newSortColumnId, bool isForwards)
{
    if (newSortColumnId != sortedColumnId || isForwards != sortedForward)
    {
        sortedColumnId = newSortColumnId;
        sortedForward = isForwards;

        // We can't directly sort the project items, so we'll need to reload the table
        // after sorting is changed
        playlistTable->updateContent();
    }
}

void LibraryComponent::createPluginRack(std::unique_ptr<tracktion::engine::Edit>& edit)
{
    if (auto masterTrack = edit->getMasterTrack())
    {
        tracktion::engine::Plugin::Array plugins;

        auto reverbPlugin = EngineHelpers::createPlugin(*edit, tracktion::engine::ReverbPlugin::xmlTypeName);
        reverbPlugin->remapOnTempoChange.setValue(true, nullptr);
        plugins.add (reverbPlugin);

        auto delayPlugin = EngineHelpers::createPlugin(*edit, AutoDelayPlugin::xmlTypeName);
        delayPlugin->remapOnTempoChange.setValue(true, nullptr);
        plugins.add (delayPlugin);

        auto flangerPlugin = EngineHelpers::createPlugin(*edit, FlangerPlugin::xmlTypeName);
        flangerPlugin->remapOnTempoChange.setValue(true, nullptr);
        plugins.add (flangerPlugin);

        auto phaserPlugin = EngineHelpers::createPlugin(*edit, AutoPhaserPlugin::xmlTypeName);
        phaserPlugin->remapOnTempoChange.setValue(true, nullptr);
        plugins.add (phaserPlugin);

        // Create the rack type with proper channel connections
        if (auto rack = tracktion::engine::RackType::createTypeToWrapPlugins (plugins, *edit))
        {
            masterTrack->pluginList.insertPlugin (tracktion::engine::RackInstance::create (*rack), 0);
        }
    }
}

void LibraryComponent::addToLibrary (const juce::File& file)
{
    if (!libraryProject)
    {
        DBG ("Error: No library project available");
        return;
    }

    if (!file.existsAsFile())
    {
        DBG ("Error: File does not exist: " + file.getFullPathName());
        return;
    }

    // Calculate BPM first
    float detectedBPM = 120.0f; // Default BPM

    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();

    std::unique_ptr<juce::AudioFormatReader> reader (formatManager.createReaderFor (file));
    if (reader)
    {
        // Create MiniBPM detector
        breakfastquay::MiniBPM bpmDetector (reader->sampleRate);
        bpmDetector.setBPMRange (60, 180); // typical range for music

        // Process audio in chunks
        const int blockSize = 1024;
        juce::AudioBuffer<float> buffer (1, blockSize);
        std::vector<float> samples (blockSize);

        for (int pos = 0; pos < reader->lengthInSamples; pos += blockSize)
        {
            const int numSamples = std::min (blockSize,
                static_cast<int> (reader->lengthInSamples - pos));

            reader->read (&buffer, 0, numSamples, pos, true, false);
            memcpy (samples.data(), buffer.getReadPointer (0), numSamples * sizeof (float));

            bpmDetector.process (samples.data(), numSamples);
        }

        float tempBPM = bpmDetector.estimateTempo();
        if (tempBPM > 0)
            detectedBPM = tempBPM;
    }

    // Calculate beat duration in seconds
    double beatDuration = 60.0 / detectedBPM;

    // Create a new Edit for this file
    auto options = te::Edit::Options {engine};
    options.editState = te::createEmptyEdit (engine);
    options.editProjectItemID = te::ProjectItemID::fromProperty (options.editState, te::IDs::projectID);
    options.numUndoLevelsToStore = 0;
    options.numAudioTracks = 2;
    options.role = te::Edit::forEditing;

    auto edit = te::Edit::createEdit (std::move (options));
    if (!edit)
    {
        DBG ("Error: Failed to create edit");
        return;
    }

    // add chop plugin to master plugin list
    // Should use PluginCache::createNewPlugin to create the ones you add here

    auto chopPlugin = edit->getPluginCache().createNewPlugin(ChopPlugin::xmlTypeName, juce::PluginDescription());
    if (chopPlugin != nullptr)
    {
        chopPlugin->remapOnTempoChange.setValue(true, nullptr);
        
        auto crossfaderParameter = chopPlugin->getAutomatableParameterByID("crossfader");
        if (crossfaderParameter != nullptr)
        {
            crossfaderParameter->getCurve().addPoint(tracktion::TimePosition::fromSeconds(0.0), 0.0f, 0.0f);
        }

        edit->getMasterPluginList().insertPlugin(chopPlugin, -1, nullptr);
    } else {
        DBG ("Error: Failed to create chop plugin");
        return;
    }

    createPluginRack(edit);

    // Create two tracks and import the audio file to both
    bool clipsCreated = false;
    for (int trackIndex = 0; trackIndex < 2; trackIndex++)
    {
        if (auto track = EngineHelpers::getAudioTrack (*edit, trackIndex))
        {
            DBG ("Setup track " + juce::String (trackIndex + 1));
            // Get the audio file length
            te::AudioFile audioFile (engine, file);
            if (!audioFile.isValid())
            {
                DBG ("Error: Invalid audio file");
                continue;
            }

            auto fileLength = audioFile.getLength();
            DBG ("Audio file length: " + juce::String (fileLength) + " seconds");

            // Create clip position
            auto timeRange = tracktion::TimeRange (tracktion::TimePosition(), tracktion::TimePosition::fromSeconds (fileLength));
            auto position = tracktion::engine::createClipPosition (edit->tempoSequence, timeRange, tracktion::TimeDuration::fromSeconds (trackIndex == 0 ? 0.0 : beatDuration));

            DBG ("Clip position: " + juce::String (position.time.getStart().inSeconds()) + " to " + juce::String (position.time.getEnd().inSeconds()));
            DBG ("Clip offset: " + juce::String (position.offset.inSeconds()));
            DBG ("Clip LENGTH: " + juce::String (position.time.getLength().inSeconds()));

            // Create the clip
            auto clip = track->insertWaveClip (file.getFileNameWithoutExtension(),
                file,
                position,
                true);

            if (!clip)
            {
                DBG ("Failed to create clip for track " + juce::String (trackIndex + 1));
                return;
            }

            DBG ("Created clip for track " + juce::String (trackIndex + 1) + " with BPM: " + juce::String (detectedBPM) + " and beat duration: " + juce::String (beatDuration) + " at position: " + juce::String (position.time.getStart().inSeconds()) + " to " + juce::String (position.time.getEnd().inSeconds()));

            clip->setSyncType (te::Clip::syncBarsBeats);
            clip->setAutoPitch (false);
            clip->setTimeStretchMode (te::TimeStretcher::elastiquePro);
            clip->setUsesProxy (false);
            clip->setAutoTempo (true);
            clip->getLoopInfo().setBpm (detectedBPM, clip->getAudioFile().getInfo());

            // Flush clip state to ValueTree
            clip->flushStateToValueTree();
            clipsCreated = true;
        }
        else
        {
            DBG ("Failed to create track " + juce::String (trackIndex + 1));
        }
    }

    if (!clipsCreated)
    {
        DBG ("Error: No clips were created successfully");
        return;
    }

    // Store BPM in the Edit's ValueTree
    edit->state.setProperty ("bpm", detectedBPM, nullptr);

    DBG ("Edit created");

    // Create a file path for the edit in the library project directory
    auto editFileName = file.getFileNameWithoutExtension() + ".tracktionedit";
    auto editFile = libraryProject->getDefaultDirectory().getChildFile (editFileName);

    // Save the edit to disk
    if (!te::EditFileOperations (*edit).saveAs (editFile, true))
    {
        DBG ("Error: Failed to save edit file");
        return;
    }

    if (!editFile.exists())
    {
        throw std::runtime_error (("Failed to save edit file: " + editFile.getFullPathName()).toStdString());
    }

    DBG ("Edit saved to: " + editFile.getFullPathName());

    onEditSelected (std::move (edit));

    // Add the Edit file to the project
    auto projectItem = libraryProject->createNewItem (editFile,
        te::ProjectItem::editItemType(),
        file.getFileNameWithoutExtension(),
        {},
        te::ProjectItem::Category::edit,
        true);

    if (projectItem)
    {
        // Store BPM as a property on the project item too
        projectItem->setNamedProperty ("bpm", juce::String (detectedBPM));

        // Save the project to ensure the item is persisted
        if (libraryProject->save())
        {
            DBG ("Project saved successfully with new item");
            playlistTable->updateContent();
            playlistTable->repaint();
        }
        else
        {
            DBG ("Error: Failed to save project");
        }
    }
    else
    {
        DBG ("Error: Failed to create project item");
    }
}

void LibraryComponent::removeFromLibrary (int index)
{
    if (!libraryProject || index < 0 || index >= libraryProject->getNumProjectItems())
        return;

    auto projectItem = libraryProject->getProjectItemAt (index);
    auto projectItemID = libraryProject->getProjectItemID (index);

    if (projectItem != nullptr)
    {
        DBG ("Removing item from library: " + projectItem->getName() + " (ID: " + projectItemID.toString() + ")");
    }

    libraryProject->removeProjectItem (projectItemID, false); // false = don't delete source material
    libraryProject->save();
    playlistTable->updateContent();

    DBG ("Library now contains " + juce::String (libraryProject->getNumProjectItems()) + " items");
}

void LibraryComponent::loadLibrary()
{
    // The project is already loaded in the constructor
    playlistTable->updateContent();

    // Log the current state of the library
    if (libraryProject)
    {
        DBG ("Library loaded with " + juce::String (libraryProject->getNumProjectItems()) + " items");

        // Sort the items if needed
        if (sortedColumnId != 0)
        {
            DBG ("Items are sorted by " + juce::String (sortedColumnId == 1 ? "Name" : "BPM") + (sortedForward ? " (ascending)" : " (descending)"));
        }
    }
}

te::ProjectItem::Ptr LibraryComponent::getProjectItemForFile (const juce::File& file) const
{
    if (!libraryProject)
    {
        DBG ("getProjectItemForFile: No library project available");
        return nullptr;
    }

    auto projectItem = libraryProject->getProjectItemForFile (file);

    if (projectItem != nullptr)
    {
        DBG ("Found project item for file: " + file.getFileName() + " (BPM: " + juce::String (projectItem->getNamedProperty ("bpm").getFloatValue(), 1) + ", ID: " + projectItem->getID().toString() + ")");
    }
    else
    {
        DBG ("No project item found for file: " + file.getFileName());
    }

    return projectItem;
}

void LibraryComponent::showBpmEditorWindow (int rowIndex)
{
    DBG ("Opening BPM editor for row: " + juce::String (rowIndex));

    if (!libraryProject)
    {
        DBG ("ERROR: No library project available");
        return;
    }

    if (rowIndex < 0 || rowIndex >= libraryProject->getNumProjectItems())
    {
        DBG ("ERROR: Invalid row index: " + juce::String (rowIndex) + " (Project has " + juce::String (libraryProject->getNumProjectItems()) + " items)");
        return;
    }

    auto projectItem = libraryProject->getProjectItemAt (rowIndex);
    if (projectItem == nullptr)
    {
        DBG ("ERROR: Failed to get project item at index: " + juce::String (rowIndex));
        return;
    }

    DBG ("Editing BPM for item: " + projectItem->getName() + " (ID: " + projectItem->getID().toString() + ", File: " + projectItem->getSourceFile().getFileName() + ")");

    float currentBpm = projectItem->getNamedProperty ("bpm").getFloatValue();
    if (currentBpm <= 0)
    {
        currentBpm = 120.0f;
        DBG ("Invalid BPM value, using default: " + juce::String (currentBpm, 1));
    }
    else
    {
        DBG ("Current BPM: " + juce::String (currentBpm, 1));
    }

    juce::DialogWindow::LaunchOptions options;

    auto content = std::make_unique<juce::Component>();
    content->setSize (200, 150);

    auto editor = new juce::TextEditor();
    editor->setBounds (50, 20, 100, 24);
    editor->setText (juce::String (currentBpm, 1));
    editor->setInputRestrictions (6, "0123456789.");
    editor->setColour (juce::TextEditor::backgroundColourId, black);
    editor->setColour (juce::TextEditor::textColourId, matrixGreen);
    editor->setColour (juce::TextEditor::outlineColourId, matrixGreen.withAlpha (0.5f));
    content->addAndMakeVisible (editor);

    auto halfButton = new juce::TextButton ("1/2x");
    halfButton->setBounds (30, 60, 60, 24);
    halfButton->setColour (juce::TextButton::buttonColourId, black);
    halfButton->setColour (juce::TextButton::textColourOffId, matrixGreen);
    halfButton->setColour (juce::TextButton::textColourOnId, matrixGreen);
    halfButton->onClick = [editor]() {
        double currentValue = editor->getText().getDoubleValue();
        double newValue = currentValue * 0.5;
        editor->setText (juce::String (newValue, 1));
        DBG ("BPM halved: " + juce::String (currentValue, 1) + " -> " + juce::String (newValue, 1));
    };
    content->addAndMakeVisible (halfButton);

    auto doubleButton = new juce::TextButton ("2x");
    doubleButton->setBounds (110, 60, 60, 24);
    doubleButton->setColour (juce::TextButton::buttonColourId, black);
    doubleButton->setColour (juce::TextButton::textColourOffId, matrixGreen);
    doubleButton->setColour (juce::TextButton::textColourOnId, matrixGreen);
    doubleButton->onClick = [editor]() {
        double currentValue = editor->getText().getDoubleValue();
        double newValue = currentValue * 2.0;
        editor->setText (juce::String (newValue, 1));
        DBG ("BPM doubled: " + juce::String (currentValue, 1) + " -> " + juce::String (newValue, 1));
    };
    content->addAndMakeVisible (doubleButton);

    auto okButton = new juce::TextButton ("OK");
    okButton->setBounds (50, 100, 100, 24);
    okButton->setColour (juce::TextButton::buttonColourId, black);
    okButton->setColour (juce::TextButton::textColourOffId, matrixGreen);
    okButton->setColour (juce::TextButton::textColourOnId, matrixGreen);

    // Store a reference to the project item for the lambda
    te::ProjectItem::Ptr itemRef = projectItem;

    okButton->onClick = [this, itemRef, editor, currentBpm]() {
        float newBpm = editor->getText().getFloatValue();

        if (newBpm <= 0)
        {
            DBG ("ERROR: Invalid BPM value entered: " + editor->getText());
            return;
        }

        if (itemRef != nullptr)
        {
            DBG ("Updating BPM for item: " + itemRef->getName() + " from " + juce::String (currentBpm, 1) + " to " + juce::String (newBpm, 1));

            try
            {
                itemRef->setNamedProperty ("bpm", juce::String (newBpm));

                DBG ("Saving project after BPM update...");
                libraryProject->save();

                playlistTable->updateContent();
                DBG ("BPM updated successfully");
            } catch (const std::exception& e)
            {
                DBG ("EXCEPTION while updating BPM: " + juce::String (e.what()));
            } catch (...)
            {
                DBG ("UNKNOWN EXCEPTION while updating BPM");
            }
        }
        else
        {
            DBG ("ERROR: Project item reference is null");
        }

        if (auto* dw = juce::Component::getCurrentlyModalComponent())
            dw->exitModalState (0);
    };
    content->addAndMakeVisible (okButton);

    content->setColour (juce::ResizableWindow::backgroundColourId, black);

    options.content.setOwned (content.release());
    options.dialogTitle = "Edit BPM";
    options.dialogBackgroundColour = black;
    options.escapeKeyTriggersCloseButton = true;
    options.useNativeTitleBar = true;
    options.resizable = false;

    DBG ("Launching BPM editor dialog");
    options.launchAsync();
}

std::unique_ptr<tracktion::engine::Edit> LibraryComponent::loadEditFromProjectItem(tracktion::engine::ProjectItem::Ptr projectItem)
{
    DBG("=== Starting Edit Load Process ===");
    
    if (!projectItem)
    {
        DBG("ERROR: Project item is null");
        return nullptr;
    }

    auto sourceFile = projectItem->getSourceFile();
    DBG("Loading edit from: " + sourceFile.getFullPathName());
    DBG("Project Item ID: " + projectItem->getID().toString());

    if (!sourceFile.exists())
    {
        DBG("ERROR: Source file doesn't exist: " + sourceFile.getFullPathName());
        return nullptr;
    }

    // Load edit state
    DBG("Loading edit state from project manager...");
    auto editState = te::loadEditFromProjectManager(engine.getProjectManager(), projectItem->getID());
    
    if (!editState.isValid())
    {
        DBG("ERROR: Failed to load edit state");
        return nullptr;
    }
    DBG("Edit state loaded successfully");

    // Create edit options
    auto options = te::Edit::Options{engine};
    options.editState = editState;
    options.editProjectItemID = projectItem->getID();
    options.numUndoLevelsToStore = 0;
    options.role = te::Edit::forEditing;

    // Create edit
    DBG("Creating edit from state...");
    auto edit = te::Edit::createEdit(std::move(options));
    
    if (!edit)
    {
        DBG("ERROR: Failed to create edit");
        return nullptr;
    }
    DBG("Edit created successfully");

    // Debug edit contents
    DBG("=== Edit Contents ===");
    DBG("Edit ID: " + edit->getProjectItemID().toString());
    DBG("Edit Name: " + edit->getName());
    
    // Debug tracks
    auto audioTracks = te::getAudioTracks(*edit);
    DBG("Number of audio tracks: " + juce::String(audioTracks.size()));

    for (auto track : audioTracks)
    {
        DBG("--- Track: " + track->getName() + " (ID: " + track->state.getProperty("id").toString() + ") ---");
        DBG("  Track type: " + track->state.getType().toString());
        // DBG("  playSlotClips value: " + juce::String(track->playSlotClips.get()));
        
        // Debug clips
        auto clips = track->getClips();
        DBG("  Number of clips: " + juce::String(clips.size()));
        
        for (auto clip : clips)
        {
            DBG("    Clip: " + clip->getName());
            DBG("    Start: " + juce::String(clip->getPosition().getStart().inSeconds()));
            DBG("    Length: " + juce::String(clip->getPosition().getLength().inSeconds()));
            DBG("    Source file: " + clip->getSourceFileReference().getFile().getFullPathName());
        }

        // Debug clip slots
        auto& clipSlots = track->getClipSlotList();
        auto slots = clipSlots.getClipSlots();
        DBG("  Number of clip slots: " + juce::String(slots.size()));
        
        for (auto slot : slots)
        {
            if (auto slotClip = slot->getClip())
            {
                DBG("    Slot clip: " + slotClip->getName());
            }
        }
    }

    // Debug ValueTree structure
    DBG("=== Edit State Structure ===");
    std::function<void(const juce::ValueTree&, const juce::String&)> dumpTree = 
        [&](const juce::ValueTree& tree, const juce::String& indent)
    {
        DBG(indent + tree.getType().toString());
        
        // Print all properties
        for (int i = 0; i < tree.getNumProperties(); ++i)
        {
            auto propName = tree.getPropertyName(i);
            DBG(indent + "  " + propName.toString() + ": " + tree.getProperty(propName).toString());
        }
        
        // Recursively print children
        for (int i = 0; i < tree.getNumChildren(); ++i)
            dumpTree(tree.getChild(i), indent + "  ");
    };
    
    dumpTree(edit->state, "");

    DBG("=== Edit Load Process Complete ===");
    return edit;
}

// FileBrowserListener methods (no longer used but kept for interface)
void LibraryComponent::selectionChanged() {}
void LibraryComponent::fileClicked (const juce::File&, const juce::MouseEvent&) {}
void LibraryComponent::fileDoubleClicked (const juce::File&) {}
void LibraryComponent::browserRootChanged (const juce::File&) {}

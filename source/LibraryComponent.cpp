/*
  ==============================================================================

    LibraryComponent.cpp
    Created: 17 Jan 2025 11:02:15pm
    Author:  Adam Hammad

  ==============================================================================
*/

#include "LibraryComponent.h"
#include "minibpm.h"


LibraryComponent::LibraryComponent()
{
    // Set up add file button
    addFileButton.setColour(juce::TextButton::buttonColourId, black);
    addFileButton.setColour(juce::TextButton::textColourOffId, matrixGreen);
    addFileButton.setColour(juce::TextButton::textColourOnId, matrixGreen);
    addAndMakeVisible(addFileButton);
    
    // Set up remove file button
    removeFileButton.setColour(juce::TextButton::buttonColourId, black);
    removeFileButton.setColour(juce::TextButton::textColourOffId, matrixGreen);
    removeFileButton.setColour(juce::TextButton::textColourOnId, matrixGreen);
    addAndMakeVisible(removeFileButton);
    
    // Set up edit BPM button
    editBpmButton.setButtonText("Edit BPM");
    editBpmButton.setColour(juce::TextButton::buttonColourId, black);
    editBpmButton.setColour(juce::TextButton::textColourOffId, matrixGreen);
    editBpmButton.setColour(juce::TextButton::textColourOnId, matrixGreen);
    addAndMakeVisible(editBpmButton);
    
    // Set up playlist table
    playlistTable = std::make_unique<juce::TableListBox>();
    playlistTable->setModel(this);
    playlistTable->getHeader().addColumn("Name", 1, 300);
    playlistTable->getHeader().addColumn("BPM", 2, 100);
    playlistTable->getHeader().setStretchToFitActive(true);
    playlistTable->setColour(juce::ListBox::backgroundColourId, black);
    playlistTable->setColour(juce::ListBox::outlineColourId, matrixGreen.withAlpha(0.5f));
    playlistTable->setColour(juce::ListBox::textColourId, matrixGreen);
    addAndMakeVisible(playlistTable.get());
    
    // Enable sorting
    playlistTable->getHeader().setSortColumnId(1, true); // Default sort by name
    
    // Set up button callbacks
    addFileButton.onClick = [this]() {
        fileChooser = std::make_shared<juce::FileChooser>(
            "Select Audio Files",
            juce::File::getSpecialLocation(juce::File::userMusicDirectory),
            "*.wav;*.mp3;*.aif;*.aiff");
            
        fileChooser->launchAsync(juce::FileBrowserComponent::openMode | 
                           juce::FileBrowserComponent::canSelectFiles |
                           juce::FileBrowserComponent::canSelectMultipleItems,
                           [this](const juce::FileChooser& fc) {
                               auto results = fc.getResults();
                               for (const auto& file : results) {
                                   if (file.exists()) {
                                       addToPlaylist(file);
                                   }
                               }
                           });
    };
    
    removeFileButton.onClick = [this]() {
        auto selectedRow = playlistTable->getSelectedRow();
        if (selectedRow >= 0) {
            removeFromPlaylist(selectedRow);
        }
    };
    
    editBpmButton.onClick = [this]() {
        auto selectedRow = playlistTable->getSelectedRow();
        if (selectedRow >= 0) {
            showBpmEditorWindow(selectedRow);
        }
    };
    
    // Load existing playlist
    loadPlaylist();
}

LibraryComponent::~LibraryComponent()
{
    savePlaylist();
}

void LibraryComponent::paint(juce::Graphics& g)
{
    g.fillAll(black);
    g.setColour(matrixGreen.withAlpha(0.5f));
    g.drawRect(getLocalBounds(), 1);
}

void LibraryComponent::resized()
{
    auto bounds = getLocalBounds();
    auto buttonHeight = 30;
    
    // Playlist table takes all space except bottom button area
    auto buttonArea = bounds.removeFromBottom(buttonHeight);
    playlistTable->setBounds(bounds.reduced(2));
    
    // Add buttons at the bottom
    addFileButton.setBounds(buttonArea.removeFromLeft(100).reduced(2));
    removeFileButton.setBounds(buttonArea.removeFromLeft(100).reduced(2));
    editBpmButton.setBounds(buttonArea.removeFromLeft(100).reduced(2));
}

// TableListBoxModel implementations
int LibraryComponent::getNumRows()
{
    return static_cast<int>(playlist.size());
}

void LibraryComponent::paintRowBackground(juce::Graphics& g, int rowNumber, int width, int height, bool rowIsSelected)
{
    if (rowIsSelected)
        g.fillAll(matrixGreen.withAlpha(0.3f));
}

void LibraryComponent::paintCell(juce::Graphics& g, int rowNumber, int columnId, int width, int height, bool rowIsSelected)
{
    if (rowNumber < playlist.size()) {
        g.setColour(matrixGreen);
        
        if (columnId == 1) // Name column
            g.drawText(playlist[rowNumber].name, 2, 0, width - 4, height, juce::Justification::centredLeft);
        else if (columnId == 2) // BPM column
            g.drawText(juce::String(playlist[rowNumber].bpm, 1), 2, 0, width - 4, height, juce::Justification::centred);
    }
}

void LibraryComponent::cellDoubleClicked(int rowNumber, int columnId, const juce::MouseEvent&)
{
    if (rowNumber < playlist.size() && onFileSelected) {
        juce::File file(playlist[rowNumber].filePath);
        if (file.exists())
            onFileSelected(file);
    }
}

void LibraryComponent::cellClicked(int rowNumber, int columnId, const juce::MouseEvent& event)
{
    if (event.mods.isRightButtonDown() && rowNumber < playlist.size())
    {
        juce::PopupMenu menu;
        menu.addItem(1, "Show in Finder");
        menu.addItem(2, "Remove");

        menu.showMenuAsync(juce::PopupMenu::Options(), [this, rowNumber](int result)
        {
            if (result == 1) // Show in Finder
            {
                juce::File file(playlist[rowNumber].filePath);
                if (file.exists())
                    file.revealToUser();
            }
            else if (result == 2) // Remove
            {
                removeFromPlaylist(rowNumber);
            }
        });
    }
}

void LibraryComponent::addToPlaylist(const juce::File& file)
{
    PlaylistEntry entry;
    entry.name = file.getFileNameWithoutExtension();
    entry.filePath = file.getFullPathName();
    entry.lastModified = file.getLastModificationTime().toMilliseconds();
    
    // Calculate BPM
    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();
    
    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file));
    if (reader)
    {
        // Create MiniBPM detector
        breakfastquay::MiniBPM bpmDetector(reader->sampleRate);
        bpmDetector.setBPMRange(60, 180);  // typical range for music
        
        // Process audio in chunks
        const int blockSize = 1024;
        juce::AudioBuffer<float> buffer(1, blockSize);
        std::vector<float> samples(blockSize);
        
        for (int pos = 0; pos < reader->lengthInSamples; pos += blockSize) 
        {
            const int numSamples = std::min(blockSize, 
                static_cast<int>(reader->lengthInSamples - pos));
                
            reader->read(&buffer, 0, numSamples, pos, true, false);
            memcpy(samples.data(), buffer.getReadPointer(0), numSamples * sizeof(float));
            
            bpmDetector.process(samples.data(), numSamples);
        }
        
        float detectedBPM = bpmDetector.estimateTempo();
        entry.bpm = detectedBPM > 0 ? detectedBPM : 120.0f;  // Use 120 as fallback
    }
    else
    {
        entry.bpm = 120.0f;  // Default BPM if file can't be read
    }
    
    playlist.push_back(entry);
    
    playlistTable->updateContent();
    savePlaylist();
}

void LibraryComponent::removeFromPlaylist(int index)
{
    if (index >= 0 && index < playlist.size()) {
        playlist.erase(playlist.begin() + index);
        playlistTable->updateContent();
        savePlaylist();
    }
}

void LibraryComponent::loadPlaylist()
{
    auto properties = juce::PropertiesFile::Options();
    properties.applicationName = "ChopShop";
    properties.filenameSuffix = "settings";
    properties.folderName = "ChopShop";
    properties.osxLibrarySubFolder = "Application Support";
    
    juce::ApplicationProperties appProperties;
    appProperties.setStorageParameters(properties);
    
    if (auto userSettings = appProperties.getUserSettings()) {
        auto playlistData = userSettings->getValue("playlist");
        if (playlistData.isNotEmpty()) {
            auto var = juce::JSON::parse(playlistData);
            if (auto* arr = var.getArray()) {
                for (const auto& entry : *arr) {
                    if (auto* obj = entry.getDynamicObject()) {
                        PlaylistEntry pe;
                        pe.name = obj->getProperty("name").toString();
                        pe.filePath = obj->getProperty("path").toString();
                        pe.lastModified = obj->getProperty("modified").toString().getLargeIntValue();
                        pe.bpm = static_cast<float>(obj->getProperty("bpm"));
                        playlist.push_back(pe);
                    }
                }
            }
        }
    }
}

void LibraryComponent::savePlaylist()
{
    auto properties = juce::PropertiesFile::Options();
    properties.applicationName = "ChopShop";
    properties.filenameSuffix = "settings";
    properties.folderName = "ChopShop";
    properties.osxLibrarySubFolder = "Application Support";
    
    juce::ApplicationProperties appProperties;
    appProperties.setStorageParameters(properties);
    
    if (auto userSettings = appProperties.getUserSettings()) {
        juce::Array<juce::var> playlistArray;
        
        for (const auto& entry : playlist) {
            juce::DynamicObject::Ptr obj = new juce::DynamicObject();
            obj->setProperty("name", entry.name);
            obj->setProperty("path", entry.filePath);
            obj->setProperty("modified", entry.lastModified);
            obj->setProperty("bpm", entry.bpm);
            playlistArray.add(obj.get());
        }
        
        userSettings->setValue("playlist", juce::JSON::toString(playlistArray));
        userSettings->save();
    }
}

// These methods are no longer used but kept for FileBrowserListener interface
void LibraryComponent::selectionChanged() {}
void LibraryComponent::fileClicked(const juce::File& file, const juce::MouseEvent& e) {}
void LibraryComponent::fileDoubleClicked(const juce::File& file) {}
void LibraryComponent::browserRootChanged(const juce::File& newRoot) {}

void LibraryComponent::showBpmEditorWindow(int rowIndex)
{
    if (rowIndex < 0 || rowIndex >= playlist.size())
        return;

    auto& entry = playlist[rowIndex];
    
    juce::DialogWindow::LaunchOptions options;
    
    auto content = std::make_unique<juce::Component>();
    content->setSize(200, 150);
    
    const auto matrixGreen = juce::Colour(0xFF00FF41);
    const auto black = juce::Colour(0xFF000000);
    
    auto editor = new juce::TextEditor();
    editor->setBounds(50, 20, 100, 24);
    editor->setText(juce::String(entry.bpm, 1));
    editor->setInputRestrictions(6, "0123456789.");
    editor->setColour(juce::TextEditor::backgroundColourId, black);
    editor->setColour(juce::TextEditor::textColourId, matrixGreen);
    editor->setColour(juce::TextEditor::outlineColourId, matrixGreen.withAlpha(0.5f));
    content->addAndMakeVisible(editor);
    
    auto halfButton = new juce::TextButton("1/2x");
    halfButton->setBounds(30, 60, 60, 24);
    halfButton->setColour(juce::TextButton::buttonColourId, black);
    halfButton->setColour(juce::TextButton::textColourOffId, matrixGreen);
    halfButton->setColour(juce::TextButton::textColourOnId, matrixGreen);
    halfButton->onClick = [editor]() {
        double currentValue = editor->getText().getDoubleValue();
        editor->setText(juce::String(currentValue * 0.5, 1));
    };
    content->addAndMakeVisible(halfButton);
    
    auto doubleButton = new juce::TextButton("2x");
    doubleButton->setBounds(110, 60, 60, 24);
    doubleButton->setColour(juce::TextButton::buttonColourId, black);
    doubleButton->setColour(juce::TextButton::textColourOffId, matrixGreen);
    doubleButton->setColour(juce::TextButton::textColourOnId, matrixGreen);
    doubleButton->onClick = [editor]() {
        double currentValue = editor->getText().getDoubleValue();
        editor->setText(juce::String(currentValue * 2.0, 1));
    };
    content->addAndMakeVisible(doubleButton);
    
    auto okButton = new juce::TextButton("OK");
    okButton->setBounds(50, 100, 100, 24);
    okButton->setColour(juce::TextButton::buttonColourId, black);
    okButton->setColour(juce::TextButton::textColourOffId, matrixGreen);
    okButton->setColour(juce::TextButton::textColourOnId, matrixGreen);
    okButton->onClick = [this, rowIndex, editor]() {
        float newBpm = editor->getText().getFloatValue();
        if (newBpm > 0) {
            playlist[rowIndex].bpm = newBpm;
            playlistTable->updateContent();
            savePlaylist();
        }
        if (auto* dw = juce::Component::getCurrentlyModalComponent())
            dw->exitModalState(0);
    };
    content->addAndMakeVisible(okButton);
    
    content->setColour(juce::ResizableWindow::backgroundColourId, black);
    
    options.content.setOwned(content.release());
    options.dialogTitle = "Edit BPM";
    options.dialogBackgroundColour = black;
    options.escapeKeyTriggersCloseButton = true;
    options.useNativeTitleBar = true;
    options.resizable = false;
    
    options.launchAsync();
}

void LibraryComponent::sortOrderChanged(int newSortColumnId, bool isForwards)
{
    if (newSortColumnId != sortedColumnId || isForwards != sortedForward)
    {
        sortedColumnId = newSortColumnId;
        sortedForward = isForwards;
        
        std::sort(playlist.begin(), playlist.end(), 
            [this](const PlaylistEntry& a, const PlaylistEntry& b) {
                if (sortedColumnId == 1) // Name column
                    return sortedForward ? 
                        (a.name.compareNatural(b.name) < 0) : 
                        (b.name.compareNatural(a.name) < 0);
                else // BPM column
                    return sortedForward ? 
                        (a.bpm < b.bpm) : 
                        (b.bpm < a.bpm);
            });
        
        playlistTable->updateContent();
    }
}   


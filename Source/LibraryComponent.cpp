/*
  ==============================================================================

    LibraryComponent.cpp
    Created: 17 Jan 2025 11:02:15pm
    Author:  Adam Hammad

  ==============================================================================
*/

#include "LibraryComponent.h"

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
    
    // Set up playlist table
    playlistTable = std::make_unique<juce::TableListBox>();
    playlistTable->setModel(this);
    playlistTable->getHeader().addColumn("Name", 1, 200);
    playlistTable->getHeader().addColumn("Path", 2, 300);
    playlistTable->setColour(juce::ListBox::backgroundColourId, black);
    playlistTable->setColour(juce::ListBox::outlineColourId, matrixGreen.withAlpha(0.5f));
    playlistTable->setColour(juce::ListBox::textColourId, matrixGreen);
    addAndMakeVisible(playlistTable.get());
    
    // Set up button callbacks
    addFileButton.onClick = [this]() {
        fileChooser = std::make_shared<juce::FileChooser>(
            "Select Audio File",
            juce::File::getSpecialLocation(juce::File::userMusicDirectory),
            "*.wav;*.mp3;*.aif;*.aiff");
            
        fileChooser->launchAsync(juce::FileBrowserComponent::openMode | 
                           juce::FileBrowserComponent::canSelectFiles,
                           [this](const juce::FileChooser& fc) {
                               auto result = fc.getResult();
                               if (result.exists()) {
                                   addToPlaylist(result);
                               }
                           });
    };
    
    removeFileButton.onClick = [this]() {
        auto selectedRow = playlistTable->getSelectedRow();
        if (selectedRow >= 0) {
            removeFromPlaylist(selectedRow);
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
    
    auto buttonArea = bounds.removeFromTop(buttonHeight);
    addFileButton.setBounds(buttonArea.removeFromLeft(100).reduced(2));
    removeFileButton.setBounds(buttonArea.removeFromLeft(100).reduced(2));
    
    // Add some padding
    bounds.removeFromTop(4);
    
    // Playlist table takes remaining space
    playlistTable->setBounds(bounds.reduced(2));
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
        else if (columnId == 2) // Path column
            g.drawText(playlist[rowNumber].filePath, 2, 0, width - 4, height, juce::Justification::centredLeft);
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

void LibraryComponent::addToPlaylist(const juce::File& file)
{
    PlaylistEntry entry;
    entry.name = file.getFileNameWithoutExtension();
    entry.filePath = file.getFullPathName();
    entry.lastModified = file.getLastModificationTime().toMilliseconds();
    
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
    properties.applicationName = "ChopScrew";
    properties.filenameSuffix = "settings";
    properties.folderName = "ChopScrew";
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
    properties.applicationName = "ChopScrew";
    properties.filenameSuffix = "settings";
    properties.folderName = "ChopScrew";
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

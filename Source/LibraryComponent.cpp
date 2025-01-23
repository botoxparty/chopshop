/*
  ==============================================================================

    LibraryComponent.cpp
    Created: 17 Jan 2025 11:02:15pm
    Author:  Adam Hammad

  ==============================================================================
*/

#include "LibraryComponent.h"

LibraryComponent::LibraryComponent()
    : timeSliceThread("Library Scanner Thread")
{
    // Style the choose folder button
    chooseFolderButton.setButtonText("Choose Library Folder");
    chooseFolderButton.setColour(juce::TextButton::buttonColourId, black);
    chooseFolderButton.setColour(juce::TextButton::textColourOffId, matrixGreen);
    chooseFolderButton.setColour(juce::TextButton::textColourOnId, matrixGreen);
    addAndMakeVisible(chooseFolderButton);
    
    // Start the background thread for file scanning
    timeSliceThread.startThread();
    
    // Set up file list component with audio file filter
    auto fileFilter = std::make_unique<juce::WildcardFileFilter>("*.wav;*.mp3;*.aif;*.aiff", "*", "Audio Files");
    directoryList = std::make_unique<juce::DirectoryContentsList>(fileFilter.release(), timeSliceThread);
    fileListComponent = std::make_unique<juce::FileListComponent>(*directoryList);
    
    // Style the file list component
    fileListComponent->setColour(juce::DirectoryContentsDisplayComponent::highlightColourId, matrixGreen.withAlpha(0.3f));
    fileListComponent->setColour(juce::DirectoryContentsDisplayComponent::textColourId, matrixGreen);
    fileListComponent->setColour(juce::ListBox::backgroundColourId, black);
    fileListComponent->setColour(juce::ListBox::outlineColourId, matrixGreen.withAlpha(0.5f));
    
    fileListComponent->addListener(this);
    addAndMakeVisible(fileListComponent.get());
    
    // Load the last used directory or default to user's music folder
    currentLibraryDirectory = getStoredLibraryDirectory();
    updateLibraryDirectory(currentLibraryDirectory);
    
    // Set up folder chooser button callback
    chooseFolderButton.onClick = [this]() {
        auto chooser = std::make_unique<juce::FileChooser>("Select Library Folder", 
                                                          currentLibraryDirectory,
                                                          "*");
        
        chooser->launchAsync(juce::FileBrowserComponent::openMode | 
                            juce::FileBrowserComponent::canSelectDirectories,
                            [this](const juce::FileChooser& fc) {
                                auto result = fc.getResult();
                                if (result.exists()) {
                                    updateLibraryDirectory(result);
                                    saveLibraryDirectory(result);
                                }
                            });
    };
}

LibraryComponent::~LibraryComponent()
{
    fileListComponent = nullptr;  // Destroy FileListComponent first
    directoryList = nullptr;      // Then destroy DirectoryContentsList
    timeSliceThread.stopThread(2000);
}

void LibraryComponent::paint(juce::Graphics& g)
{
    // Draw the background
    g.fillAll(black);
    
    // Draw border
    g.setColour(matrixGreen.withAlpha(0.5f));
    g.drawRect(getLocalBounds(), 1);
}

void LibraryComponent::resized()
{
    auto bounds = getLocalBounds();
    chooseFolderButton.setBounds(bounds.removeFromTop(30).reduced(2));
    
    // Add some padding between the button and file list
    bounds.removeFromTop(4);
    
    // File list takes up the remaining space
    fileListComponent->setBounds(bounds.reduced(2));
}

void LibraryComponent::updateLibraryDirectory(const juce::File& directory)
{
    currentLibraryDirectory = directory;
    directoryList->setDirectory(directory, true, true);
    fileListComponent->updateContent();
}

void LibraryComponent::saveLibraryDirectory(const juce::File& directory)
{
    auto properties = juce::PropertiesFile::Options();
    properties.applicationName = "ChopScrew";
    properties.filenameSuffix = "settings";
    properties.folderName = "ChopScrew";
    properties.osxLibrarySubFolder = "Application Support";
    
    juce::ApplicationProperties appProperties;
    appProperties.setStorageParameters(properties);
    
    if (auto userSettings = appProperties.getUserSettings()) {
        userSettings->setValue("libraryDirectory", directory.getFullPathName());
        userSettings->save();
    }
}

juce::File LibraryComponent::getStoredLibraryDirectory()
{
    auto properties = juce::PropertiesFile::Options();
    properties.applicationName = "ChopScrew";
    properties.filenameSuffix = "settings";
    properties.folderName = "ChopScrew";
    properties.osxLibrarySubFolder = "Application Support";
    
    juce::ApplicationProperties appProperties;
    appProperties.setStorageParameters(properties);
    
    if (auto userSettings = appProperties.getUserSettings()) {
        auto savedPath = userSettings->getValue("libraryDirectory");
        if (savedPath.isNotEmpty()) {
            return juce::File(savedPath);
        }
    }
    
    return juce::File("/Users/adamhammad/Downloads/ChoppedTracks");
}

void LibraryComponent::selectionChanged()
{
    // Handle selection changes if needed
}

void LibraryComponent::fileClicked(const juce::File& file, const juce::MouseEvent& e)
{
    // Handle single clicks if needed
}

void LibraryComponent::fileDoubleClicked(const juce::File& file)
{
    if (onFileSelected)
        onFileSelected(file);
}

void LibraryComponent::browserRootChanged(const juce::File& newRoot)
{
    // Handle root directory changes if needed
}

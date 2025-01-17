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
    chooseFolderButton.setButtonText("Choose Library Folder");
    addAndMakeVisible(chooseFolderButton);
    
    // Start the background thread for file scanning
    timeSliceThread.startThread();
    
    // Set up file list component with audio file filter
    auto fileFilter = std::make_unique<juce::WildcardFileFilter>("*.wav;*.mp3;*.aif;*.aiff", "*", "Audio Files");
    directoryList = std::make_unique<juce::DirectoryContentsList>(fileFilter.release(), timeSliceThread);
    fileListComponent = std::make_unique<juce::FileListComponent>(*directoryList);
    
    fileListComponent->addListener(this);
    addAndMakeVisible(fileListComponent.get());
    
    // Load the last used directory or default to user's music folder
    currentLibraryDirectory = getStoredLibraryDirectory();
    updateLibraryDirectory(currentLibraryDirectory);
    
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
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
}

void LibraryComponent::resized()
{
    auto bounds = getLocalBounds();
    chooseFolderButton.setBounds(bounds.removeFromTop(30).reduced(2));
    fileListComponent->setBounds(bounds.reduced(2));

    // Debug painting
    // g.setColour(juce::Colours::red);
    // g.drawRect(getLocalBounds(), 1); // This will draw a red border around the component
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

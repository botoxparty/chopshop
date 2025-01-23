/*
  ==============================================================================

    LibraryComponent.h
    Created: 17 Jan 2025 11:02:15pm
    Author:  Adam Hammad

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

class LibraryComponent : public juce::Component,
                        public juce::FileBrowserListener
{
public:
    LibraryComponent();
    ~LibraryComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    
    // FileBrowserListener methods
    void selectionChanged() override;
    void fileClicked(const juce::File& file, const juce::MouseEvent& e) override;
    void fileDoubleClicked(const juce::File& file) override;
    void browserRootChanged(const juce::File& newRoot) override;

    std::function<void(const juce::File&)> onFileSelected;

private:
    void updateLibraryDirectory(const juce::File& directory);
    void saveLibraryDirectory(const juce::File& directory);
    juce::File getStoredLibraryDirectory();

    const juce::Colour matrixGreen { 0xFF00FF41 };  // Bright matrix green
    const juce::Colour darkWire { 0xFF003B00 };     // Dark green for backgrounds
    const juce::Colour black { 0xFF000000 };        // Pure black

    juce::TextButton chooseFolderButton{"Choose Library Folder"};
    std::unique_ptr<juce::DirectoryContentsList> directoryList;
    std::unique_ptr<juce::FileListComponent> fileListComponent;
    juce::TimeSliceThread timeSliceThread{"Library Scanner Thread"};
    
    juce::File currentLibraryDirectory;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LibraryComponent)
};

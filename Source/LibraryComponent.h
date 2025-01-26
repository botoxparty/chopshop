/*
  ==============================================================================

    LibraryComponent.h
    Created: 17 Jan 2025 11:02:15pm
    Author:  Adam Hammad

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

struct PlaylistEntry {
    juce::String name;
    juce::String filePath;
    juce::int64 lastModified;
    float bpm;
};

class LibraryComponent : public juce::Component,
                        public juce::FileBrowserListener,
                        public juce::TableListBoxModel
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

    // TableListBoxModel methods
    int getNumRows() override;
    void paintRowBackground(juce::Graphics& g, int rowNumber, int width, int height, bool rowIsSelected) override;
    void paintCell(juce::Graphics& g, int rowNumber, int columnId, int width, int height, bool rowIsSelected) override;
    void cellDoubleClicked(int rowNumber, int columnId, const juce::MouseEvent&) override;
    void cellClicked(int rowNumber, int columnId, const juce::MouseEvent& event) override;

    std::function<void(const juce::File&)> onFileSelected;

    float getBPMForFile(const juce::File& file) const {
        auto it = std::find_if(playlist.begin(), playlist.end(),
            [&file](const PlaylistEntry& entry) {
                return entry.filePath == file.getFullPathName();
            });
        return it != playlist.end() ? it->bpm : 120.0f;
    }

private:
    void addToPlaylist(const juce::File& file);
    void removeFromPlaylist(int index);
    void loadPlaylist();
    void savePlaylist();
    
    const juce::Colour matrixGreen { 0xFF00FF41 };  // Bright matrix green
    const juce::Colour darkWire { 0xFF003B00 };     // Dark green for backgrounds
    const juce::Colour black { 0xFF000000 };        // Pure black

    juce::TextButton addFileButton{"Add Files"};
    juce::TextButton removeFileButton{"Remove"};
    std::unique_ptr<juce::TableListBox> playlistTable;
    
    std::vector<PlaylistEntry> playlist;
    
    std::shared_ptr<juce::FileChooser> fileChooser;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LibraryComponent)
};

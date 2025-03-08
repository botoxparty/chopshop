/*
  ==============================================================================

    LibraryComponent.h
    Created: 17 Jan 2025 11:02:15pm
    Author:  Adam Hammad

  ==============================================================================
*/

#pragma once


#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <tracktion_engine/tracktion_engine.h>
#include "Plugins/ChopPlugin.h"
#include "Plugins/AutoDelayPlugin.h"
#include "Plugins/FlangerPlugin.h"
#include "Plugins/AutoPhaserPlugin.h"
#include "Utilities.h"

// We'll use ProjectItem instead of PlaylistEntry
class LibraryComponent : public juce::Component,
                        public juce::FileBrowserListener,
                        public juce::TableListBoxModel
{
public:
    LibraryComponent(tracktion::engine::Engine& engineToUse);
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
    void sortOrderChanged(int newSortColumnId, bool isForwards) override;

    std::function<void(std::unique_ptr<tracktion::engine::Edit>)> onEditSelected;

    float getBPMForFile(const juce::File& file) const {
        auto projectItem = getProjectItemForFile(file);
        if (projectItem != nullptr)
            return projectItem->getNamedProperty("bpm").getFloatValue();
        return 120.0f;
    }

private:
    void addToLibrary(const juce::File& file);
    void removeFromLibrary(int index);
    void loadLibrary();
    void showBpmEditorWindow(int rowIndex);
    void createPluginRack(std::unique_ptr<tracktion::engine::Edit>& edit);
    
    tracktion::engine::ProjectItem::Ptr getProjectItemForFile(const juce::File& file) const;
    
    const juce::Colour matrixGreen { 0xFF00FF41 };  // Bright matrix green
    const juce::Colour darkWire { 0xFF003B00 };     // Dark green for backgrounds
    const juce::Colour black { 0xFF000000 };        // Pure black

    juce::TextButton addFileButton{"Add File"};
    juce::TextButton removeFileButton{"Remove File"};
    juce::TextButton editBpmButton{"Edit BPM"};
    
    std::unique_ptr<juce::TableListBox> playlistTable;
    
    tracktion::engine::Engine& engine;
    tracktion::engine::Project::Ptr libraryProject;
    
    std::shared_ptr<juce::FileChooser> fileChooser;
    
    int sortedColumnId = 0;  // 0 means unsorted
    bool sortedForward = true;
    
    // Helper to load an Edit from a project item
    std::unique_ptr<tracktion::engine::Edit> loadEditFromProjectItem(tracktion::engine::ProjectItem::Ptr projectItem);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LibraryComponent)
};

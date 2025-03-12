#pragma once

#include <juce_core/juce_core.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <tracktion_engine/tracktion_engine.h>
#include "AutomationLane.h"
#include "ZoomState.h"

class PluginAutomationComponent : public juce::Component
{
public:
    class HeightListener
    {
    public:
        virtual ~HeightListener() = default;
        virtual void heightChanged() = 0;
    };

    void addHeightListener(HeightListener* listener) { heightListener = listener; }
    void removeHeightListener(HeightListener* listener) { if (heightListener == listener) heightListener = nullptr; }

    PluginAutomationComponent(tracktion::engine::Edit& e, tracktion::engine::Plugin* p = nullptr, ZoomState& zs = ZoomState::instance());
    ~PluginAutomationComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void setPlugin(tracktion::engine::Plugin* plugin);
    
    // Add method to get preferred height
    float getPreferredHeight() const;
    void updateSize();

private:
    void updateAutomationLanes();
    void createAutomationLaneForParameter(tracktion::engine::AutomatableParameter* param);
    void toggleLaneCollapsed(size_t laneIndex);
    void toggleGroupCollapsed();
    void notifyHeightChanged() 
    { 
        if (heightListener != nullptr)
            heightListener->heightChanged();
    }

    tracktion::engine::Edit& edit;
    tracktion::engine::Plugin* plugin = nullptr;
    bool isGroupCollapsed = true;
    std::unique_ptr<juce::DrawableButton> groupCollapseButton;
    ZoomState& zoomState;
    
    struct AutomationLaneInfo {
        std::unique_ptr<AutomationLane> lane;
        std::unique_ptr<juce::Label> nameLabel;
        std::unique_ptr<juce::DrawableButton> collapseButton;
        bool isCollapsed = false;
        
        AutomationLaneInfo() {
            nameLabel = std::make_unique<juce::Label>("", "");
            nameLabel->setJustificationType(juce::Justification::centredLeft);
            nameLabel->setFont(juce::Font(14.0f));
            
            // Create collapse button with custom path
            auto* collapseIcon = new juce::DrawablePath();
            juce::Path path;
            path.addTriangle(0.0f, 0.0f, 10.0f, 0.0f, 5.0f, 10.0f);
            collapseIcon->setPath(path);
            collapseIcon->setFill(juce::Colours::white);
            
            auto* collapseIconClosed = new juce::DrawablePath();
            juce::Path closedPath;
            closedPath.addTriangle(0.0f, 0.0f, 10.0f, 5.0f, 0.0f, 10.0f);
            collapseIconClosed->setPath(closedPath);
            collapseIconClosed->setFill(juce::Colours::white);
            
            collapseButton = std::make_unique<juce::DrawableButton>("collapse", juce::DrawableButton::ImageFitted);
            collapseButton->setImages(collapseIcon, nullptr, nullptr, nullptr, collapseIconClosed);
        }
        
        AutomationLaneInfo(const AutomationLaneInfo&) = delete;
        AutomationLaneInfo& operator=(const AutomationLaneInfo&) = delete;
        AutomationLaneInfo(AutomationLaneInfo&& other) noexcept = default;
        AutomationLaneInfo& operator=(AutomationLaneInfo&& other) noexcept = default;
    };
    
    std::vector<AutomationLaneInfo> automationLanes;
    
    const float laneHeight = 60.0f;
    const float collapsedLaneHeight = 25.0f;
    const float labelWidth = 150.0f;
    const float headerHeight = 20.0f;
    
    HeightListener* heightListener = nullptr;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginAutomationComponent)
}; 
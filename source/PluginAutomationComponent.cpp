#include "PluginAutomationComponent.h"

PluginAutomationComponent::PluginAutomationComponent(tracktion::engine::Edit& e, tracktion::engine::Plugin* p, ZoomState& zs)
    : edit(e)
    , plugin(nullptr)
    , zoomState(zs)
    , isGroupCollapsed(true)  // Initialize as collapsed
{
    // Create group collapse button with custom path
    auto* collapseIcon = new juce::DrawablePath();
    juce::Path path;
    // Point down for open state
    path.addTriangle(0.0f, 0.0f, 10.0f, 0.0f, 5.0f, 10.0f);
    collapseIcon->setPath(path);
    collapseIcon->setFill(juce::Colours::white);
    
    auto* collapseIconClosed = new juce::DrawablePath();
    juce::Path closedPath;
    // Point right for closed state
    closedPath.addTriangle(0.0f, 0.0f, 10.0f, 5.0f, 0.0f, 10.0f);
    collapseIconClosed->setPath(closedPath);
    collapseIconClosed->setFill(juce::Colours::white);
    
    groupCollapseButton = std::make_unique<juce::DrawableButton>("groupCollapse", juce::DrawableButton::ImageFitted);
    groupCollapseButton->setImages(collapseIcon, nullptr, nullptr, nullptr, collapseIconClosed);
    groupCollapseButton->setToggleState(true, juce::dontSendNotification); // Set initial state to collapsed
    groupCollapseButton->onClick = [this]() { toggleGroupCollapsed(); };
    addAndMakeVisible(*groupCollapseButton);

    setPlugin(p);
}

PluginAutomationComponent::~PluginAutomationComponent()
{
}

void PluginAutomationComponent::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
    
    // Draw plugin name if available
    if (plugin != nullptr)
    {
        auto headerBounds = getLocalBounds().removeFromTop(headerHeight);
        
        // Draw metallic header background gradient
        juce::ColourGradient headerGradient(
            juce::Colours::grey.brighter(0.2f),
            headerBounds.getTopLeft().toFloat(),
            juce::Colours::grey.darker(0.2f),
            headerBounds.getBottomLeft().toFloat(),
            false);
        g.setGradientFill(headerGradient);
        g.fillRect(headerBounds);
        
        // Add subtle highlight at top
        g.setColour(juce::Colours::white.withAlpha(0.1f));
        g.fillRect(headerBounds.removeFromTop(1));
        
        // Add shadow at bottom
        g.setColour(juce::Colours::black.withAlpha(0.2f));
        g.fillRect(headerBounds.getX(), headerBounds.getBottom() - 1, headerBounds.getWidth(), 1);
        
        // Draw plugin name with text shadow
        g.setFont(juce::Font(16.0f).boldened());
        auto textBounds = headerBounds;
        textBounds.removeFromLeft(25); // Space for collapse button
        
        // Draw text shadow
        g.setColour(juce::Colours::black.withAlpha(0.5f));
        g.drawText(plugin->getName(), textBounds.translated(1, 1), juce::Justification::centredLeft, true);
        
        // Draw main text
        g.setColour(juce::Colours::white);
        g.drawText(plugin->getName(), textBounds, juce::Justification::centredLeft, true);
    }
    
    // Draw separators between lanes
    if (!isGroupCollapsed)
    {
        auto bounds = getLocalBounds();
        bounds.removeFromTop(headerHeight);
        
        for (size_t i = 0; i < automationLanes.size(); ++i)
        {
            auto& laneInfo = automationLanes[i];
            float currentLaneHeight = laneInfo.isCollapsed ? collapsedLaneHeight : laneHeight;
            
            // Draw lane background with metallic effect
            auto laneBounds = bounds.removeFromTop(currentLaneHeight);
            
            // Metallic background gradient
            juce::ColourGradient laneGradient(
                getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId).brighter(0.1f),
                laneBounds.getTopLeft().toFloat(),
                getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId).darker(0.1f),
                laneBounds.getBottomLeft().toFloat(),
                false);
            g.setGradientFill(laneGradient);
            g.fillRect(laneBounds);
            
            // Draw header section with different gradient
            auto headerBounds = laneBounds.removeFromTop(25);
            juce::ColourGradient headerGradient(
                juce::Colours::grey.darker(0.2f),
                headerBounds.getTopLeft().toFloat(),
                juce::Colours::grey.darker(0.4f),
                headerBounds.getBottomLeft().toFloat(),
                false);
            g.setGradientFill(headerGradient);
            g.fillRect(headerBounds);
            
            // Add subtle highlight at top of header
            g.setColour(juce::Colours::white.withAlpha(0.05f));
            g.fillRect(headerBounds.removeFromTop(1));
            
            // Add separator line at bottom of lane
            if (i < automationLanes.size() - 1)
            {
                g.setColour(juce::Colours::black.withAlpha(0.3f));
                g.fillRect(laneBounds.getX(), laneBounds.getBottom() - 1, laneBounds.getWidth(), 1);
                g.setColour(juce::Colours::white.withAlpha(0.02f));
                g.fillRect(laneBounds.getX(), laneBounds.getBottom(), laneBounds.getWidth(), 1);
            }
        }
    }
}

void PluginAutomationComponent::resized()
{
    auto bounds = getLocalBounds();
    
    // Position group collapse button in header
    if (groupCollapseButton != nullptr)
    {
        const int buttonSize = 15;
        const int margin = 5;
        groupCollapseButton->setBounds(bounds.getX() + margin,
                                     bounds.getY() + (headerHeight - buttonSize) / 2,
                                     buttonSize,
                                     buttonSize);
    }
    
    // Reserve space for plugin name
    auto headerBounds = bounds.removeFromTop(headerHeight);
    
    // If group is collapsed, don't layout the lanes
    if (isGroupCollapsed)
        return;
    
    // Layout automation lanes
    for (size_t i = 0; i < automationLanes.size(); ++i)
    {
        auto& laneInfo = automationLanes[i];
        
        // Calculate lane height based on collapsed state
        float currentLaneHeight = laneInfo.isCollapsed ? collapsedLaneHeight : laneHeight;
        auto laneBounds = bounds.removeFromTop(currentLaneHeight);
        
        // Create header section
        auto headerBounds = laneBounds.removeFromTop(25);
        
        // Position collapse button
        if (laneInfo.collapseButton != nullptr)
        {
            const int buttonSize = 15;
            const int margin = 5;
            laneInfo.collapseButton->setBounds(headerBounds.getX() + margin, 
                                             headerBounds.getY() + (headerBounds.getHeight() - buttonSize) / 2,
                                             buttonSize, 
                                             buttonSize);
        }
        
        // Position label next to collapse button
        if (laneInfo.nameLabel != nullptr)
        {
            const int labelMargin = 25; // Space for collapse button
            laneInfo.nameLabel->setBounds(headerBounds.getX() + labelMargin, 
                                        headerBounds.getY(),
                                        labelWidth - labelMargin,
                                        headerBounds.getHeight());
        }
        
        // Position automation lane if not collapsed
        if (!laneInfo.isCollapsed && laneInfo.lane != nullptr)
        {
            laneInfo.lane->setBounds(laneBounds);
            laneInfo.lane->setVisible(true);
        }
        else if (laneInfo.lane != nullptr)
        {
            laneInfo.lane->setVisible(false);
        }
    }
}

void PluginAutomationComponent::setPlugin(tracktion::engine::Plugin* p)
{
    plugin = p;
    updateAutomationLanes();
    resized();
}

void PluginAutomationComponent::updateAutomationLanes()
{
    // Clear existing lanes
    automationLanes.clear();
    
    if (plugin != nullptr)
    {
        // Get all automatable parameters from the plugin
        auto params = plugin->getAutomatableParameters();
        
        // Create a lane for each parameter
        for (auto* param : params)
        {
            if (param != nullptr)
                createAutomationLaneForParameter(param);
        }
    }
    
    resized();
}

void PluginAutomationComponent::createAutomationLaneForParameter(tracktion::engine::AutomatableParameter* param)
{
    if (param == nullptr)
        return;
        
    AutomationLaneInfo laneInfo;
    
    // Create and setup automation lane
    laneInfo.lane = std::make_unique<AutomationLane>(edit, zoomState);
    laneInfo.lane->setParameter(param);
    addAndMakeVisible(*laneInfo.lane);
    
    // Setup label
    laneInfo.nameLabel->setText(param->getParameterName(), juce::dontSendNotification);
    laneInfo.nameLabel->setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(*laneInfo.nameLabel);
    
    // Setup collapse button
    const size_t laneIndex = automationLanes.size();
    laneInfo.collapseButton->onClick = [this, laneIndex]() { toggleLaneCollapsed(laneIndex); };
    addAndMakeVisible(*laneInfo.collapseButton);
    
    automationLanes.push_back(std::move(laneInfo));
}

float PluginAutomationComponent::getPreferredHeight() const
{
    if (isGroupCollapsed)
        return headerHeight;

    float totalHeight = headerHeight;
    
    for (const auto& laneInfo : automationLanes)
    {
        totalHeight += laneInfo.isCollapsed ? collapsedLaneHeight : laneHeight;
    }
    
    return totalHeight;
}

void PluginAutomationComponent::updateSize()
{
    auto preferredHeight = getPreferredHeight();
    auto currentBounds = getBounds();
    setBounds(currentBounds.withHeight(static_cast<int>(preferredHeight)));
}

void PluginAutomationComponent::toggleGroupCollapsed()
{
    isGroupCollapsed = !isGroupCollapsed;
    groupCollapseButton->setToggleState(isGroupCollapsed, juce::dontSendNotification);
    
    // Show/hide all automation lanes based on group state
    for (auto& laneInfo : automationLanes)
    {
        if (laneInfo.lane != nullptr)
            laneInfo.lane->setVisible(!isGroupCollapsed);
        if (laneInfo.nameLabel != nullptr)
            laneInfo.nameLabel->setVisible(!isGroupCollapsed);
        if (laneInfo.collapseButton != nullptr)
            laneInfo.collapseButton->setVisible(!isGroupCollapsed);
    }
    
    notifyHeightChanged();
    resized();
}

void PluginAutomationComponent::toggleLaneCollapsed(size_t laneIndex)
{
    if (laneIndex < automationLanes.size())
    {
        auto& laneInfo = automationLanes[laneIndex];
        laneInfo.isCollapsed = !laneInfo.isCollapsed;
        laneInfo.collapseButton->setToggleState(laneInfo.isCollapsed, juce::dontSendNotification);
        
        if (laneInfo.lane != nullptr)
            laneInfo.lane->setVisible(!laneInfo.isCollapsed);
            
        notifyHeightChanged();
        resized();
    }
} 
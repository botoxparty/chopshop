#pragma once

#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>

class ZoomStateListener
{
public:
    virtual ~ZoomStateListener() = default;
    virtual void zoomLevelChanged(double newLevel) {}
    virtual void scrollPositionChanged(double newPosition) {}
    virtual void gridSizeChanged(float newGridSize) {}
};

class ZoomState : public juce::ChangeBroadcaster
{
public:
    ZoomState();
    
    // Getters
    double getZoomLevel() const { return zoomLevel; }
    double getScrollPosition() const { return scrollPosition; }
    double getMaxScrollPosition() const { return zoomLevel > 1.0 ? 1.0 - (1.0 / zoomLevel) : 0.0; }
    float getGridSize() const { return gridSize; }
    
    // Setters (these will notify listeners)
    void setZoomLevel(double newLevel);
    void setScrollPosition(double newPosition);
    void setGridSize(float newSize);
    
    // Listener management
    void addListener(ZoomStateListener* listener);
    void removeListener(ZoomStateListener* listener);
    
    // Singleton access
    static ZoomState& instance() {
        static ZoomState instance;
        return instance;
    }
    
private:
    double zoomLevel;
    double scrollPosition;
    float gridSize;
    juce::Array<ZoomStateListener*> listeners;
    
    static constexpr double minZoom = 1.0;
    static constexpr double maxZoom = 20.0;
    static constexpr float defaultGridSize = 1.0f;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZoomState)
}; 
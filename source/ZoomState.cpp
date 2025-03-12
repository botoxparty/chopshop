#include "ZoomState.h"

ZoomState::ZoomState()
    : zoomLevel(maxZoom)
    , scrollPosition(0.0)
    , gridSize(defaultGridSize)
{
}

void ZoomState::setZoomLevel(double newLevel)
{
    newLevel = juce::jlimit(minZoom, maxZoom, newLevel);
    
    if (std::abs(zoomLevel - newLevel) > 0.0001)
    {
        zoomLevel = newLevel;
        
        // Notify listeners
        for (auto* listener : listeners)
            listener->zoomLevelChanged(zoomLevel);
            
        sendChangeMessage();
    }
}

void ZoomState::setScrollPosition(double newPosition)
{
    // Calculate max scroll based on zoom level
    double maxScroll = zoomLevel > 1.0 ? 1.0 - (1.0 / zoomLevel) : 0.0;
    newPosition = juce::jlimit(0.0, maxScroll, newPosition);
    
    if (std::abs(scrollPosition - newPosition) > 0.0001)
    {
        scrollPosition = newPosition;
        
        // Notify listeners
        for (auto* listener : listeners)
            listener->scrollPositionChanged(scrollPosition);
            
        sendChangeMessage();
    }
}

void ZoomState::setGridSize(float newSize)
{
    if (std::abs(gridSize - newSize) > 0.0001f)
    {
        gridSize = newSize;
        
        // Notify listeners
        for (auto* listener : listeners)
            listener->gridSizeChanged(gridSize);
            
        sendChangeMessage();
    }
}

void ZoomState::addListener(ZoomStateListener* listener)
{
    listeners.addIfNotAlreadyThere(listener);
}

void ZoomState::removeListener(ZoomStateListener* listener)
{
    listeners.removeFirstMatchingValue(listener);
} 
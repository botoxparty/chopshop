// RegionManager.h

#include "tracktion_engine/tracktion_engine.h"
class RegionManager {
public:
    static constexpr double GAP = 0.005; // Consistent gap size

    RegionManager(tracktion::engine::AutomatableParameter* param)
        : parameter(param)
    {
        if (parameter != nullptr)
            parameter->getCurve().setOwnerParameter(parameter);
    }

    struct Region {
        double startTime;
        double endTime;
        bool isASide;
        float mixAmount;
        std::array<int, 4> pointIndices;  // [preGap, start, end, postGap]

        Region(double start, double end, bool a, float mix = 1.0f)
            : startTime(start), endTime(end), isASide(a), mixAmount(mix), 
              pointIndices{-1, -1, -1, -1} {}
    };

    void addRegion(const Region& region) {
        if (parameter == nullptr) return;

        auto& curve = parameter->getCurve();

        DBG("ADD REGION - Creating new region at [" + juce::String(region.startTime) + " -> " + juce::String(region.endTime) + "]" +
            " (isASide: " + juce::String(region.isASide ? "true" : "false") + ")");
        
        // Log existing regions
        DBG("  Current regions:");
        for (size_t i = 0; i < regions.size(); ++i) {
            const auto& existingRegion = regions[i];
            DBG("    Region " + juce::String(i) + ": [" + 
                juce::String(existingRegion.startTime) + " -> " + 
                juce::String(existingRegion.endTime) + "]" +
                " (isASide: " + juce::String(existingRegion.isASide ? "true" : "false") + ")");
        }
        
        // Clear any existing points in the region
        for (int i = curve.getNumPoints() - 1; i >= 0; --i) {
            auto time = curve.getPointTime(i).inSeconds();
            if (time >= region.startTime - GAP && time <= region.endTime + GAP) {
                curve.removePoint(i);
            }
        }

        Region newRegion = region;
        int currentIndex = curve.getNumPoints();

        // Add points in sequence and store their indices
        if (region.startTime > 0) {
            float startValue = region.isASide ? 0.0f : 1.0f;
            curve.addPoint(tracktion::TimePosition::fromSeconds(region.startTime - GAP), 
                         startValue, 0.0f);
            newRegion.pointIndices[0] = currentIndex++;
        }

        float mainValue = region.isASide ? 1.0f : 0.0f;
        curve.addPoint(tracktion::TimePosition::fromSeconds(region.startTime), 
                      mainValue, 0.0f);
        newRegion.pointIndices[1] = currentIndex++;
        
        curve.addPoint(tracktion::TimePosition::fromSeconds(region.endTime), 
                      mainValue, 0.0f);
        newRegion.pointIndices[2] = currentIndex++;
        
        curve.addPoint(tracktion::TimePosition::fromSeconds(region.endTime + GAP), 
                      region.isASide ? 0.0f : 1.0f, 0.0f);
        newRegion.pointIndices[3] = currentIndex;

        // Add the region to the vector
        regions.push_back(newRegion);
        
        DBG("  Region added successfully. Total regions: " + juce::String(regions.size()));
    }

    void removeRegion(size_t index) {
        if (index >= regions.size() || parameter == nullptr) return;

        const auto& regionToRemove = regions[index];
        auto& curve = parameter->getCurve();

        // Remove points in reverse order to maintain indices
        for (int i = 3; i >= 0; --i) {
            if (regionToRemove.pointIndices[i] >= 0) {
                curve.removePoint(regionToRemove.pointIndices[i]);
            }
        }

        // Remove from regions vector
        regions.erase(regions.begin() + index);

        // Update indices for remaining regions
        for (auto& region : regions) {
            for (auto& pointIndex : region.pointIndices) {
                if (pointIndex > regionToRemove.pointIndices[3]) {
                    pointIndex -= 4; // Adjust for removed points
                }
            }
        }
    }

    void moveRegion(size_t index, double newStartTime) {
        if (index >= regions.size() || parameter == nullptr) return;

        auto& region = regions[index];
        double duration = region.endTime - region.startTime;
        double newEndTime = newStartTime + duration;

        DBG("MOVE REGION - Moving region " + juce::String(index) + 
            " from [" + juce::String(region.startTime) + " -> " + juce::String(region.endTime) + "]" +
            " to [" + juce::String(newStartTime) + " -> " + juce::String(newEndTime) + "]");

        // Check for overlaps with other regions
        for (size_t i = 0; i < regions.size(); ++i) {
            if (i != index) {
                const auto& otherRegion = regions[i];
                DBG("  Checking overlap with region " + juce::String(i) + 
                    " at [" + juce::String(otherRegion.startTime) + " -> " + juce::String(otherRegion.endTime) + "]");
                
                // Check if the new position would overlap with another region
                bool startOverlap = (newStartTime >= otherRegion.startTime && newStartTime < otherRegion.endTime);
                bool endOverlap = (newEndTime > otherRegion.startTime && newEndTime <= otherRegion.endTime);
                bool encompassOverlap = (newStartTime <= otherRegion.startTime && newEndTime >= otherRegion.endTime);
                
                if (startOverlap || endOverlap || encompassOverlap) {
                    DBG("  OVERLAP DETECTED! Move cancelled.");
                    if (startOverlap) DBG("    - Start point overlaps");
                    if (endOverlap) DBG("    - End point overlaps");
                    if (encompassOverlap) DBG("    - Complete encompass overlap");
                    return;
                }
            }
        }

        DBG("  No overlaps found, proceeding with move");

        auto& curve = parameter->getCurve();

        // First, remove the existing points for this region
        for (int i = 3; i >= 0; --i) {
            if (region.pointIndices[i] >= 0) {
                curve.removePoint(region.pointIndices[i]);
            }
        }

        // Update indices for all regions after removal
        for (auto& r : regions) {
            for (auto& pointIndex : r.pointIndices) {
                if (pointIndex > region.pointIndices[3]) {
                    pointIndex -= 4; // Adjust for removed points
                }
            }
        }

        // Find the correct insertion point in the curve
        int insertionIndex = 0;
        for (size_t i = 0; i < regions.size(); ++i) {
            if (i != index && regions[i].startTime < newStartTime) {
                insertionIndex = regions[i].pointIndices[3] + 1;
            }
        }

        DBG("  Inserting points at index: " + juce::String(insertionIndex));

        // Add new points at the correct position
        if (newStartTime > 0) {
            float startValue = region.isASide ? 0.0f : 1.0f;
            curve.addPoint(tracktion::TimePosition::fromSeconds(newStartTime - GAP), 
                         startValue, 0.0f);
            region.pointIndices[0] = insertionIndex++;
        }

        float mainValue = region.isASide ? 1.0f : 0.0f;
        curve.addPoint(tracktion::TimePosition::fromSeconds(newStartTime), 
                      mainValue, 0.0f);
        region.pointIndices[1] = insertionIndex++;
        
        curve.addPoint(tracktion::TimePosition::fromSeconds(newEndTime), 
                      mainValue, 0.0f);
        region.pointIndices[2] = insertionIndex++;
        
        curve.addPoint(tracktion::TimePosition::fromSeconds(newEndTime + GAP), 
                      region.isASide ? 0.0f : 1.0f, 0.0f);
        region.pointIndices[3] = insertionIndex;

        // Update indices for all regions after insertion
        for (auto& r : regions) {
            if (&r != &region) { // Skip the moved region
                for (auto& pointIndex : r.pointIndices) {
                    if (pointIndex >= insertionIndex - 3) { // -3 because we've already incremented insertionIndex
                        pointIndex += 4;
                    }
                }
            }
        }

        // Update region times
        region.startTime = newStartTime;
        region.endTime = newEndTime;
        
        DBG("  Move completed successfully");
    }

    void clearRegions() {
        if (parameter == nullptr) return;
        parameter->getCurve().clear();
        regions.clear();
    }

    const std::vector<Region>& getRegions() const { return regions; }
    
    Region* getRegionAtTime(double time) {
        for (auto& region : regions) {
            if (time >= region.startTime && time <= region.endTime)
                return &region;
        }
        return nullptr;
    }

private:
    tracktion::engine::AutomatableParameter* parameter;
    std::vector<Region> regions;
};
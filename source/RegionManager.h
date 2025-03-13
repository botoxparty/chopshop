// RegionManager.h

#include "tracktion_engine/tracktion_engine.h"
class RegionManager {
public:
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

        Region(double start, double end, bool a, float mix = 1.0f)
            : startTime(start), endTime(end), isASide(a), mixAmount(mix) {}
    };

    void addRegion(const Region& region) {
        if (parameter == nullptr) return;

        auto& curve = parameter->getCurve();

        DBG("ADD REGION, START TIME: " + juce::String(region.startTime) + 
            ", END TIME: " + juce::String(region.endTime) + 
            ", isASide: " + juce::String(region.isASide ? "true" : "false"));

        const double GAP = 0.005; // 5ms gap
        
        // Clear any existing points in the region
        for (int i = curve.getNumPoints() - 1; i >= 0; --i) {
            auto time = curve.getPointTime(i).inSeconds();
            if (time >= region.startTime - GAP && time <= region.endTime + GAP) {
                curve.removePoint(i);
            }
        }

        // Add points in sequence
        if (region.startTime > 0) {
            float startValue = region.isASide ? 0.0f : 1.0f;
            DBG("Adding pre-start point at " + juce::String(region.startTime - GAP));
            curve.addPoint(tracktion::TimePosition::fromSeconds(region.startTime - GAP), 
                         startValue, 0.0f);
        }

        float mainValue = region.isASide ? 1.0f : 0.0f;
        DBG("Adding start point at " + juce::String(region.startTime));
        curve.addPoint(tracktion::TimePosition::fromSeconds(region.startTime), 
                      mainValue, 0.0f);
        
        DBG("Adding end point at " + juce::String(region.endTime));
        curve.addPoint(tracktion::TimePosition::fromSeconds(region.endTime), 
                      mainValue, 0.0f);
        
        float endValue = region.isASide ? 0.0f : 1.0f;
        DBG("Adding post-end point at " + juce::String(region.endTime + GAP));
        curve.addPoint(tracktion::TimePosition::fromSeconds(region.endTime + GAP), 
                      endValue, 0.0f);

        // Add the region to the vector and update regions from curve
        regions.push_back(region);
        updateRegionsFromCurve();

        // Verify final state
        DBG("Final curve state:");
        for (int i = 0; i < curve.getNumPoints(); ++i) {
            auto point = curve.getPoint(i);
            DBG("Point " + juce::String(i) + ": time=" + 
                juce::String(point.time.inSeconds()) + ", value=" + 
                juce::String(point.value));
        }
    }

    void removeRegion(size_t index) {
        if (index >= regions.size() || parameter == nullptr) return;

        const auto& regionToRemove = regions[index];
        auto& curve = parameter->getCurve();

        // Remove points within region time range with small buffer
        for (int i = curve.getNumPoints() - 1; i >= 0; --i) {
            auto time = curve.getPoint(i).time.inSeconds();
            if (time >= regionToRemove.startTime - 0.002 && 
                time <= regionToRemove.endTime + 0.002) {
                curve.removePoint(i);
            }
        }

        updateRegionsFromCurve();
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
    void updateRegionsFromCurve() {
        if (parameter == nullptr) return;

        auto& curve = parameter->getCurve();
        regions.clear();

        DBG("Number of curve points: " + juce::String(curve.getNumPoints()));

        bool isInRegion = false;
        double regionStart = 0.0;
        bool currentSide = false;

        for (int i = 0; i < curve.getNumPoints(); ++i) {
            auto point = curve.getPoint(i);
            double time = point.time.inSeconds();
            bool isASide = point.value > 0.5f;
            
            DBG("Point " + juce::String(i) + ": time=" + juce::String(time) + 
                ", value=" + juce::String(point.value) + 
                ", isASide=" + juce::String(isASide ? "true" : "false"));

            if (!isInRegion && isASide) {
                DBG("Starting new region at time " + juce::String(time));
                isInRegion = true;
                regionStart = time;
                currentSide = isASide;
            }
            else if (isInRegion && !isASide) {
                DBG("Ending region at time " + juce::String(time));
                regions.emplace_back(regionStart, time, currentSide);
                isInRegion = false;
            }
        }

        // Handle case where the last region extends to the end
        if (isInRegion) {
            auto lastPoint = curve.getPoint(curve.getNumPoints() - 1);
            DBG("Adding final region ending at " + juce::String(lastPoint.time.inSeconds()));
            regions.emplace_back(regionStart, lastPoint.time.inSeconds(), currentSide);
        }

        DBG("UPDATE REGIONS FROM CURVE, REGIONS SIZE: " + juce::String(regions.size()));
    }

    tracktion::engine::AutomatableParameter* parameter;
    std::vector<Region> regions;
};
#pragma once

#include "BaseEffectComponent.h"
#include "Utilities.h"

class ScratchComponent : public BaseEffectComponent
{
public:
    explicit ScratchComponent(tracktion::engine::Edit&);
    ~ScratchComponent() override;
    
    void resized() override;
    
private:
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScratchComponent)
}; 
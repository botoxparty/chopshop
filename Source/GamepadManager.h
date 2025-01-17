#pragma once

#include <JuceHeader.h>
#include <SDL2/SDL.h>

class GamepadManager : private juce::Timer
{
public:
    class Listener
    {
    public:
        virtual ~Listener() = default;
        virtual void gamepadButtonPressed(int buttonId) {}
        virtual void gamepadButtonReleased(int buttonId) {}
        virtual void gamepadAxisMoved(int axisId, float value) {}
    };

    GamepadManager();
    ~GamepadManager() override;

    void addListener(Listener* listener);
    void removeListener(Listener* listener);

private:
    void timerCallback() override;
    void handleGamepadEvents();

    SDL_GameController* gameController = nullptr;
    juce::ListenerList<Listener> listeners;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GamepadManager)
}; 
#pragma once


#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>

#include <SDL3/SDL.h>

class GamepadManager : private juce::Timer
{
public:
    class Listener
    {
    public:
        virtual ~Listener() = default;
        virtual void gamepadButtonPressed([[maybe_unused]] int buttonId) {}
        virtual void gamepadButtonReleased([[maybe_unused]] int buttonId) {}
        virtual void gamepadAxisMoved([[maybe_unused]] int axisId, [[maybe_unused]] float value) {}
        virtual void gamepadTouchpadMoved([[maybe_unused]] float x, [[maybe_unused]] float y, [[maybe_unused]] bool touched) {}
    };

    GamepadManager();
    ~GamepadManager() override;

    void addListener(Listener* listener);
    void removeListener(Listener* listener);

    static GamepadManager* getInstance()
    {
        static GamepadManager instance;
        return &instance;
    }

    bool isGamepadConnected() const { return gamepad != nullptr; }
    
    const char* getConnectedGamepadName() const 
    { 
        return gamepad ? SDL_GetGamepadName(gamepad) : ""; 
    }

private:
    void timerCallback() override;
    void handleGamepadEvents();

    SDL_Gamepad* gamepad = nullptr;
    juce::ListenerList<Listener> listeners;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GamepadManager)
}; 
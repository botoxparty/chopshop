#pragma once

#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>
#include <SDL3/SDL.h>
#include <thread>
#include <atomic>
#include <mutex>

class GamepadManager
{
public:
    class Listener
    {
    public:
        virtual ~Listener() = default;
        virtual void gamepadButtonPressed(int buttonId) = 0;
        virtual void gamepadButtonReleased(int buttonId) = 0;
        virtual void gamepadAxisMoved(int axisId, float value) = 0;
        virtual void gamepadTouchpadMoved(float x, float y, bool touched) = 0;
    };

    GamepadManager();
    ~GamepadManager();

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
    void eventLoop();

    SDL_Gamepad* gamepad = nullptr;
    juce::ListenerList<Listener> listeners;
    std::mutex listenerMutex;
    std::unique_ptr<std::thread> eventThread;
    std::atomic<bool> shouldContinue;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GamepadManager)
}; 
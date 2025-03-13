#include "GamepadManager.h"

GamepadManager::GamepadManager() : shouldContinue(true)
{
    DBG("Initializing GamepadManager...");
    
    if (SDL_Init(SDL_INIT_GAMEPAD | SDL_INIT_JOYSTICK) == false) 
    {
        DBG("SDL could not initialize! SDL Error: " << SDL_GetError());
        return;
    }
    DBG("SDL initialized successfully");

    // Log number of joysticks detected
    int count = 0;
    SDL_GetJoysticks(&count);
    DBG("Number of joysticks detected: " << count);

    // Open the first available controller
    for (int i = 0; i < count; i++) 
    {
        DBG("Checking joystick " << i);
        
        if (SDL_IsGamepad(static_cast<SDL_JoystickID>(i))) 
        {
            DBG("Joystick " << i << " is a gamepad");
            gamepad = SDL_OpenGamepad(static_cast<SDL_JoystickID>(i));
            
            if (gamepad) 
            {
                DBG("Successfully opened gamepad: " << SDL_GetGamepadName(gamepad));
                DBG("Gamepad mapping: " << SDL_GetGamepadMapping(gamepad));
                
                // Enable touchpad support for PS5 controller
                SDL_SetGamepadSensorEnabled(gamepad, SDL_SENSOR_ACCEL, true);
                SDL_SetGamepadSensorEnabled(gamepad, SDL_SENSOR_GYRO, true);
                break;
            }
            else
            {
                DBG("Failed to open controller: " << SDL_GetError());
            }
        }
        else
        {
            DBG("Joystick " << i << " is not a game controller");
        }
    }

    if (!gamepad)
    {
        DBG("No game pads found or connected");
    }

    // Start the event thread
    eventThread = std::make_unique<std::thread>(&GamepadManager::eventLoop, this);
}

GamepadManager::~GamepadManager()
{
    // Signal the event loop to stop
    shouldContinue = false;
    
    // Wait for the event thread to finish
    if (eventThread && eventThread->joinable())
    {
        eventThread->join();
    }

    if (gamepad)
    {
        SDL_CloseGamepad(gamepad);
        gamepad = nullptr;
    }
    
    SDL_Quit();
}

void GamepadManager::addListener(Listener* listener)
{
    std::lock_guard<std::mutex> lock(listenerMutex);
    listeners.add(listener);
}

void GamepadManager::removeListener(Listener* listener)
{
    std::lock_guard<std::mutex> lock(listenerMutex);
    listeners.remove(listener);
}

void GamepadManager::eventLoop()
{
    while (shouldContinue)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            std::lock_guard<std::mutex> lock(listenerMutex);
            
            switch (event.type)
            {
                case SDL_EVENT_GAMEPAD_ADDED:
                    DBG("Gamepad connected:");
                    if (!gamepad && SDL_IsGamepad(event.gdevice.which)) {
                        gamepad = SDL_OpenGamepad(event.gdevice.which);
                        if (gamepad) {
                            DBG("Successfully opened gamepad: " << SDL_GetGamepadName(gamepad));
                            DBG("Gamepad mapping: " << SDL_GetGamepadMapping(gamepad));
                            
                            SDL_SetGamepadSensorEnabled(gamepad, SDL_SENSOR_ACCEL, true);
                            SDL_SetGamepadSensorEnabled(gamepad, SDL_SENSOR_GYRO, true);

                            // Notify listeners of connection
                            dispatchToUI([this]() {
                                listeners.call([](Listener& l) { l.gamepadConnected(); });
                            });
                        }
                    }
                    break;
                    
                case SDL_EVENT_GAMEPAD_REMOVED:
                    DBG("Gamepad disconnected:");
                    if (gamepad && SDL_GetGamepadID(gamepad) == event.gdevice.which) {
                        SDL_CloseGamepad(gamepad);
                        gamepad = nullptr;
                        DBG("Active gamepad was disconnected");

                        // Notify listeners of disconnection
                        dispatchToUI([this]() {
                            listeners.call([](Listener& l) { l.gamepadDisconnected(); });
                        });
                    }
                    break;
                    
                case SDL_EVENT_GAMEPAD_TOUCHPAD_MOTION:
                    if (event.gtouchpad.touchpad == 0)  // PS5 main touchpad
                    {
                        float x = event.gtouchpad.x;
                        float y = event.gtouchpad.y;
                        dispatchToUI([this, x, y]() {
                            listeners.call([&](Listener& l) { 
                                l.gamepadTouchpadMoved(x, y, true);
                            });
                        });
                    }
                    break;

                case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
                    dispatchToUI([this, buttonId = event.gbutton.button]() {
                        listeners.call([&](Listener& l) { 
                            l.gamepadButtonPressed(buttonId); 
                        });
                    });
                    break;

                case SDL_EVENT_GAMEPAD_BUTTON_UP:
                    dispatchToUI([this, buttonId = event.gbutton.button]() {
                        listeners.call([&](Listener& l) { 
                            l.gamepadButtonReleased(buttonId);
                        });
                    });
                    break;

                case SDL_EVENT_GAMEPAD_AXIS_MOTION:
                    float value = event.gaxis.value / 32767.0f;
                    dispatchToUI([this, axisId = event.gaxis.axis, value]() {
                        listeners.call([&](Listener& l) { 
                            l.gamepadAxisMoved(axisId, value);
                        });
                    });
                    break;
            }
        }
        
        // Add a small sleep to prevent busy-waiting
        SDL_Delay(16); // roughly 60Hz
    }
} 
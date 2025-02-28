#include "GamepadManager.h"

GamepadManager::GamepadManager()
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
        
        if (SDL_IsGamepad(i)) 
        {
            DBG("Joystick " << i << " is a gamepad");
            gamepad = SDL_OpenGamepad(i);
            
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

    startTimerHz(60);
}

GamepadManager::~GamepadManager()
{
    if (gamepad)
        SDL_CloseGamepad(gamepad);
    
    SDL_Quit();
}

void GamepadManager::addListener(Listener* listener)
{
    listeners.add(listener);
}

void GamepadManager::removeListener(Listener* listener)
{
    listeners.remove(listener);
}

void GamepadManager::timerCallback()
{
    handleGamepadEvents();
}

void GamepadManager::handleGamepadEvents()
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
            case SDL_EVENT_GAMEPAD_ADDED:
                DBG("Gamepad connected:");
                // Try to open the newly connected gamepad
                if (!gamepad && SDL_IsGamepad(event.gdevice.which)) {
                    gamepad = SDL_OpenGamepad(event.gdevice.which);
                    if (gamepad) {
                        DBG("Successfully opened gamepad: " << SDL_GetGamepadName(gamepad));
                        DBG("Gamepad mapping: " << SDL_GetGamepadMapping(gamepad));
                        
                        // Enable touchpad support for PS5 controller
                        SDL_SetGamepadSensorEnabled(gamepad, SDL_SENSOR_ACCEL, true);
                        SDL_SetGamepadSensorEnabled(gamepad, SDL_SENSOR_GYRO, true);
                    }
                }
                break;
                
            case SDL_EVENT_GAMEPAD_REMOVED:
                DBG("Gamepad disconnected:");
                // If this was our active gamepad, close it
                if (gamepad && SDL_GetGamepadID(gamepad) == event.gdevice.which) {
                    SDL_CloseGamepad(gamepad);
                    gamepad = nullptr;
                    DBG("Active gamepad was disconnected");
                }
                break;
                
            case SDL_EVENT_GAMEPAD_TOUCHPAD_MOTION:
                DBG("Touchpad event received: x=" + juce::String(event.gtouchpad.x) + 
                    " y=" + juce::String(event.gtouchpad.y) + 
                    " finger=" + juce::String(event.gtouchpad.finger));
                    
                if (event.gtouchpad.touchpad == 0)  // PS5 main touchpad
                {
                    // Raw values are already in 0-1 range, no need to normalize
                    float x = event.gtouchpad.x;
                    float y = event.gtouchpad.y;
                    listeners.call([&](Listener& l) { 
                        l.gamepadTouchpadMoved(x, y, true);  // Always send touched=true for motion events
                    });
                }
                break;

            case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
                listeners.call([&](Listener& l) { l.gamepadButtonPressed(event.gbutton.button); });
                break;

            case SDL_EVENT_GAMEPAD_BUTTON_UP:
                listeners.call([&](Listener& l) { l.gamepadButtonReleased(event.gbutton.button); });
                break;

            case SDL_EVENT_GAMEPAD_AXIS_MOTION:
                float value = event.gaxis.value / 32767.0f; // Normalize to -1.0 to 1.0
                listeners.call([&](Listener& l) { l.gamepadAxisMoved(event.gaxis.axis, value); });
                break;
        }
    }
} 
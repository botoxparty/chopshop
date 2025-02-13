#include "GamepadManager.h"

GamepadManager::GamepadManager()
{
    DBG("Initializing GamepadManager...");
    
    if (SDL_Init(SDL_INIT_GAMECONTROLLER | SDL_INIT_JOYSTICK) < 0) 
    {
        DBG("SDL could not initialize! SDL Error: " << SDL_GetError());
        return;
    }
    DBG("SDL initialized successfully");

    // Log number of joysticks detected
    int numJoysticks = SDL_NumJoysticks();
    DBG("Number of joysticks detected: " << numJoysticks);

    // Open the first available controller
    for (int i = 0; i < numJoysticks; i++) 
    {
        DBG("Checking joystick " << i);
        
        if (SDL_IsGameController(i)) 
        {
            DBG("Joystick " << i << " is a game controller");
            gameController = SDL_GameControllerOpen(i);
            
            if (gameController) 
            {
                DBG("Successfully opened controller: " << SDL_GameControllerName(gameController));
                DBG("Controller mapping: " << SDL_GameControllerMapping(gameController));
                
                // Enable touchpad support for PS5 controller
                SDL_GameControllerSetSensorEnabled(gameController, SDL_SENSOR_ACCEL, SDL_TRUE);
                SDL_GameControllerSetSensorEnabled(gameController, SDL_SENSOR_GYRO, SDL_TRUE);
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

    if (!gameController)
    {
        DBG("No game controllers found or connected");
    }

    startTimerHz(60);
}

GamepadManager::~GamepadManager()
{
    if (gameController)
        SDL_GameControllerClose(gameController);
    
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
            case SDL_CONTROLLERTOUCHPADMOTION:
                DBG("Touchpad event received: x=" + juce::String(event.ctouchpad.x) + 
                    " y=" + juce::String(event.ctouchpad.y) + 
                    " finger=" + juce::String(event.ctouchpad.finger));
                    
                if (event.ctouchpad.touchpad == 0)  // PS5 main touchpad
                {
                    // Raw values are already in 0-1 range, no need to normalize
                    float x = event.ctouchpad.x;
                    float y = event.ctouchpad.y;
                    listeners.call([&](Listener& l) { 
                        l.gamepadTouchpadMoved(x, y, true);  // Always send touched=true for motion events
                    });
                }
                break;

            case SDL_CONTROLLERBUTTONDOWN:
                listeners.call([&](Listener& l) { l.gamepadButtonPressed(event.cbutton.button); });
                break;

            case SDL_CONTROLLERBUTTONUP:
                listeners.call([&](Listener& l) { l.gamepadButtonReleased(event.cbutton.button); });
                break;

            case SDL_CONTROLLERAXISMOTION:
                float value = event.caxis.value / 32767.0f; // Normalize to -1.0 to 1.0
                listeners.call([&](Listener& l) { l.gamepadAxisMoved(event.caxis.axis, value); });
                break;
        }
    }
} 
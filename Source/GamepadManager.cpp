#include "GamepadManager.h"

GamepadManager::GamepadManager()
{
    DBG("Initializing GamepadManager...");
    
    if (SDL_Init(SDL_INIT_GAMECONTROLLER) < 0) 
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
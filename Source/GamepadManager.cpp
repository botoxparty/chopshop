#include "GamepadManager.h"

GamepadManager::GamepadManager()
{
    if (SDL_Init(SDL_INIT_GAMECONTROLLER) < 0) 
    {
        DBG("SDL could not initialize! SDL Error: " << SDL_GetError());
        return;
    }

    // Open the first available controller
    for (int i = 0; i < SDL_NumJoysticks(); i++) 
    {
        if (SDL_IsGameController(i)) 
        {
            gameController = SDL_GameControllerOpen(i);
            if (gameController) 
            {
                DBG("Found gamepad: " << SDL_GameControllerName(gameController));
                break;
            }
        }
    }

    startTimerHz(60); // Poll for controller events at 60Hz
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
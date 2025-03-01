# Debugging Guide for ScratchComponent Crash

## Setup

1. The VS Code debugging configuration has been set up for you in `.vscode/launch.json` and `.vscode/tasks.json`.

## Running with the Debugger

1. Open VS Code in this project directory
2. Set breakpoints in the following locations:
   - In `ScratchComponent.cpp` constructor
   - In `MainComponent.cpp` where `addAndMakeVisible(*scratchComponent)` is called (around line 191)
   - In `BaseEffectComponent.cpp` constructor

3. Press F5 or click the "Run and Debug" button in VS Code's left sidebar, then select "Debug ChopShop"

## Debugging the Crash

The crash is likely occurring because:
1. The `plugin` member variable in `BaseEffectComponent` was not being initialized in the `ScratchComponent` constructor
2. We've fixed this by adding `plugin = createPlugin("volume");` to the constructor

### Key Areas to Watch

When debugging, pay attention to:

1. **Constructor Initialization**: Make sure all member variables are properly initialized
2. **Callback Functions**: The `getCurrentTempo` and `getBaseTempo` callbacks should be properly set up
3. **Plugin Creation**: Check if `createPlugin("volume")` succeeds and returns a valid plugin

### If Still Crashing

If the app still crashes after our fixes:

1. Check if `screwComponent` is properly initialized before being used in the callbacks
2. Verify that `BaseEffectComponent::createPlugin` is working correctly
3. Look for any null pointer dereferences in the `paint()` or `resized()` methods

## Additional Debugging Tips

- Use LLDB commands in the Debug Console:
  - `bt` - Show backtrace
  - `frame variable` - Show variables in current stack frame
  - `po object` - Print object description

- Add logging statements with `DBG("Message")` to track execution flow 
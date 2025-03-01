#!/bin/bash

# Build the application in debug mode
echo "Building ChopShop in Debug mode..."
cd build
cmake --build . --config Debug

# Check if build was successful
if [ $? -ne 0 ]; then
    echo "Build failed. Exiting."
    exit 1
fi

echo "Build successful."

# Path to the executable
APP_PATH="./ChopShop_artefacts/Debug/ChopShop Beta.app/Contents/MacOS/ChopShop Beta"

# Check if the executable exists
if [ ! -f "$APP_PATH" ]; then
    echo "Error: Executable not found at $APP_PATH"
    exit 1
fi

echo "Starting application with LLDB..."
echo "Type 'run' to start the application"
echo "When it crashes, type 'bt' to see the backtrace"

# Run with LLDB
lldb "$APP_PATH" 
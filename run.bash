#!/bin/bash

BUILD_DIR="build"
EXECUTABLE="sdl_app"

# Function to build and run the app
run_app() {
    echo "--- Building ---"
    cmake -S . -B $BUILD_DIR -G Ninja
    ninja -C $BUILD_DIR
    
    if [ $? -eq 0 ]; then
        echo "--- Running (Press 'R' in this terminal to restart) ---"
        # Run in background so the script can keep listening for 'R'
        ./$BUILD_DIR/$EXECUTABLE &
        APP_PID=$!
    else
        echo "Build failed! Fix errors and press 'R' to try again."
        APP_PID=""
    fi
}

# Initial run
run_app

while true; do
    # Listen for a single keypress (-n 1) without requiring Enter (-s for silent)
    read -n 1 -s key
    
    if [[ $key == "r" || $key == "R" ]]; then
        echo "Restarting..."
        
        # Kill the previous process if it's still running
        if [ -n "$APP_PID" ] && ps -p $APP_PID > /dev/null; then
            kill $APP_PID
            wait $APP_PID 2>/dev/null
        fi
        
        run_app
    elif [[ $key == "q" || $key == "Q" ]]; then
        echo "Exiting..."
        if [ -n "$APP_PID" ]; then kill $APP_PID; fi
        exit 0
    fi
done
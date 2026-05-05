#!/bin/bash

BUILD_DIR="build"
EXECUTABLE="sdl_app"

run_app() {
    echo "--- Building ---"
    cmake -S . -B $BUILD_DIR -G Ninja
    ninja -C $BUILD_DIR
    
    if [ $? -eq 0 ]; then
        echo "--- Running (Press 'R' to restart, F to format, Q to quit) ---"
        ./$BUILD_DIR/$EXECUTABLE &
        APP_PID=$!
    else
        echo "Build failed! Fix errors and press 'R' to try again."
        APP_PID=""
    fi
}

run_app

while true; do
    read -n 1 -s key

    if [[ $key == "r" || $key == "R" ]]; then
        echo "Restarting..."
        
        if [ -n "$APP_PID" ] && ps -p $APP_PID > /dev/null; then
            kill $APP_PID
            wait $APP_PID 2>/dev/null
        fi
        
        run_app

    elif [[ $key == "f" || $key == "F" ]]; then
        echo "Formatting..."
        astyle main.cpp

    elif [[ $key == "q" || $key == "Q" ]]; then
        echo "Exiting..."
        if [ -n "$APP_PID" ]; then kill $APP_PID; fi
        exit 0
    fi
done
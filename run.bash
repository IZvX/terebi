#!/bin/bash

BUILD_DIR="build"
EXECUTABLE="sdl_app"

cleanup_stale_locks() {
    # Remove child wayland locks (preserving host wayland-0 if running)
    find /run/user/$UID/ -maxdepth 1 -name "wayland-[1-9]*.lock" -delete 2>/dev/null
    find /run/user/$UID/ -maxdepth 1 -name "wayland-[1-9]*" -type s -delete 2>/dev/null
    find /run/user/$UID/ -maxdepth 1 -name "*-awww-daemon.sock" -delete 2>/dev/null
}

run_app() {
    echo "--- Building ---"
    cmake -S . -B $BUILD_DIR -G Ninja
    ninja -C $BUILD_DIR
    
    if [ $? -eq 0 ]; then
        #cleanup_stale_locks
        echo "--- Running (Press 'R' to restart, F to format, Q to quit) ---"
        
        # Run compositor in background
        ./$BUILD_DIR/$EXECUTABLE
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
            kill -TERM $APP_PID 2>/dev/null
            wait $APP_PID 2>/dev/null
        fi
        
        pkill -f "sdl_app --ui" 2>/dev/null
        pkill -f "awww-daemon" 2>/dev/null
        
        run_app

    elif [[ $key == "f" || $key == "F" ]]; then
        echo "Formatting..."
        astyle main.cpp

    elif [[ $key == "q" || $key == "Q" ]]; then
        echo "Exiting..."
        if [ -n "$APP_PID" ]; then 
            kill -TERM $APP_PID 2>/dev/null
        fi
        pkill -f "sdl_app --ui" 2>/dev/null
        pkill -f "awww-daemon" 2>/dev/null
        cleanup_stale_locks
        exit 0
    fi
done

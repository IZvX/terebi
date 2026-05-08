#!/bin/bash

BUILD_DIR="build"
EXECUTABLE="sdl_app"
RENDERER=""
DONT_RUN=0
RECOMPILE=0

while [[ $# -gt 0 ]]; do
    case "$1" in
        -r|--renderer)
            if [[ -z "$2" ]]; then
                echo "Error: $1 requires a renderer name."
                exit 1
            fi
            RENDERER="$2"
            shift 2
            ;;
        --dont-run)
            DONT_RUN=1
            shift
            ;;
        --recompile)
            RECOMPILE=1
            shift
            ;;
        *)
            echo "Unknown option: $1"
            echo "Usage: bash run.bash [-r|--renderer <name>] [--dont-run] [--recompile]"
            exit 1
            ;;
    esac
done

configure_if_needed() {
    if [[ ! -d "$BUILD_DIR" ]]; then
        echo "--- Configuring (first time) ---"
        cmake -S . -B "$BUILD_DIR" -G Ninja
    fi
}

full_rebuild() {
    echo "--- Full Recompile ---"
    rm -rf "$BUILD_DIR"
    cmake -S . -B "$BUILD_DIR" -G Ninja
    ninja -C "$BUILD_DIR"
}

incremental_build() {
    echo "--- Incremental Build ---"
    configure_if_needed
    ninja -C "$BUILD_DIR"
}

run_app() {
    if [[ "$RECOMPILE" -eq 1 ]]; then
        full_rebuild
    else
        incremental_build
    fi

    if [ $? -eq 0 ]; then
        if [[ "$DONT_RUN" -eq 1 ]]; then
            echo "--- Build complete (--dont-run set) ---"
            APP_PID=""
            return
        fi

        echo "--- Running ---"

        if [[ -n "$RENDERER" ]]; then
            "./$BUILD_DIR/$EXECUTABLE" --renderer "$RENDERER" &
        else
            "./$BUILD_DIR/$EXECUTABLE" &
        fi

        APP_PID=$!
    else
        echo "Build failed!"
        APP_PID=""
    fi
}

run_app

if [[ "$DONT_RUN" -eq 1 ]]; then
    exit 0
fi

while true; do
    read -n 1 -s key

    if [[ $key == "r" || $key == "R" ]]; then
        echo "Restarting..."

        if [[ -n "$APP_PID" ]] && ps -p $APP_PID > /dev/null; then
            kill $APP_PID
            wait $APP_PID 2>/dev/null
        fi

        run_app

    elif [[ $key == "f" || $key == "F" ]]; then
        echo "Formatting..."
        astyle main.cpp

    elif [[ $key == "q" || $key == "Q" ]]; then
        echo "Exiting..."
        if [[ -n "$APP_PID" ]]; then kill $APP_PID; fi
        exit 0
    fi
done
#!/bin/bash

echo "--- Cleaning up old logs and binaries ---"
rm -f *.log pids.txt
make clean

echo "--- Compiling Project ---"
make

# Check if make succeeded
if [ $? -ne 0 ]; then
    echo "Error: Compilation failed."
    exit 1
fi

# Ensure ALL binaries are executable (Added watchdog here)
chmod +x master blackboard dynamics obstacles targets map_window control watchdog

echo "--- Starting Drone System ---"
./master

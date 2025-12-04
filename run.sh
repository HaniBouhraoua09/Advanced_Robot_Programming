#!/bin/bash

# 1. Clean previous builds (Optional, ensures fresh start)
make clean

# 2. Compile everything using the new Makefile
make

# 3. Check if compilation succeeded
if [ $? -eq 0 ]; then
    echo "Compilation successful. Starting Drone Simulator..."
    # 4. Run the master process
    ./master
else
    echo "Compilation failed."
fi

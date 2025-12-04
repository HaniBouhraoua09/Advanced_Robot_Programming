# Drone Simulator Project - Assignment 1
**Student:** 8314923  
**Date:** December 4, 2024

## 1. Sketch of the Architecture
[cite_start]The system implements a concurrent, multi-process **Blackboard Architecture**[cite: 31, 49].  
The `Blackboard` process acts as the central repository. All other processes communicate only with the Blackboard using **unnamed pipes**, ensuring modularity and decoupling.

[Image of blackboard architecture diagram]

**Data Flow:**
1. **Input:** `Control Window` captures keystrokes → sends force data to the Blackboard.  
2. **Environment:** `Obstacles` & `Targets` generate coordinates → send to the Blackboard.  
3. **Physics:** `Dynamics` reads state from the Blackboard → computes new state → updates Blackboard.  
4. **Output:** `Map Window` reads Blackboard state → renders the simulation.

**Refactoring Attempt:**  
A partial implementation was attempted for **runtime parameter configuration**, enabling dynamic safety threshold updates without recompiling.  
While not fully integrated due to time and technical constraints, the rest of the system (control, monitoring, collision avoidance) works correctly.  
This feature is planned for a future release.

---

## 2. Active Components Definition

### A. Master (`master.c`)
* **Role:** System orchestrator.  
* **Function:** Creates 7 pipes, forks all processes, manages PIDs, and ensures safe termination.  
* **Primitives:** `pipe()`, `fork()`, `exec()`, `kill()`, `waitpid()`.

### B. Blackboard Server (`blackboard.c`)
* **Role:** Central Hub.  
* **Function:** Routes data between agents using non-blocking multiplexing.  
* [cite_start]**Primitives:** `select()`, `read()`, `write()`[cite: 137].  
* **Logic:** FD-set scanning + event-driven routing.

### C. Drone Dynamics (`dynamics.c`)
* **Role:** Physics engine.  
* **Function:** Maintains drone state: position, velocity, forces.  
* **Algorithms:**  
  1. [cite_start]**Motion Integration:** Euler method[cite: 74, 75].  
  2. [cite_start]**Repulsion Model:** Latombe/Khatib potential fields[cite: 85].  
  3. [cite_start]**Virtual Key Mapping:** Projects repulsion to directional commands[cite: 94].

### D. Map Window (`map_window.c`)
* **Role:** Renderer.  
* **Function:** Draws drone, targets, obstacles. Handles terminal resizing.  
* [cite_start]**Primitives:** `ncurses` (colors, windows)[cite: 133].

### E. Control Window (`control.c`)
* **Role:** Input handler.  
* **Function:** Reads raw keyboard inputs and maps them to force vectors.  
* **Primitives:** `ncurses` (non-blocking).

### F. Targets & Obstacles (`targets.c`, `obstacles.c`)
* **Role:** Random environment generators.  
* **Function:** Periodically send coordinate updates.  
* **Logic:** Random generation with minimum-distance constraints.

---

## 3. Directory Structure & Files

```text
/DRONE_ROOT/
│
├── Makefile                  # Build script
├── run.sh                    # Execution script
├── README.md                 # Project documentation
│
├── configuration/            # Configuration Directory
│   └── params.txt            # Physics & Window parameters (Editable)
│
└── src_code/                 # Source Code Directory
    ├── master.c              # Main process
    ├── blackboard.c          # Server
    ├── dynamics.c            # Physics logic
    ├── map_window.c          # Visualization
    ├── control.c             # Input handling
    ├── obstacles.c           # Generator
    ├── targets.c             # Generator
    └── drone.h               # Shared header (Structs & Includes)
    ``` 
    
## 4. Operational Instructions

### Controls
Q / W / E : Up-Left / Up / Up-Right  
A / S / D : Left / BRAKE / Right  
Z / X / C : Down-Left / Down / Down-Right  
P         : Pause / Quit Simulation

### Visual Legend
+   → Drone (white)  
1–9 → Targets (green, collected in order)  
O   → Obstacles (yellow, repulsive)

### Real-Time Parameters (Editable at Runtime)

Configuration file:

configuration/params.txt

Parameters:  
- M — Drone mass  
- K — Viscous friction  
- T — Time-step  

These can be edited while the system is running.

---

## 5. Installation and Running

### Prerequisites
Install ncurses:

sudo apt-get install libncurses5-dev libncursesw5-dev

### Compilation
From project root:

make

This compiles all files inside src_code/.

### Execution

Using script:

./run.sh

Or manual:

./master

Stop simulation using P in the Control Window or by closing the terminal.


# Power Grid Simulator

**Power Grid Simulator** is an interactive, real-time C++ application built with Raylib that simulates a highly dynamic, hierarchical power distribution network. It features advanced load balancing, real-time demand propagation, automated alternative path routing, and interactive visual analytics.

The project models a three-tier infrastructure: **Power Plants** → **Substations** → **Consumers**, providing deep insights into grid stability, bottleneck mitigation, and capacity optimization.

---

## 🌟 Key Features

### 1. Robust Network Simulation & Architecture
* **Three-Tier Hierarchy:** Generates random, spatially aware grids connecting central Power Plants to intermediate Substations, which then serve distinct consumer nodes (Residential, Hospital, Industrial, Institute).
* **Modular Codebase:** Cleanly refactored into modular components (`graph.hpp`, `redistribution.hpp`, `gui.hpp`) to cleanly separate rendering loops from complex graph theory calculations.
* **Memory Safe Lifecycle:** Graph structures securely manage deep memory allocations, ensuring smooth resets and zero memory leaks during prolonged sessions.

### 2. Advanced Load Balancing & Fault Tolerance
* **Real-Time Overload Detection:** Automatically detects capacity thresholds. If an edge exceeds 75% load capacity, it is instantly flagged as overloaded and visually pulsates.
* **Widest-Path Routing (Modified Dijkstra):** When resolving an overload, the system employs a custom pathfinding algorithm to route power through the alternative network path with the highest available bottleneck capacity, ensuring long-term grid stability.
* **Proportional Precision Throttling:** If no stable alternative route exists, the simulator gracefully falls back to "throttling." It executes a proportional, localized demand reduction exclusively on the consumers drawing from the overloaded substation.
* **Recursive Capacity Upgrades:** Substations can be manually upgraded, which intelligently propagates new limit ceilings up the supply chain to upstream power plants.

### 3. Interactive Fullscreen UI & Analytics
* **Immersive Visuals:** The visualizer automatically detects the user's resolution and opens in borderless fullscreen. It supports smooth camera panning and dynamic zoom-based Level of Detail (LOD).
* **Live Status HUD:** A persistent bottom status bar tracks elapsed simulation time, the current count of overloaded edges, and the number of actively throttled substations. 
* **Inspector Panel:** Left-clicking any node or edge instantly reveals a detailed panel displaying real-time properties (e.g., current load, max capacity, consumer demand, overload status).
* **Visual Color Coding & Legend:** Instantly interpret the grid's health with color-coded alerts:
  * **Pulsing Red Arrows:** Overloaded edge requiring immediate attention.
  * **Thick Green Arrows:** Successfully applied alternative reroute path.
  * **Solid Black Nodes:** Overloaded node.
  * **White Outlined Nodes:** Substation experiencing active throttling.

---

## 🛠️ Tech Stack

* **Language:** C++17 (or newer)
* **Graphics Library:** [Raylib](https://www.raylib.com/)
* **Build System:** CMake
* **Algorithms:** Graph Theory, Modified Dijkstra's Algorithm, Proportional Load Balancing

---

## 🎮 Controls

| Key / Action | Result |
|---|---|
| **`I`** | **Increase Demand:** Randomly injects a significant demand spike at a consumer node, instantly highlighting any resulting infrastructure overloads. |
| **`O`** | **Optimize / Fix Overload:** Triggers the pathfinding logic to either seamlessly reroute power along the widest-path, or execute precision throttling. |
| **`U`** | **Upgrade Node:** Upgrades the hardware limits of a currently selected (and overloaded) substation. |
| **`C`** | **Clear Highlights:** Resets all visual indicators, clearing glowing paths and resetting throttled node highlights. |
| **`BACKSPACE`** | **Reset Simulation:** Safely deallocates the entire grid memory and returns to the Main Setup Menu. |
| **Mouse Wheel** | Zoom Camera In / Out |
| **Left Drag** | Pan Camera |
| **Left Click** | Select Node or Edge (Opens Inspector) |

---

## 🚀 Build Instructions (Linux / macOS)

This project uses **CMake** for seamless building. Ensure you have `cmake`, `make`, and a modern C++ compiler installed, along with the `raylib` dependencies on your system.

### 1. Install Dependencies (Ubuntu/Debian example)
```bash
sudo apt update
sudo apt install build-essential cmake libasound2-dev mesa-common-dev libx11-dev libxrandr-dev libxi-dev xorg-dev libgl1-mesa-dev libglu1-mesa-dev
```

### 2. Clone and Build
```bash
git clone <your-repository-link>
cd PowerGridSimulator

# Create a build directory
mkdir build
cd build

# Generate build files and compile
cmake ..
make
```

### 3. Run the Simulator
```bash
./PowerGridSimulator
```

---

## 🖥️ Input Configuration

Upon starting the executable, you will be greeted with a configuration screen. You can customize the scale of the simulation by defining:
- Total **Power Plants** (Nodes that supply infinite power)
- Total **Substations** (Intermediary distribution hubs with hardware limits)
- Total **Consumers** (The end-users distributed among substations)
- **Base Demand Weights** for distinct zoning types (Residential, Hospital, Industrial, Institute)

Once parameters are set, click **START SIMULATION** to procedurally generate your unique power grid!

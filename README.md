# Power Grid Visualizer

Power Grid Visualizer is a C++ application built with Raylib that simulates a hierarchical power distribution network with real-time load balancing, demand propagation, throttling, and capacity optimization. It models a three-tier system of power plants, substations, and consumers, and includes interactive visualization plus console logging for analysis. :contentReference[oaicite:0]{index=0} :contentReference[oaicite:1]{index=1}

## Features

- Three-tier network topology: Power Plants → Substations → Consumers
- Widest-path based rerouting for overload handling
- Real-time demand propagation and load balancing
- Proportional throttling when capacity is insufficient
- Interactive visualization with zoom-based LOD rendering
- Node/edge selection with an inspector panel
- Console output for debugging and simulation tracing :contentReference[oaicite:2]{index=2} :contentReference[oaicite:3]{index=3}
## Tech Stack

- **Language:** C++
- **Graphics Library:** Raylib
- **Compiler:** g++
- **Environment:** MSYS2 / MinGW64 :contentReference[oaicite:4]{index=4}

## Core Components

- `Vertex_A` — infrastructure nodes (`power_plant`, `substation`)
- `Vertex_B` — consumer nodes (`residential`, `hospital`, `industrial`, `institute`)
- `Edge` — network connections with load and capacity tracking :contentReference[oaicite:5]{index=5}

## Controls

| Key / Action | Result |
|---|---|
| `I` | Increase random demand |
| `O` | Fix overload / run load balancing |
| `U` | Upgrade selected node |
| `C` | Clear highlights |
| Mouse Wheel | Zoom in/out |
| Left Drag | Pan camera |
| Left Click | Select node/edge |
| `BACKSPACE` | Return to menu / clear graph | :contentReference[oaicite:6]{index=6}

## Build Requirements

- g++ compiler
- MSYS2 development environment
- Raylib graphics library
- Standard C++ libraries :contentReference[oaicite:7]{index=7}

## Input Configuration

At startup, the simulation asks for:
- Number of power plants
- Number of substations
- Total consumers
- Base demand per consumer type :contentReference[oaicite:8]{index=8}

## How to Run

### 1. Install dependencies
Install **MSYS2**, then open the **MSYS2 MinGW64** terminal. Install the compiler and Raylib package there.

### 2. Clone the repository
```bash
git clone <your-repository-link>
cd <project-folder>

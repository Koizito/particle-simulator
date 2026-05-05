# Particle Simulator

[![C++17](https://img.shields.io/badge/C%2B%2B-17-blue)](https://en.cppreference.com/w/cpp/17)
[![CMake](https://img.shields.io/badge/CMake-3.16+-green)](https://cmake.org)
[![vcpkg](https://img.shields.io/badge/vcpkg-manifest%20mode-brightgreen)](https://vcpkg.io)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

A real‑time classical particle simulator with a multi‑threaded, priority‑aware network messaging system.
It runs a deterministic physics simulation and streams snapshots to a connected WebSocket client using a high‑priority /
normal‑priority message queue, ensuring critical data is never delayed.

---

## Features

- **Classical particle simulation** – Newtonian motion (position, velocity, gravity, world boundaries) of point masses
- **Priority‑based messaging** – high‑priority snapshots are sent even when the normal queue is paused
- **WebSocket server** – remote control (`start`, `stop`, `create_particles`, etc.) and live snapshot streaming
- **Multi‑threaded architecture**
    - `SimulationThread` – runs the physics and produces snapshots
    - `SendThread` – consumes messages (high‑priority first) and sends them over the WebSocket
    - Main/network thread – handles incoming WebSocket control messages
- **Thread‑safe queue** – custom `MessagingQueue` with fine‑grained condition variables and correct wake‑up predicates
- **Deterministic testing** – physics golden‑master tests using Catch2, including a RAII thread joiner for safe
  concurrency tests
- **Cross‑platform** – builds on Linux (GCC/Clang) and Windows (MSVC) with CMake and vcpkg manifest mode

---

## Quick Start

### Prerequisites

- **CMake** ≥ 3.16
- **C++17 compiler** (GCC 9+, Clang 10+, MSVC 2019+)
- **Git**
- **vcpkg** – any recent clone is fine (we use manifest mode, no global integration required)

### Clone & Build

```bash
# Clone the project
git clone https://github.com/yourusername/particle-simulator.git
cd particle-simulator

# Configure (use your vcpkg toolchain)
cmake -B build -S . \
    -DCMAKE_TOOLCHAIN_FILE=/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake \
    -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build build
```

On Windows (using PowerShell or Command Prompt with MSVC), the toolchain path uses forward slashes:

```powershell
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=C:/dev/vcpkg/scripts/buildsystems/vcpkg.cmake -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

The first configure will automatically install all dependencies (ixwebsocket, nlohmann/json, spdlog, Catch2) via the
manifest.

### Run

```bash
./build/simulator    # or build\Release\simulator.exe on Windows
```

The server starts listening on `http://0.0.0.0:8080`. Connect any WebSocket client to begin.

---

## Usage – WebSocket Control

The simulator accepts JSON messages over the WebSocket. Here are the supported commands:

| Command              | Description                                     | Example payload                                                         |
|----------------------|-------------------------------------------------|-------------------------------------------------------------------------|
| `start`              | Start the simulation and send thread            | `{"type":"start"}`                                                      |
| `stop`               | Pause the simulation and send thread            | `{"type":"stop"}`                                                       |
| `exit`               | Graceful shutdown of the application            | `{"type":"exit"}`                                                       |
| `create_particles`   | Add particles to the world                      | `{"type":"create_particles", "particles":[{"mass":1.0, "x":0.0, ...}]}` |
| `delete_particles`   | Remove particles by ID                          | `{"type":"delete_particles", "particle_ids":[1,2]}`                     |
| `update_world`       | Change world parameters (gravity, bounds, etc.) | `{"type":"update_world", "world":{...}}`                                |
| `reset_world`        | Reset to an empty world                         | `{"type":"reset_world"}`                                                |
| `get_world_snapshot` | Request a one‑time high‑priority snapshot       | `{"type":"get_world_snapshot"}`                                         |

When the simulation is running, snapshots are streamed automatically: first a text frame with JSON metadata (
`{"type":"particles","count":2}`), then a binary frame containing all particle data (id, mass, position, velocity – no
padding, little‑endian `int` + `float` arrays).

---

## Testing

The project includes a comprehensive unit test suite built with **Catch2**. Tests cover:

- `MessagingQueue` blocking, priority, and shutdown behaviour (concurrent tests)
- Simulation deterministic output (golden‑master tests)

### Build & Run Tests

```bash
cmake --build build
ctest --test-dir build
```

All tests also run directly:

```bash
./build/unit_tests                 # all tests
./build/unit_tests "[queue]"       # only queue tests
./build/unit_tests "[simulation]"  # only simulation tests
```

### Concurrency testing

Tests that involve threads use a lightweight `ThreadJoiner` helper (RAII) to ensure threads are always joined even if an
assertion fails, preventing `std::terminate` and guaranteeing full diagnostic output.

---

## Project Structure

```
particle-simulator/
├── main.cpp                    # Application entry point
├── CMakeLists.txt              # CMake build definition
├── vcpkg.json                  # vcpkg manifest (dependencies)
├── core/                       # Simulation core
│   ├── World.cpp/hpp
│   ├── Particle.cpp/hpp
│   ├── Force.cpp/hpp
│   └── OutgoingMessage.cpp/hpp # Combined text+binary message
├── networking/                 # WebSocket server & message handling
│   ├── NetworkingHandler.cpp/hpp
│   └── MessageHandler.cpp/hpp
├── app/                        # Application context & messaging
│   ├── AppContext.cpp/hpp
│   └── MessagingQueue.cpp/hpp
├── threads/                    # Concurrent workers
│   ├── BaseThread.cpp/hpp
│   ├── SimulationThread.cpp/hpp
│   └── SendThread.cpp/hpp
└── tests/                      # Unit tests
    ├── test_messaging_queue.cpp
    ├── TestSimulationThread.cpp
    ├── TestHelpers.hpp         # ThreadJoiner, etc.
    └── golden_snapshot_5steps.json
```

---

## Architecture & Concurrency Design

### Thread Model

```
               Main Thread
                    │
                    │ creates
                    ▼
         ┌──────────────────────┐
         │      AppContext      │
         │  (owns the queue,    │
         │   flags, world, etc.)│
         └──┬───────────────┬───┘
            │               │
            │ reference     │ reference
            ▼               ▼
    ┌──────────────┐  ┌──────────────┐
    │ Simulation   │  │    Send      │
    │   Thread     │  │   Thread     │
    └──────┬───────┘  └──────┬───────┘
           │                 │
           │   access via    │   access via
           │   AppContext    │   AppContext
           ▼                 ▼
    ┌────────────────────────────────┐
    │        MessagingQueue          │
    │  (high‑priority + normal)      │
    └────────────────────────────────┘
```

**Flow:**

1. Simulation thread waits for space in the queue (`waitForSpaceInNormalQueue`). When space is available and the run
   flag is set, it steps the physics, creates a snapshot (text metadata + binary), and pushes it to the normal queue. If
   the queue is full, it blocks.
2. Send thread waits for data (`waitForDataInQueues`). It always checks the high‑priority queue first; if a
   high‑priority message is present, it sends it regardless of the run flag. Normal messages are sent only when the run
   flag is on.
3. External commands (e.g., `get_world_snapshot`) push to the high‑priority queue, waking the send thread immediately
   even if the simulation is paused. This guarantees that snapshots requested by the user are delivered without waiting
   for the simulation to resume.
4. On shutdown (`exit` command or signal), `shouldExit` is set and all condition variables are notified, causing every
   waiting thread to wake and exit cleanly.

### Condition Variable Predicates

The `MessagingQueue` uses two separate condition variables:

- `dataAvailableCV` – wakes the send thread when there is a high‑priority message, or a normal message **and** the run
  flag is true, or exit is requested.
- `spaceAvailableCV` – wakes the simulation thread when the normal queue has space **and** the run flag is true, or exit
  is requested.

Predicates always include `shouldExit` so that threads never get stuck during shutdown. Notifications are always
performed while holding the associated mutex, preventing missed wake‑ups.

---

## Dependencies

- [ixwebsocket](https://github.com/machinezone/IXWebSocket) – WebSocket server/client library (with OpenSSL)
- [nlohmann/json](https://github.com/nlohmann/json) – JSON parsing
- [spdlog](https://github.com/gabime/spdlog) – Logging
- [Catch2](https://github.com/catchorg/Catch2) – Unit testing framework (test only)

All are managed automatically via vcpkg manifest mode.

---

## License

This project is licensed under the MIT License. See [LICENSE](LICENSE) for details.

---

## Acknowledgements

Built as a personal project to practice advanced C++ concurrency patterns, real‑time simulation, and cross‑platform
build systems. Special thanks to the open‑source community for the excellent libraries that made this possible.
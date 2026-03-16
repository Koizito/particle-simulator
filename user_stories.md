# Particle Physics Simulator – Product Backlog

## Product Vision

A **2D particle physics simulation engine** that advances a simulation world in time and exposes its state so a web-based frontend can visualize it.

The engine is responsible only for **simulation logic**, not rendering.

---

# Epic 1 — Core Simulation Engine

Goal: Create the minimal system capable of running a simulation step.

## User Story 1.1 — Create the simulation world - X

**As a** simulation engine  
**I want** a world object that contains all particles  
**So that** the simulation state can be managed in one place  

### Acceptance Criteria

- A world object exists
- It stores multiple particles
- Particles can be added to the world
- The world persists particle state between simulation steps

---

## User Story 1.2 — Represent particles - X

**As a** simulation engine  
**I want** particles with physical properties  
**So that** they can move and interact  

### Acceptance Criteria

Particles must have at minimum:

- position
- velocity
- mass

The system must allow creating multiple particles with different values.

---

## User Story 1.3 — Implement simulation step - X

**As a** simulation engine  
**I want** to advance the simulation in time  
**So that** particles move realistically  

### Acceptance Criteria

- The world exposes a function like `step(dt)`
- Each step updates particle positions
- Movement depends on velocity
- The step updates all particles in the world

---

# Epic 2 — Forces

Goal: Allow particles to accelerate.

---

## User Story 2.1 — Add force accumulation - X

**As a** particle  
**I want** forces applied to me  
**So that** my velocity can change over time  

### Acceptance Criteria

- Forces influence particle velocity
- The simulation step applies acceleration
- Multiple forces can affect a particle

---

## User Story 2.2 — Implement gravity - X

**As a** simulation user  
**I want** a gravity force  
**So that** particles accelerate downward  

### Acceptance Criteria

- A global gravity force exists
- Gravity affects all particles
- Particles accelerate each simulation step

---

# Epic 3 — World Boundaries

Goal: Keep particles inside the simulation area.

---

## User Story 3.1 — Define world bounds - X

**As a** simulation engine  
**I want** boundaries for the world  
**So that** particles stay within the simulation space  

### Acceptance Criteria

- The world has width and height
- Particle positions are validated against boundaries

---

## User Story 3.2 — Implement wall collisions - X

**As a** particle  
**I want** to bounce when hitting walls  
**So that** I stay within the simulation  

### Acceptance Criteria

- Particles cannot leave the world
- Velocity changes when hitting a boundary
- Bounce behavior preserves motion realistically

---

# Epic 4 — Simulation State Export

Goal: Make the simulation visible to the frontend.

---

## User Story 4.1 — Expose world state

**As a** frontend application  
**I want** access to particle positions  
**So that** I can render them  

### Acceptance Criteria

The simulation can output:

- all particles
- their positions
- their velocities

The output format should be easy to serialize.

---

## User Story 4.2 — Provide a simulation snapshot

**As a** frontend  
**I want** the current simulation state  
**So that** I can update the visualization  

### Acceptance Criteria

- The system can produce a snapshot of the world
- Snapshot includes all particle data
- Snapshot can be requested repeatedly

---

# Epic 5 — Simulation Control

Goal: Allow external systems to control the simulation.

---

## User Story 5.1 — Spawn particles

**As a** frontend user  
**I want** to create particles dynamically  
**So that** I can interact with the simulation  

### Acceptance Criteria

- New particles can be added during runtime
- The simulation includes them in future steps

---

## User Story 5.2 — Control simulation time

**As a** simulation user  
**I want** control over the timestep  
**So that** the simulation speed can be adjusted  

### Acceptance Criteria

- The simulation accepts different `dt` values
- Larger `dt` produces faster simulation progression

---

# Epic 6 — Advanced Physics (Stretch Goals)

These are not required for the MVP.

---

## User Story 6.1 — Particle collisions

Particles detect and respond to collisions with other particles.

---

## User Story 6.2 — Drag / air resistance

Particles gradually lose velocity over time.

---

## User Story 6.3 — Particle emitters

The system can generate particles automatically.

---

## User Story 6.4 — Springs

Particles can be connected with spring forces.

---

# MVP Definition

The Minimum Viable Product is complete when:

- Particles exist
- Particles move
- Gravity affects them
- They bounce off world boundaries
- The simulation state can be exported
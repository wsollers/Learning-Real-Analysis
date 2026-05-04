# Proposed Architecture Diagram

This is the proposed target architecture for the next major refactor. It is intentionally not thread-oriented yet; threading should be a later refactor once the simulation/service/render boundaries are clean.

## Core Principle

```text
Engine owns lifecycle and services.
Simulation owns math state and recalculation.
RenderService owns renderer-neutral view submission.
Renderer owns Vulkan.
Panels and hotkeys are registered capabilities, not scene-owned side effects.
```

## Target System Layers

```mermaid
%%{init: {'theme': 'dark'}}%%
flowchart TB
    Main["app/main.cpp"] --> Engine["Engine"]

    Engine --> Services["EngineServices / SimulationHost"]
    Engine --> SimLifecycle["Simulation lifecycle"]
    Engine --> Renderer["Vulkan Renderer"]

    Services --> PanelService["PanelService"]
    Services --> HotkeyService["HotkeyService"]
    Services --> RenderService["RenderService"]
    Services --> MemoryService["Memory / Buffer / Arena Service"]
    Services --> Clock["SimulationClock / TickSource"]
    Services --> Config["ConfigService"]

    SimLifecycle --> ISim["ISimulation"]
    ISim --> Sim1["SimulationSurfaceGaussian"]
    ISim --> Sim2["SimulationSineRationalAnalysis"]
    ISim --> Sim3["SimulationMultiWellCentroid"]
    ISim --> Sim4["SimulationWavePredatorPrey"]

    ISim --> Context["SimulationContext"]
    Context --> Surface["ISurface"]
    Context --> Particles["Particle Store"]
    Context --> History["History / Ring Buffers"]
    Context --> Caches["Math Caches / Dirty Flags"]
    Context --> TickState["Tick / Time State"]
    Context --> Deformation["Deformation State"]
    Context --> MemoryService

    ISim --> RenderPackets["RenderPacket / GeometryBatch"]
    RenderPackets --> RenderService
    RenderService --> Renderer
```

## Engine Services

The engine is the composition root. Simulations should not depend on concrete `Engine`; they receive a narrowed host/services interface.

```mermaid
%%{init: {'theme': 'dark'}}%%
classDiagram
    class Engine {
        +start()
        +stop()
        +run_frame()
        -EngineServices services
        -ISimulation active_simulation
    }

    class EngineServices {
        +panels() PanelService
        +hotkeys() HotkeyService
        +render() RenderService
        +memory() MemoryService
        +clock() SimulationClock
        +config() ConfigService
    }

    class SimulationHost {
        +panels() PanelService
        +hotkeys() HotkeyService
        +render() RenderService
        +memory() MemoryService
        +clock() SimulationClock
    }

    Engine --> EngineServices
    EngineServices ..> SimulationHost
    ISimulation --> SimulationHost
```

## Simulation Lifecycle

Registration uses RAII handles so panels, hotkeys, render views, and commands roll back automatically when a simulation stops or is destroyed.

```mermaid
%%{init: {'theme': 'dark'}}%%
sequenceDiagram
    participant Engine
    participant Host as SimulationHost
    participant Sim as ISimulation
    participant Panels as PanelService
    participant Hotkeys as HotkeyService
    participant Render as RenderService
    participant Clock as SimulationClock

    Engine->>Sim: construct concrete SimulationXXX
    Sim->>Sim: construct surface, particles, behaviors, goals, constraints
    Engine->>Sim: on_register(host)
    Sim->>Panels: register panels
    Panels-->>Sim: PanelHandle
    Sim->>Hotkeys: register callbacks
    Hotkeys-->>Sim: HotkeyHandle
    Sim->>Render: register main + alternate render views
    Render-->>Sim: RenderViewHandle

    Engine->>Sim: on_start()

    loop active frame
        Engine->>Clock: next_tick()
        Clock-->>Engine: TickInfo
        Engine->>Sim: on_tick(TickInfo)
        Sim->>Sim: recalculate SimulationContext
        Sim->>Render: submit RenderPackets
        Engine->>Render: flush views to renderer
    end

    Engine->>Sim: on_stop()
    Sim->>Sim: destroy handles / unregister capabilities
    Engine->>Sim: destroy simulation
```

## ISimulation Contract

```mermaid
%%{init: {'theme': 'dark'}}%%
classDiagram
    class ISimulation {
        <<interface>>
        +name() string_view
        +on_register(SimulationHost&)
        +on_start()
        +on_tick(TickInfo)
        +on_stop()
        +snapshot() SimulationSnapshot
    }

    class SimulationContext {
        +surface() ISurface
        +particles() ParticleStore
        +history() HistoryStore
        +math_cache() DerivedMathCache
        +dirty() DirtyState
        +memory() SimulationMemory
        +tick() TickInfo
    }

    class TickInfo {
        +tick_index u64
        +dt f32
        +time f32
        +paused bool
    }

    ISimulation *-- SimulationContext
    ISimulation --> TickInfo
```

## SimulationContext As State

`SimulationContext` is the authoritative simulation state, not just a temporary accessor.

```mermaid
%%{init: {'theme': 'dark'}}%%
flowchart TB
    Context["SimulationContext"] --> Surface["ISurface instance"]
    Context --> ParticleStore["ParticleStore"]
    Context --> BehaviorState["Behavior state"]
    Context --> GoalState["Goal / win state"]
    Context --> History["HistoryStore"]
    Context --> MathCache["Derived math caches"]
    Context --> Dirty["Dirty flags"]
    Context --> Tick["Tick/time"]
    Context --> Scratch["Scratch/frame allocation"]
    Context --> Persistent["Persistent allocation"]

    ParticleStore --> Particles["Particles"]
    Particles --> Behaviors["Behavior stack"]
    Particles --> Constraints["Constraints"]
    Particles --> Trails["Trails"]

    Surface --> Equations["equations"]
    Surface --> Derivatives["derivatives"]
    Surface --> Curvature["curvature / frames"]

    Scratch --> MemoryService["MemoryService"]
    Persistent --> MemoryService
```

## Surfaces And Math

All domain structures should use NDDE-owned types and math wrappers. Backend/GPU types stay behind aliases or conversion boundaries.

```mermaid
%%{init: {'theme': 'dark'}}%%
flowchart TB
    ISurface["ISurface"] --> Gaussian["GaussianSurface"]
    ISurface --> SineRational["SineRationalSurface"]
    ISurface --> MultiWell["MultiWellSurface"]
    ISurface --> Wave["WavePredatorPreySurface"]
    ISurface --> Deformable["IDeformableSurface capability"]

    Gaussian --> NddeTypes["NDDE scalar/vector/matrix types"]
    SineRational --> NddeTypes
    MultiWell --> NddeTypes
    Wave --> NddeTypes
    Deformable --> SurfaceState["surface deformation state"]

    NddeTypes --> NumericOps["numeric::ops"]
    NumericOps --> MathTraits["MathTraits"]
    MathTraits --> MathConfig["math_config / approximation switches"]

    RenderPackets["RenderPacket / GeometryBatch"] --> NddeTypes
    RendererBoundary["Renderer conversion boundary"] --> GpuAliases["GPU aliases / backend layout types"]
    RenderPackets --> RendererBoundary
```

Rules:

- Domain, simulation, surface, particle, panel, and renderer-neutral structures use NDDE types.
- Math routes through `numeric::ops` / `MathTraits` or approved NDDE wrappers.
- Raw backend types do not leak into simulation APIs.
- GPU layout types live at renderer conversion boundaries.
- Surfaces may be static or dynamic/deformable.
- Time-varying surfaces evaluate against tick/time and deformation state.
- Deformation propagation is simulation math, not renderer logic.

## Deformable Surface Interaction

Deformable surfaces are simulation state. A user action, such as double-clicking a spot on the surface, becomes a simulation command that perturbs the surface and marks dependent caches/views dirty.

```mermaid
%%{init: {'theme': 'dark'}}%%
sequenceDiagram
    participant View as RenderView
    participant Camera as CameraController
    participant Input as Input/Command Service
    participant Sim as ISimulation
    participant Context as SimulationContext
    participant Surface as IDeformableSurface
    participant Render as RenderService

    View->>Camera: double-click screen point
    Camera->>View: ray / surface-pick query
    View->>Input: SurfacePerturbation command
    Input->>Sim: dispatch command
    Sim->>Context: write perturbation event/state
    Context->>Surface: perturb(uv, amplitude, radius, falloff, seed)
    Surface->>Context: mark surface/math/render dirty

    loop following ticks
        Sim->>Surface: advance_deformation(tick)
        Sim->>Context: rebuild dirty math/surface caches
        Sim->>Render: submit updated main and alternate view geometry
    end
```

Suggested POD command:

```cpp
struct SurfacePerturbation {
    Vec2 uv;
    f32 amplitude;
    f32 radius;
    f32 falloff;
    u32 seed;
};
```

Architectural rules:

- Deformable surface state lives in `SimulationContext`.
- Input services produce POD commands; they do not mutate the surface directly.
- Simulations decide how perturbations evolve over time.
- Surface deformation marks main render views and alternate views dirty.
- Dynamic surface caches must rebuild as needed for surface mesh, contour/isocline/vector-field views, particles, and hover math.

## Render Views

Replace hard-coded `Contour2D` with registered render views. A render view might be shown as a second OS window today, a docked panel tomorrow, or a capture target later.

```mermaid
%%{init: {'theme': 'dark'}}%%
flowchart TB
    Simulation["ISimulation"] --> MainView["register Main RenderView"]
    Simulation --> AltView["register Alternate RenderView(s)"]

    MainView --> Camera["CameraController"]
    MainView --> Overlays["ViewOverlayState"]
    MainView --> Geometry["Surface / particles / trails / frames"]

    AltView --> AltMode["AlternateViewMode"]
    AltMode --> Contour["Contour"]
    AltMode --> Isoclines["Isoclines"]
    AltMode --> LevelCurves["Level Curves"]
    AltMode --> VectorField["Vector Field / Flow"]
    AltMode --> Curvature["Curvature Map"]

    Geometry --> RenderPackets["RenderPackets"]
    Contour --> RenderPackets
    Isoclines --> RenderPackets
    LevelCurves --> RenderPackets
    VectorField --> RenderPackets
    Curvature --> RenderPackets

    RenderPackets --> RenderService["RenderService"]
    RenderService --> VulkanRenderer["Vulkan Renderer"]
```

## Camera And View Overlays

Camera and axes belong to render views, not individual simulations.

```mermaid
%%{init: {'theme': 'dark'}}%%
classDiagram
    class RenderView {
        +id RenderViewId
        +title string
        +camera CameraController
        +overlays ViewOverlayState
    }

    class CameraController {
        +set_mode(CameraMode)
        +handle_input(ViewInputFrame)
        +state() CameraState
        +view_projection(RenderView) Mat4
    }

    class ViewOverlayState {
        +show_axes bool
        +show_grid bool
        +show_frame bool
        +show_labels bool
        +show_hover_frenet bool
        +show_osculating_circle bool
    }

    class CameraMode {
        <<enum>>
        Orbit
        Fly
        Locked2D
    }

    RenderView *-- CameraController
    RenderView *-- ViewOverlayState
    CameraController --> CameraMode
```

Default camera model:

- Right drag: orbit
- Shift + right drag: pan
- Wheel: zoom
- `F` or `Home`: frame surface/all particles
- Later: double-click sets orbit pivot under cursor
- Optional later mode: fly camera with right mouse look and WASD/QE

Axes/grid/frame overlays should emit render packets just like simulation geometry, so captures and alternate views remain consistent.

## Hover Math Overlays

Hover inspection is a render-view feature backed by simulation math queries. The view determines what is under the cursor; the simulation answers the math question.

```mermaid
%%{init: {'theme': 'dark'}}%%
graph TB
    Cursor["cursor in RenderView"] --> Picking["view picking / nearest trail point"]
    Picking --> Query["Surface/Particle math query"]
    Query --> Frenet["Frenet frame"]
    Query --> OscCircle["Osculating circle"]
    Query --> Curvature["curvature / derivative values"]

    Frenet --> OverlayPackets["overlay RenderPackets"]
    OscCircle --> OverlayPackets
    Curvature --> PanelReadout["optional panel readout"]
    OverlayPackets --> RenderService["RenderService"]
```

Rules:

- Frenet frame and osculating circle overlays remain first-class features.
- Hover overlays are controlled by `ViewOverlayState`.
- Overlay geometry is renderer-neutral and goes through `RenderService`.
- Picking should use a standard path: screen point -> render view camera -> surface hit or nearest particle/trail point -> simulation math query.
- The simulation provides derivative/frame/curvature data; the renderer only draws the resulting geometry.

## Memory And Allocation Rule

All dynamic allocation goes through a central allocation facility. Stack allocation is allowed.

```mermaid
%%{init: {'theme': 'dark'}}%%
flowchart TB
    MemoryService["MemoryService"] --> PersistentArena["Persistent Arena"]
    MemoryService --> FrameArena["Frame / Render Arena"]
    MemoryService --> ScratchArena["Scratch Arena"]
    MemoryService --> RingBuffers["History Ring Buffers"]
    MemoryService --> Pools["Stable Pools"]

    SimulationContext --> PersistentArena
    SimulationContext --> ScratchArena
    RenderService --> FrameArena
    HistoryStore --> RingBuffers
    ParticleStore --> Pools

    Forbidden["new/delete/malloc/free"] -. only allowed in .-> MemoryService
```

Rules:

- `new`, `delete`, `malloc`, `free`, and raw heap allocation appear only inside allocator/memory facility code.
- Standard containers are allowed only when backed by approved allocators, or in explicitly exempt boundary/setup code.
- Callback storage allocation is owned by services, not scattered through simulation code.
- Start with audit/report mode, then enforce after migration.

## Panel And Hotkey Registration

Panels and hotkeys are registered against services and return handles. Simulations can register callbacks, but the services own callback storage and allocation policy.

```mermaid
%%{init: {'theme': 'dark'}}%%
flowchart TB
    Simulation["SimulationXXX"] --> PanelDefs["Panel descriptors / panel callbacks"]
    Simulation --> HotkeyDefs["Hotkey descriptors / command callbacks"]

    PanelDefs --> PanelService["PanelService"]
    HotkeyDefs --> HotkeyService["HotkeyService"]

    PanelService --> PanelHandle["PanelHandle RAII"]
    HotkeyService --> HotkeyHandle["HotkeyHandle RAII"]

    PanelService --> EngineUI["Engine UI frame"]
    HotkeyService --> EngineInput["Engine input dispatch"]

    EngineUI --> PanelCallbacks["invoke active simulation panel callbacks"]
    EngineInput --> HotkeyCallbacks["invoke active simulation hotkey callbacks"]
```

## Proposed Frame Flow

```mermaid
%%{init: {'theme': 'dark'}}%%
flowchart TD
    Frame["Engine::run_frame"] --> Input["Poll input"]
    Input --> Hotkeys["HotkeyService dispatch"]
    Frame --> Tick["SimulationClock next_tick"]
    Tick --> SimTick["Active ISimulation::on_tick"]

    SimTick --> Recalc["Recalculate SimulationContext"]
    Recalc --> SurfaceDirty["Rebuild surface/math caches if dirty"]
    Recalc --> ParticleStep["Update particles/behaviors/goals"]
    Recalc --> AltDirty["Rebuild alternate views if dirty"]

    SurfaceDirty --> MainPackets["Main view RenderPackets"]
    ParticleStep --> MainPackets
    ParticleStep --> AltPackets["Alternate view RenderPackets"]
    AltDirty --> AltPackets

    MainPackets --> RenderService["RenderService queue"]
    AltPackets --> RenderService
    RenderService --> Overlays["Axes/grid/camera overlays"]
    Overlays --> Vulkan["Vulkan renderer flush"]

    Frame --> Panels["PanelService draw registered panels"]
    Panels --> Commands["Panel commands mutate sim state / dirty flags"]
```

## Current Refactor Scope

In scope for this architecture refactor:

- `ISimulation`
- concrete `SimulationXXX` objects
- engine-owned service access through `SimulationHost`
- `PanelService`
- `HotkeyService`
- `RenderService`
- render views and alternate views
- reusable camera controller and axes/grid overlays
- `SimulationContext` as state container
- central allocation facility policy
- NDDE type/math routing constraints

Out of scope for this pass:

- multithreaded simulation/render separation
- config-authored simulations
- full editor tooling
- scripting/plugin runtime

## Implementation / Audit Item

Add a CI/static audit check for forbidden allocation and type usage patterns, with an allowlist for allocator/memory facility code and temporary explicit exemptions.

Candidate scan patterns:

- `\bnew\b`
- `\bdelete\b`
- `malloc`
- `free`
- unapproved `std::vector`
- unapproved `std::string`
- unapproved `std::make_unique`
- `std::function` outside service/registration code

Start as warning/report-only. Make it enforcing after the allocation migration is complete.

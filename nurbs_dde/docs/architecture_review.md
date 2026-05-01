# nurbs_dde — Architectural Review

## Executive summary

The architecture is **sound for a solo research tool** and follows a coherent
layering discipline throughout.  The engine-scene separation via `EngineAPI` is
the strongest design decision in the codebase.  The sim layer (IEquation /
IIntegrator / IDeformableSurface) is textbook-correct and extensible.  The main
structural debts are: `SurfaceSimScene` has grown into a God Object and needs
decomposition; `GaussianSurface.hpp` hosts three unrelated abstractions;
threading is absent but would be straightforward to add at one well-defined
seam; and the system has no data-export pathway at all, which blocks offline
analysis.

---

## Layer map (what actually exists)

```
┌─────────────────────────────────────────────────────────────────────────┐
│  app/                                                                   │
│    main.cpp              4-line entry point — clean                     │
│    SurfaceSimScene       scene orchestration + UI + rendering           │
│    GaussianSurface.hpp   ISurface impl + FrenetFrame + AnimatedCurve   │
│    GaussianRipple.hpp    IDeformableSurface impl                        │
│    Scene / AnalysisPanel older conics scene, currently inactive        │
├─────────────────────────────────────────────────────────────────────────┤
│  sim/                   equation / integrator / history               │
│    IEquation             pure interface — velocity + noise              │
│    IIntegrator           pure interface — step()                        │
│    GradientWalker        ODE equation                                   │
│    BrownianMotion        SDE equation                                   │
│    EulerIntegrator       deterministic integrator                       │
│    MilsteinIntegrator    stochastic integrator                          │
│    HistoryBuffer         ring buffer for DDE history                    │
│    DelayPursuitEquation  DDE equation                                   │
├─────────────────────────────────────────────────────────────────────────┤
│  math/                  surface geometry                               │
│    ISurface + IDeformableSurface   abstract surface interface           │
│    Torus, Paraboloid              concrete surfaces                     │
│    Scalars.hpp          Vec2/3/4, Topology, DrawMode — no Vulkan       │
├─────────────────────────────────────────────────────────────────────────┤
│  engine/                                                               │
│    Engine               owns all subsystems, runs the frame loop       │
│    EngineAPI            narrow std::function facade — no Vulkan leaks  │
├─────────────────────────────────────────────────────────────────────────┤
│  renderer/  platform/  memory/                                         │
│    Vulkan pipeline, swapchain, arena allocator, GLFW — opaque to sim  │
└─────────────────────────────────────────────────────────────────────────┘
```

---

## What is working well

### 1. EngineAPI: the right abstraction

`EngineAPI` is six `std::function` members.  `SurfaceSimScene` has zero Vulkan
headers; it calls `m_api.acquire()` and `m_api.submit()` and knows nothing about
swapchains, command buffers, or descriptor sets.  This is the correct design.
Replacing Vulkan with Metal, D3D12, or a software rasteriser would require zero
changes to any sim or app code.

The API surface is appropriately slim:

```
acquire(n)          → ArenaSlice          get GPU memory
submit(slice, ...)  → void                draw to primary window
submit2(slice, ...) → void                draw to second window
viewport_size()     → Vec2                framebuffer dimensions
debug_stats()       → const DebugStats&   frame timing + arena use
config()            → const AppConfig&    loaded JSON config
math_font_*()       → ImFont*             font handles
```

Eight functions is tight for a full rendering + windowing API.  No leaks.

### 2. sim/ layer: clean interfaces, correct ownership

`IEquation` / `IIntegrator` / `ISurface` are the right abstractions.  The
separation of drift from integration is mathematically correct and matches the
SDE literature.  `AnimatedCurve`'s dual-ownership model (`m_equation` raw
pointer + optional `m_owned_equation` unique_ptr) is a well-considered
bridge between the shared-equation and per-particle cases.

`HistoryBuffer` is self-contained, ring-buffered, and has correct high-watermark
allocation.  The binary-search interpolation is O(log n) and appropriate.

### 3. Geometry cache

The wireframe + filled surface cache with `is_time_varying()` as the invalidation
oracle is correctly implemented.  Static surfaces never re-tessellate; deforming
surfaces always do; resolution changes and swaps use the `m_wireframe_dirty` flag.
The high-watermark `vector::resize()` avoids heap churn.  This is a textbook
read-through cache.

### 4. BufferManager: per-frame linear arena

Lock-free atomic bump allocation with `reset()` between frames is exactly right
for a single-threaded GPU upload path.  The arena never fragments, never calls
`malloc` during rendering, and has deterministic behaviour.

### 5. CMake target decomposition

`ndde_numeric → ndde_math → ndde_memory → ndde_platform → ndde_renderer →
ndde_engine → nurbs_dde` is a strict DAG with no cycles.  The `ndde_numeric`
INTERFACE library (no Vulkan) is correctly separated so it could be linked from
a Python binding or a headless test runner.

---

## Problems and structural debts

### P1 — SurfaceSimScene is a God Object (critical)

`SurfaceSimScene.cpp` contains approximately 1,400 lines and is responsible for:
- Simulation state ownership and advance loop
- Surface cache management
- Particle spawning and lifecycle
- All Frenet/surface-frame/torsion overlay rendering
- All ImGui panel code for every subsystem
- Hotkey edge-detection for 11 keys
- Wireframe + filled surface tessellation
- 2D contour window rendering
- Camera orbit and 2D pan

This violates single responsibility severely.  The immediate consequence is that
adding a new feature (e.g., Ctrl+A leader seeker) requires editing a 1,400-line
file in five non-adjacent places.  The refactor roadmap already identified
Steps 7–8 (GeometrySubmitter, shrink to orchestrator) — those are the right fix.

**Recommended decomposition:**

```
SimulationState        owns curves, surfaces, history, advance()
ParticleRenderer       submit_trail, submit_head, submit_frenet, submit_torsion
SurfaceRenderer        submit_wireframe, submit_filled, cache management
PanelUI                all ImGui draw_*_panel methods
HotkeyHandler          all edge-detection booleans and dispatch
SurfaceSimScene        thin orchestrator: owns the above, calls them in order
```

### P2 — GaussianSurface.hpp hosts three unrelated abstractions (moderate)

`GaussianSurface.hpp` currently declares:
1. `GaussianSurface` — a concrete ISurface
2. `FrenetFrame` / `SurfaceFrame` / `make_surface_frame` — curve geometry
3. `AnimatedCurve` — a simulation particle with trail, history, equation

These three things have no architectural reason to share a file.  The comment at
the top acknowledges this ("will move in later steps") — those steps should be
scheduled.  The concrete consequence today: any file that needs `AnimatedCurve`
must include the full `GaussianSurface` definition even if it never touches a
`GaussianSurface` object.

**Target layout:**
```
app/AnimatedCurve.hpp      particle + trail + history
app/FrenetFrame.hpp        FrenetFrame, SurfaceFrame, make_surface_frame
math/GaussianSurface.hpp   ISurface implementation only
```

### P3 — WalkState / ParticleState impedance mismatch (moderate)

`AnimatedCurve` stores `WalkState { f32 x, y, phase, angle }` internally and
builds a `ParticleState { glm::vec2 uv, float phase, float angle }` on each step.
These are the same four fields with different names.  The conversion is done via
four manual assignments in `step()` and then synced back after the integrator
runs.

This duplication will become a bug surface once multiple integrators with
different state requirements are added (e.g., RK4 needs intermediate k1–k4
states; Milstein for the angle would need to track noise increments).  The fix is
to make `WalkState` either a typedef for `ParticleState` or to store a
`ParticleState` directly in `AnimatedCurve` and retire `WalkState`.

### P4 — Scene.cpp / AnalysisPanel.cpp are dead code (minor)

`Scene` and `AnalysisPanel` exist in the build but `Engine::run_frame()` has
`m_scene->on_frame()` commented out.  This is live-but-inactive code that adds
compile time and cognitive overhead.  Either re-activate them or move them to a
`legacy/` subdirectory guarded by a CMake option.

### P5 — The API exposes two windows (submit / submit2) but the mental model leaks

`submit2` is "the second window" — but `SurfaceSimScene` has to know that
trails go to the primary window and the contour plot goes to the second window.
This is scene-level knowledge encoded in the API.  A cleaner abstraction would
be named render targets:

```cpp
api.submit_to("primary", slice, ...)
api.submit_to("contour", slice, ...)
```

Not urgent, but `submit2` will multiply as more windows are added.

---

## Threading: should you add a thread handler?

**Short answer: not yet, but the seam is already obvious.**

**What you have:** a single-threaded main loop.  The frame sequence is:
```
poll_events → buffer_reset → begin_frame → on_frame (sim + UI) → end_frame
```

Everything runs on one thread.  This is correct and safe for the current scale
(tens of particles, one surface).  The GPU is fed from one command buffer per
frame.

**The natural threading seam is between simulation and rendering:**

```
Thread A (render thread):   poll → begin_frame → wait_for_sim → submit → end_frame
Thread B (sim thread):      advance_simulation → fill_cpu_cache → signal_render_thread
```

The synchronisation primitive would be a double-buffered particle state or a
mutex + condition variable around the "sim complete, render may start" signal.

**What would it take:**

1. Move `advance_simulation()`, `push_history()`, and all cache rebuilds to a
   worker thread.
2. Double-buffer `m_wireframe_cache`, `m_filled_cache`, and `m_trail` (each
   `AnimatedCurve`'s trail vector) so the render thread reads last-frame data
   while the sim thread writes this-frame data.
3. Add a `std::binary_semaphore` (C++20) or `std::mutex` +
   `std::condition_variable` at the frame boundary.
4. The `BufferManager::acquire()` is already lock-free on the cursor —
   it would stay on the render thread only.

**Risk:** The `HistoryBuffer` is read by `DelayPursuitEquation::velocity()`
(sim thread) and written by `push_history()` (also sim thread) — no cross-thread
issue there.  The render thread only reads `m_trail` (for geometry submission)
while the sim thread appends to it — that IS a data race today.  Double-buffering
the trail would fix it.

**Verdict:** Add threading only when the sim is CPU-bound enough to drop frames.
At the current particle count, the sim takes < 0.1 ms per frame and GPU command
recording takes 1–2 ms.  Threading would add synchronisation complexity for zero
measured benefit.  When you add hundreds of particles or begin parallelising the
integration loop (each particle is independent — embarrassingly parallel), revisit
this.  The correct structure when you get there is a thread pool running the
particle advance loop, not one sim thread vs one render thread.

---

## API review: is it simple, slim, and logically grouped?

### EngineAPI — yes, already good

The eight `std::function` members are the right granularity.  One thing to
tighten: `math_font_body` and `math_font_small` are ImGui-specific and bleed
the rendering concern into the app layer.  A cleaner alternative:

```cpp
// instead of exposing font handles
void push_math_font(bool small = false);
void pop_math_font();
```

This keeps `ImFont*` internal to the renderer.

### IEquation — correct, one note

`velocity()` takes `ParticleState&` (mutable) because `GradientWalker` needs
to persist `angle`.  This is the right decision (as documented) but it means
`IEquation` is not a pure function — it is a stateful functor.  Making this
explicit in the name would help:

```cpp
// rename the virtual to be honest:
virtual glm::vec2 update(ParticleState& state, ...) const = 0;
// "update" signals mutation; "velocity" implies pure computation
```

### IIntegrator — correct and complete

`step(ParticleState&, IEquation&, ISurface&, float t, float dt)` is the right
signature.  Boundary handling being excluded is the right call.

### AnimatedCurve — slightly overloaded

The public interface has 18 methods.  The ones that belong together by concern:

```
Trail:     trail_size, trail_pt, has_trail, trail_vertex_count,
           tessellate_trail, trail_colour, head_world, head_uv
Lifecycle: advance, reset, role, colour_slot, head_colour
History:   enable_history, push_history, query_history, history()
Equation:  equation()
```

Eighteen methods is not too many for what it does, but grouping them with
section comments (as already partially done) helps.

---

## Data export: what would it take?

### What data exists

Each `AnimatedCurve` holds:
- `m_trail`: `std::vector<Vec3>` — world-space positions (x, y, z) at each
  trail push, sampled when distance threshold exceeded
- `m_history`: `HistoryBuffer` of `{float t, glm::vec2 uv}` — parameter-space
  position at regular timestamps (if recording enabled)
- `m_walk`: current `{x, y, phase, angle}` — live state only
- Role, colour slot, equation name

The surface holds:
- Analytic geometry (evaluated on demand)
- `m_sim_time` — accumulated simulation clock

### Minimal useful export: CSV per particle

This is 50 lines of code.  Add one method to `SurfaceSimScene`:

```cpp
void export_session(const std::string& path) const;
```

Implementation:
```cpp
void SurfaceSimScene::export_session(const std::string& path) const {
    std::ofstream f(path);
    f << "particle_id,role,eq_name,step,u,v,world_x,world_y,world_z\n";
    for (u32 ci = 0; ci < m_curves.size(); ++ci) {
        const auto& c = m_curves[ci];
        const std::string role = (c.role() == AnimatedCurve::Role::Leader)
            ? "leader" : "chaser";
        const std::string eq = c.equation() ? c.equation()->name() : "unknown";

        // History records: parameter-space positions with timestamps
        if (c.history()) {
            const auto& h = *c.history();
            for (std::size_t i = 0; i < h.size(); ++i) {
                // HistoryBuffer doesn't expose records by index yet --
                // add a records() accessor returning span<const Record>
                // This is a one-line addition to HistoryBuffer.
            }
        }

        // Trail: world-space positions
        for (u32 i = 0; i < c.trail_size(); ++i) {
            const Vec3 p = c.trail_pt(i);
            f << ci << ',' << role << ',' << eq << ','
              << i << ',' << p.x << ',' << p.y << ',' << p.x << ','
              << p.y << ',' << p.z << '\n';
        }
    }
}
```

Wire it to a "Export CSV" button in the panel or a Ctrl+E hotkey.

### The missing piece: `HistoryBuffer` doesn't expose its records

`HistoryBuffer` has `push()`, `query()`, `size()`, `oldest_t()`, `newest_t()`
but no way to iterate over all stored records.  Add one accessor:

```cpp
// In HistoryBuffer.hpp
[[nodiscard]] std::span<const Record> records() const noexcept {
    // Returns records in chronological order.
    // When the buffer hasn't wrapped: trivial span over m_records.
    // When wrapped: the span is non-contiguous (two halves around m_head).
    // For export purposes, linearise into a temp vector:
    return {};  // see note below
}
```

The ring buffer is non-contiguous when full.  For export, the simplest fix is
to add a `to_vector()` method that returns a time-sorted `vector<Record>`:

```cpp
[[nodiscard]] std::vector<Record> to_vector() const {
    std::vector<Record> out;
    out.reserve(m_records.size());
    const std::size_t n = m_records.size();
    for (std::size_t i = 0; i < n; ++i) {
        if (n < m_capacity)
            out.push_back(m_records[i]);
        else
            out.push_back(m_records[(m_head + i) % m_capacity]);
    }
    return out;  // already chronological by construction
}
```

This is the only change needed to `HistoryBuffer`.

### Richer export formats

| Format | Use case | Cost |
|--------|----------|------|
| CSV (above) | Quick plotting in Python/Excel | ~50 lines |
| JSON | Structured, nested, human-readable | ~80 lines + nlohmann already linked |
| Binary (raw float32 arrays) | Large datasets, Python `np.fromfile()` | ~30 lines, fastest |
| HDF5 | Scientific standard, hierarchical | Requires `libhdf5` dependency |
| Parquet | Arrow / pandas native | Requires `arrow` dependency |

The JSON option is essentially free since `nlohmann_json` is already a
dependency (linked by `ndde_engine`).  The binary option is the best for
large simulations — a session with 10 particles, 1200 trail points each, sampled
at 120 Hz for 60 seconds produces 10 × 120 × 60 = 72,000 records, each 12
bytes (3 floats) = 864 KB uncompressed.  That fits comfortably in RAM and in a
flat binary file.

### What to record for rigorous analysis

Beyond the trail and history, capture the session metadata:

```json
{
  "session": {
    "surface_type": "GaussianSurface",
    "surface_params": {},
    "sim_time_total": 42.7,
    "sim_speed": 1.0,
    "grid_lines": 64,
    "dt_nominal": 0.0167
  },
  "particles": [
    {
      "id": 0,
      "role": "leader",
      "equation": "GradientWalker",
      "equation_params": { "walk_speed": 0.9, "steer_amp": 0.55 },
      "integrator": "EulerIntegrator",
      "trail": [ [u0,v0,x0,y0,z0], ... ],
      "history": [ [t0,u0,v0], [t1,u1,v1], ... ]
    }
  ]
}
```

This is enough to reproduce a run (given the same surface and RNG seed) and to
perform offline Frenet analysis, mean-squared displacement calculations,
first-passage-time statistics, and Fokker-Planck density estimation.

### RNG seed capture

`MilsteinIntegrator` uses `thread_local std::mt19937 rng{ std::random_device{}() }`.
The seed is ephemeral — lost as soon as the thread exits.  For reproducible
stochastic simulations, change this to:

```cpp
// In MilsteinIntegrator or a shared RNG manager:
static u64 global_seed = 42;
thread_local std::mt19937 rng{ global_seed++ };
```

Expose `global_seed` in the export metadata.  With the seed you can replay any
Brownian path exactly.

---

## Priority action list

| Priority | Action | Effort |
|----------|--------|--------|
| High | Add `HistoryBuffer::to_vector()` and `export_session()` | 1–2 hours |
| High | Split `GaussianSurface.hpp` into three separate headers | 2 hours |
| High | Extract `ParticleRenderer` from `SurfaceSimScene` | 4 hours |
| Medium | Retire `WalkState`, use `ParticleState` directly | 1 hour |
| Medium | Rename `IEquation::velocity()` → `update()` | 30 min |
| Medium | Add RNG seed capture to export metadata | 30 min |
| Low | Replace `submit` / `submit2` with named render targets | 2 hours |
| Low | Add threading if particle count exceeds ~200 | 4+ hours |
| Low | Move `Scene.cpp` to `legacy/` | 15 min |

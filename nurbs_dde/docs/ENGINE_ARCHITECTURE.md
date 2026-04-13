# NDDE Engine — Architecture Reference

**Repository:** `F:\repos\Learning-Real-Analysis\nurbs_dde`
**Last updated:** 2026-04-13

---

## Layer Map

```
app/main.cpp
    └── Engine::start() / run()           [engine/]
          ├── AppConfig                   JSON → typed config struct
          ├── GlfwContext                 window, resize flag          [platform/]
          ├── VulkanContext               instance, device, queues     [platform/]
          ├── Swapchain                   images, format               [renderer/]
          ├── Renderer                    frame loop, pipelines, ImGui [renderer/]
          │     ├── Pipeline × 3          LineList / LineStrip / TriangleList
          │     └── ImGuiLayer            descriptor pool, fonts
          ├── BufferManager               per-frame vertex arena       [memory/]
          └── Scene(EngineAPI)            application logic            [app/]
                ├── Viewport              coordinate transforms (single source of truth)
                ├── AnalysisPanel         ε-δ controls, Frenet frame toggles
                ├── PerformancePanel      frame-time / arena sparklines
                ├── CoordDebugPanel       live coordinate diagnostics
                └── math::IConic          curves + space curves        [math/]
                      ├── Parabola        2D, analytic Frenet frame
                      ├── Hyperbola       2D, two-branch, analytic frame
                      └── Helix           3D space curve, exact κ and τ
```

The dependency rule is strict: each layer may only include headers from
layers **below** it in the diagram. `math/` has no Vulkan. `Scene` has no
Vulkan. The CMake static-library graph enforces this at link time.

---

## The EngineAPI Contract

`Scene` receives an `EngineAPI` struct of `std::function` closures. It sees
nothing else from the engine. The complete interface:

```cpp
api.acquire(n)                       // get n Vertex slots from the arena
api.submit(slice, topology, mode, color, mvp)  // record a draw call
api.math_font_body()                 // ImFont* for body-size STIX math font
api.math_font_small()                // ImFont* for smaller STIX math font
api.viewport_size()                  // Vec2 live window dimensions
api.config()                         // const AppConfig& startup config
api.debug_stats()                    // const DebugStats& per-frame engine stats
```

`acquire` and `submit` are the only two operations a math component needs.
`debug_stats()` is for display panels only — it does not drive logic.

### DebugStats

Populated by `Engine` at the start of each `run_frame()` and updated with
arena counts after `Scene::on_frame()` returns:

```cpp
struct DebugStats {
    u64 arena_bytes_used;    // bytes written this frame
    u64 arena_bytes_total;   // arena capacity (128 MiB default)
    f32 arena_utilisation;   // [0, 1]
    u64 arena_vertex_count;  // total vertices submitted
    u32 draw_calls;          // vkCmdDraw calls — latched from previous frame
    u32 swapchain_w;
    u32 swapchain_h;
    f32 frame_ms;            // wall-clock delta in milliseconds (glfwGetTime)
    f32 fps;                 // 1000 / frame_ms
};
```

`draw_calls` is the count from the **previous** completed frame because the
current frame has not begun drawing when `debug_stats()` is first populated.
It is latched in `Renderer::end_frame()` and is accurate to ±1 frame.

---

## Frame Sequencing

Every frame follows this exact order, enforced by `Engine::run_frame()`:

```
1.  Measure wall-clock delta (glfwGetTime)
2.  BufferManager::reset()           — cursor back to 0, arena reused
3.  Renderer::begin_frame()          — wait fence, acquire swapchain image,
                                       reset draw-call counter, begin cmd
4.  Renderer::imgui_new_frame()      — ImGui input snapshot
5.  Engine populates DebugStats      — previous-frame draw count, timing
6.  Scene::on_frame()                — ALL user code runs here
      a. sync_viewport()             — populate m_vp from fb and ImGui IO
      b. update_camera()             — pan/zoom/orbit input
      c. update_hover() / _3d()      — hit-test snap cache
      d. CoordDebugPanel::update()   — snapshot coordinate values
      e. draw_*_panel()              — ImGui UI (must all be in same ImGui pass)
      f. submit_*()                  — acquire → write → submit geometry
7.  Engine updates DebugStats        — arena bytes/vertex count now accurate
8.  Renderer::imgui_render()         — flush ImGui draw data
9.  Renderer::end_frame()            — latch draw-call count, end rendering,
                                       submit queue, present
```

Nothing outside `Scene::on_frame()` is user-accessible. The engine is a
black box that calls `on_frame()` once per frame and handles everything else.

---

## Memory Model

The vertex arena is a single 128 MiB host-visible, host-coherent Vulkan
buffer, persistently mapped for the lifetime of the application. Each frame:

- `reset()` moves the bump cursor back to byte 0 (O(1), no free)
- `acquire(n)` atomically advances the cursor by `n * sizeof(Vertex)`
  and returns a pointer directly into GPU-visible memory
- `submit()` records `vkCmdBindVertexBuffers` + `vkCmdDraw` referencing
  the slice's byte offset in that buffer

Zero copies. No staging. The GPU reads the memory the CPU just wrote.
The render fence in `begin_frame()` guarantees the GPU has finished
reading the previous frame before `reset()` reclaims the memory.

### Arena Budget Guidance

At current scale (parabola + hyperbola + helix + epsilon sphere + Frenet
frame + osculating circle + grid + axes), a typical frame uses under 1 MiB
of the 128 MiB arena — under 1% utilisation. The `PerformancePanel` shows
live utilisation with a 120-frame sparkline.

Future Brownian motion simulation (N particles × trajectory history) will
require a separate **device-local** `SimulationBuffer` (compute-writeable
SSBO) that does **not** reset each frame. The arena handles only per-frame
display geometry. See *Planned: SimulationBuffer* below.

---

## Camera: Orthographic (2D) and Perspective (3D)

### 2D — Orthographic

World space is right-handed: X right, Y up, Z toward viewer.

```
half_h = base_extent / zoom
half_w = half_h * (fb_w / fb_h)        ← aspect ratio correction
left   = pan_x - half_w
right  = pan_x + half_w
bottom = pan_y - half_h
top    = pan_y + half_h

MVP = glm::ortho(left, right, bottom, top, -10, 10)
```

One world unit maps to the same physical distance in X and Y — circles are
circles, not ellipses.

### 3D — Perspective Orbit

A spherical orbit camera driven by `Viewport::yaw` and `Viewport::pitch`:

```
dist   = base_extent / zoom * 3
eye.x  = pan_x + dist * cos(pitch) * sin(yaw)
eye.y  = pan_y + dist * sin(pitch)
eye.z  =         dist * cos(pitch) * cos(yaw)

MVP = glm::perspective(45°, aspect, 0.01, 500) * glm::lookAt(eye, target, up)
```

Right-drag or middle-drag orbits; scroll zooms. Both cameras use the same
`Viewport` struct — `is_3d` in `AxesConfig` selects which MVP is returned
by `m_vp.ortho_mvp()` vs `m_vp.perspective_mvp()`.

### Vulkan Y-Flip

Vulkan NDC has Y increasing downward; `glm::ortho` assumes Y-up. The
negative-height `VkViewport` trick corrects this in `Renderer::begin_frame()`:

```cpp
VkViewport{ .y = h, .height = -h, ... }
```

This makes NDC Y=+1 → screen top, matching world +Y. See
`docs/COORDINATE_SYSTEMS.md` for the full derivation.

---

## Pipelines

Three `Pipeline` objects are created at startup, one per `Topology`:

| Topology | VkPrimitiveTopology | Primary uses |
|---|---|---|
| `LineList` | LINE_LIST | Axes, grid, secant, tangent, Frenet frame arrows |
| `LineStrip` | LINE_STRIP | Curves, epsilon ball/sphere, osculating circle |
| `TriangleList` | TRIANGLE_LIST | Surfaces (future) |

The `Hyperbola::tessellate_two_branch()` uses `LineStrip` with a NaN
sentinel vertex between the two branches — the fragment shader discards
the degenerate triangle, breaking the strip cleanly.

---

## Scene Panels

| Panel | Class | Toggle |
|---|---|---|
| Scene | inline `draw_main_panel()` | always visible |
| Analysis (ε-δ controls) | `AnalysisPanel` | always visible |
| Analysis Readout | `AnalysisPanel` | always visible |
| Coord Debug | `CoordDebugPanel` | "Coord Debug" button |
| Performance | `PerformancePanel` | "Perf Stats" button (green when open) |

The Scene panel header also shows a one-liner FPS / arena% / vertex-count /
draw-call summary so health is visible without opening the full stats window.

---

## IConic — The Math Interface

All curves in `math/Conics.hpp` implement `IConic`. The interface now includes
the full **Frenet–Serret** frame:

```cpp
class IConic {
    virtual Vec3  evaluate(float t)          = 0;  // position p(t)
    virtual Vec3  derivative(float t)        = 0;  // p'(t)
    virtual Vec3  second_derivative(float t) = 0;  // p''(t)
    virtual Vec3  third_derivative(float t)  = 0;  // p'''(t)
    virtual float curvature(float t)         = 0;  // κ = |p'×p''| / |p'|³
    virtual float torsion(float t)           = 0;  // τ = (p'×p'')·p''' / |p'×p''|²
            Vec3  unit_tangent(float t);           // T = p'/|p'|
            Vec3  unit_normal(float t);            // N = (p''-(p''·T)T)/|...|
            Vec3  unit_binormal(float t);          // B = T × N
};
```

Default implementations of `curvature`, `torsion`, `unit_normal`, and
`unit_binormal` use central-difference finite differences and are correct
for all subclasses. Concrete classes override them with analytic formulas
for accuracy and performance.

`HoverResult` carries T, N, B, κ, τ, and osculating radius for every
snapped point — these are computed once per frame in `update_hover()` and
consumed by the Frenet frame arrows, the osculating circle, and the
Analysis Readout panel.

---

## Curve Types

| Type | Dim | snap_cache | Frenet frame |
|---|---|---|---|
| `Parabola` | 2D | `vector<pair<float,float>>` | Analytic T; finite-diff N, B |
| `Hyperbola` | 2D | Two-branch `vector<pair<float,float>>` | Analytic T; finite-diff N, B |
| `Helix` | 3D | `vector<float>` (x,y,z interleaved) | **All analytic**: exact κ, τ (constant) |

Helix is only rendered in 3D mode; its snap uses screen-projected distance
from the 3D cursor. See `COORDINATE_SYSTEMS.md §3D Snap`.

---

## Planned: SimulationBuffer

When the Brownian motion pursuit simulation arrives, the memory architecture
will split into two independent concerns:

```
┌──────────────────────────────────────────────────────────┐
│ Geometry Arena (host-visible VBO) — current system       │
│  Display overlays: axes, curves, epsilon sphere, labels  │
│  Host-written every frame, bump-allocator reset          │
│  128 MiB, one allocation, zero fragmentation             │
└──────────────────────────────────────────────────────────┘
┌──────────────────────────────────────────────────────────┐
│ SimulationBuffer (device-local SSBO) — future            │
│  Particle positions, velocities, Wiener increments       │
│  Written by compute shader, read by vertex shader        │
│  Persistent — does NOT reset every frame                 │
│  Spatially indexed by toroidal spatial hash (math/)      │
└──────────────────────────────────────────────────────────┘
```

The arena is not the right place for spatial indexing. The `BufferManager`
stays exactly as-is. The toroidal spatial hash lives in `math/` as a pure
data structure over particle positions.

---

## Adding a Component

See `docs/ADDING_A_MATH_COMPONENT.md` for the full walkthrough.
The short version: write a class in `math/`, add it to `ndde_math` in
`src/CMakeLists.txt`, wire it in `app/Scene`. Touch nothing else.

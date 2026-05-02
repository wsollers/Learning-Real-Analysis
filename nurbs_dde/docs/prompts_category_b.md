# Prompt: implement Category B (God Object decomposition)

Paste everything between BEGIN PROMPT and END PROMPT into a new chat.

---
<!-- BEGIN PROMPT -->

You are an expert C++20 software engineer and architect working on **nurbs_dde**,
a real-time Vulkan particle simulation.  You have Filesystem tools that let you
read and write files directly on the user's machine.  Before touching any file,
always read it first.  Never skip steps.  Never guess at existing code -- read it.

The user's repo root is:
```
F:\repos\Learning-Real-Analysis\nurbs_dde\
```

---

## Your task: implement Category B (4 items)

Category B is the God Object decomposition.  `SurfaceSimScene` is currently a
~700-line file that does six different jobs.  The goal is to split it into clean
single-responsibility units while keeping every existing feature working.

Do the items in this exact order -- each one is a prerequisite for the next.

---

## Before you start: mandatory reads

Read these files in full before writing a single line:

1. `src/app/GaussianSurface.hpp`          — AnimatedCurve, FrenetFrame, SurfaceFrame
2. `src/app/SurfaceSimScene.hpp`          — all member declarations
3. `src/app/SurfaceSimScene.cpp`          — all implementations (read it completely)
4. `src/math/Surfaces.hpp`               — ISurface interface
5. `src/app/GaussianRipple.hpp`           — IDeformableSurface concrete
6. `src/sim/IEquation.hpp`               — ParticleState, IEquation
7. `src/sim/HistoryBuffer.hpp`            — HistoryBuffer

Do not proceed until you have read all seven.  The implementations contain exact
method signatures and code patterns that the split must preserve verbatim.

---

## B1 — Split GaussianSurface.hpp into three headers

### Why

`GaussianSurface.hpp` currently declares three unrelated things in one file:
1. `GaussianSurface` — a concrete ISurface
2. `FrenetFrame`, `SurfaceFrame`, `make_surface_frame` — curve geometry structs
3. `AnimatedCurve` — a simulation particle

Any file that needs `AnimatedCurve` must include the full `GaussianSurface`
definition.  `GeometrySubmitter` (B2) needs `AnimatedCurve` but not
`GaussianSurface`.  The split is the prerequisite.

### Target layout

```
src/app/FrenetFrame.hpp      FrenetFrame, SurfaceFrame, make_surface_frame
src/app/AnimatedCurve.hpp    AnimatedCurve class (full declaration + inline methods)
src/app/GaussianSurface.hpp  GaussianSurface class only (stripped down)
```

### How to do it

**Step 1 — Create `src/app/FrenetFrame.hpp`**

Move `FrenetFrame`, `SurfaceFrame`, and `make_surface_frame` verbatim from
`GaussianSurface.hpp` into the new file.  The new file needs:
```cpp
#pragma once
#include "math/Scalars.hpp"
#include "math/Surfaces.hpp"
#include <glm/glm.hpp>
```
The `make_surface_frame` function is `inline` so it stays in the header.

**Step 2 — Create `src/app/AnimatedCurve.hpp`**

Move the `AnimatedCurve` class declaration verbatim from `GaussianSurface.hpp`.
The new file needs:
```cpp
#pragma once
#include "math/Scalars.hpp"
#include "math/Surfaces.hpp"
#include "sim/IEquation.hpp"
#include "sim/IIntegrator.hpp"
#include "sim/HistoryBuffer.hpp"
#include "app/FrenetFrame.hpp"
#include "renderer/GpuTypes.hpp"
#include <glm/glm.hpp>
#include <vector>
#include <span>
#include <memory>
```
All inline method bodies (`head_uv()`, `trail_size()`, `has_trail()`,
`trail_pt()`, `role()`, `colour_slot()`, `history()`, `equation()`,
`head_colour()`) stay in the header as they are.

**Step 3 — Strip `GaussianSurface.hpp`**

Remove `FrenetFrame`, `SurfaceFrame`, `make_surface_frame`, and `AnimatedCurve`
from `GaussianSurface.hpp`.  Replace them with includes of the new headers:
```cpp
#include "app/FrenetFrame.hpp"
#include "app/AnimatedCurve.hpp"
```
`GaussianSurface.hpp` must still compile standalone and still be included by
`SurfaceSimScene.hpp` (which already includes `GaussianSurface.hpp`).

**Step 4 — Update `SurfaceSimScene.hpp`**

Add these includes (they may already arrive transitively through
`GaussianSurface.hpp` but make them explicit for clarity):
```cpp
#include "app/FrenetFrame.hpp"
#include "app/AnimatedCurve.hpp"
```

**Step 5 — Update `GaussianSurface.cpp`**

Add at the top:
```cpp
#include "app/AnimatedCurve.hpp"
#include "app/FrenetFrame.hpp"
```

**Step 6 — Verify**

Build the project.  Every existing feature must still compile and work.
No behaviour changes.

---

## B2 — Extract ParticleRenderer

### Why

`SurfaceSimScene.cpp` contains all the `submit_*` methods — `submit_trail_3d`,
`submit_head_dot_3d`, `submit_frenet_3d`, `submit_osc_circle_3d`,
`submit_surface_frame_3d`, `submit_normal_plane_3d`, `submit_torsion_3d`,
and `submit_arrow`.  These are rendering concerns, not scene-orchestration
concerns.  They all follow the same pattern: take a `const AnimatedCurve&` and
an `mvp`, call `m_api.acquire()` and `m_api.submit()`.

### Target

New files:
```
src/app/ParticleRenderer.hpp   — class declaration
src/app/ParticleRenderer.cpp   — implementations
```

### ParticleRenderer interface

```cpp
// src/app/ParticleRenderer.hpp
#pragma once
#include "engine/EngineAPI.hpp"
#include "app/AnimatedCurve.hpp"
#include "app/FrenetFrame.hpp"
#include "math/Scalars.hpp"
#include <vector>

namespace ndde {

class ParticleRenderer {
public:
    explicit ParticleRenderer(EngineAPI api) : m_api(std::move(api)) {}

    // Visibility flags -- set by SurfaceSimScene from hotkey state
    bool show_frenet       = true;
    bool show_T            = true;
    bool show_N            = true;
    bool show_B            = true;
    bool show_osc          = true;
    bool show_dir_deriv    = false;
    bool show_normal_plane = false;
    bool show_torsion      = false;
    float frame_scale      = 0.25f;

    // Submit all particles for the current frame.
    // snap_curve / snap_idx: which trail point the cursor is hovering.
    // sim_time: passed through to surface queries inside overlay methods.
    void submit_all(const std::vector<AnimatedCurve>& curves,
                    const ndde::math::ISurface&        surface,
                    float                               sim_time,
                    const Mat4&                         mvp,
                    int snap_curve,
                    int snap_idx,
                    bool snap_on_curve);

    // Individual submit methods (kept public for future composability)
    void submit_trail_3d        (const AnimatedCurve& c, const Mat4& mvp);
    void submit_head_dot_3d     (const AnimatedCurve& c, const Mat4& mvp);
    void submit_frenet_3d       (const AnimatedCurve& c, u32 trail_idx, const Mat4& mvp);
    void submit_osc_circle_3d   (const AnimatedCurve& c, u32 trail_idx, const Mat4& mvp);
    void submit_surface_frame_3d(const AnimatedCurve& c, u32 trail_idx, const Mat4& mvp,
                                 const ndde::math::ISurface& surface, float sim_time);
    void submit_normal_plane_3d (const AnimatedCurve& c, u32 trail_idx, const Mat4& mvp,
                                 const ndde::math::ISurface& surface, float sim_time);
    void submit_torsion_3d      (const AnimatedCurve& c, u32 trail_idx, const Mat4& mvp);

private:
    EngineAPI m_api;

    void submit_arrow(Vec3 origin, Vec3 dir, Vec4 color, float length,
                      const Mat4& mvp, Topology topo = Topology::LineList);
};

} // namespace ndde
```

### Implementation

**Step 1 — Create `src/app/ParticleRenderer.cpp`**

Copy the following methods verbatim from `SurfaceSimScene.cpp` into
`ParticleRenderer.cpp`, changing `SurfaceSimScene::` to `ParticleRenderer::`:
- `submit_arrow`
- `submit_trail_3d`
- `submit_head_dot_3d`
- `submit_frenet_3d`
- `submit_osc_circle_3d`
- `submit_surface_frame_3d`  (add `const ISurface& surface, float sim_time` params)
- `submit_normal_plane_3d`   (add `const ISurface& surface, float sim_time` params)
- `submit_torsion_3d`

For `submit_surface_frame_3d` and `submit_normal_plane_3d`, these methods
currently access `m_surface` and `m_sim_time` as scene members.  In
`ParticleRenderer` they receive these as explicit parameters instead.

Implement `submit_all()` as a loop over curves calling `submit_trail_3d` and
`submit_head_dot_3d` for every particle, plus the overlay methods for the
active particle (determined by `snap_curve`/`snap_idx`/`snap_on_curve`).
This logic is currently the inner loop of `draw_surface_3d_window()` in
`SurfaceSimScene.cpp` — move it here.

**Step 2 — Remove from `SurfaceSimScene`**

In `SurfaceSimScene.hpp`:
- Remove the declarations of `submit_arrow`, `submit_trail_3d`,
  `submit_head_dot_3d`, `submit_frenet_3d`, `submit_osc_circle_3d`,
  `submit_surface_frame_3d`, `submit_normal_plane_3d`, `submit_torsion_3d`
- Add a member: `ParticleRenderer m_particle_renderer;`
- Add `#include "app/ParticleRenderer.hpp"`

In `SurfaceSimScene.cpp`:
- Remove the method bodies for all the above `submit_*` methods
- Update the constructor to initialise `m_particle_renderer`:
  ```cpp
  // ParticleRenderer gets its own copy of the EngineAPI handle
  // (EngineAPI is a value type -- all std::function members, cheap to copy)
  , m_particle_renderer(m_api)
  ```
- In `draw_surface_3d_window()`, replace the particle rendering loop with:
  ```cpp
  m_particle_renderer.show_frenet       = m_show_frenet;
  m_particle_renderer.show_T            = m_show_T;
  m_particle_renderer.show_N            = m_show_N;
  m_particle_renderer.show_B            = m_show_B;
  m_particle_renderer.show_osc          = m_show_osc;
  m_particle_renderer.show_dir_deriv    = m_show_dir_deriv;
  m_particle_renderer.show_normal_plane = m_show_normal_plane;
  m_particle_renderer.show_torsion      = m_show_torsion;
  m_particle_renderer.frame_scale       = m_frame_scale;
  m_particle_renderer.submit_all(m_curves, *m_surface, m_sim_time,
                                  mvp3d, m_snap_curve, m_snap_idx, m_snap_on_curve);
  ```

**Step 3 — Update CMakeLists.txt**

Read `src/CMakeLists.txt` first to understand the source file list pattern.
Add `app/ParticleRenderer.cpp` to the appropriate target's source list.

**Step 4 — Verify**

Build.  All existing overlays (Frenet frame, surface frame, normal plane, torsion)
must still appear correctly.  No behaviour changes.

---

## B3 — Extract SpawnStrategy

### Why

`handle_hotkeys()` currently contains six separate inline particle-spawn
blocks (Ctrl+L, Ctrl+C, Ctrl+B, Ctrl+R, and their panel button duplicates).
Each block:
1. Computes a starting `(sx, sy)` position
2. Constructs an `AnimatedCurve` (possibly via `with_equation`)
3. Pre-warms it with N `advance()` calls
4. Pushes it onto `m_curves`

This logic is duplicated between the hotkey handler and the panel buttons,
and it will grow again for Ctrl+A.  Extracting it makes the hotkey handler
a thin dispatcher.

### Target

New files:
```
src/app/SpawnStrategy.hpp    — free functions or a simple struct
```

Header-only is fine — these are short functions.

### Interface

```cpp
// src/app/SpawnStrategy.hpp
#pragma once
#include "app/AnimatedCurve.hpp"
#include "math/Surfaces.hpp"
#include "sim/IEquation.hpp"
#include "sim/IIntegrator.hpp"
#include <vector>

namespace ndde::spawn {

struct SpawnContext {
    const ndde::math::ISurface* surface;
    ndde::sim::IEquation*       shared_equation;   // for Leader/Chaser
    const ndde::sim::IIntegrator* shared_integrator;
    const ndde::sim::IIntegrator* milstein;
    float sim_speed = 1.f;
    int   prewarm_frames = 60;
};

// Reference particle (curve 0 head or zero if no curves exist).
[[nodiscard]] inline glm::vec2 reference_uv(
    const std::vector<AnimatedCurve>& curves,
    const ndde::math::ISurface& surface)
{
    if (curves.empty())
        return { (surface.u_min() + surface.u_max()) * 0.5f,
                 (surface.v_min() + surface.v_max()) * 0.5f };
    return curves[0].head_uv();
}

// Offset spawn position from a reference UV, clamped to domain.
[[nodiscard]] inline glm::vec2 offset_spawn(
    glm::vec2 ref_uv,
    float     offset_radius,
    float     angle,
    const ndde::math::ISurface& surface)
{
    const float margin = 0.5f;
    return {
        std::clamp(ref_uv.x + offset_radius * std::cos(angle),
                   surface.u_min() + margin, surface.u_max() - margin),
        std::clamp(ref_uv.y + offset_radius * std::sin(angle),
                   surface.v_min() + margin, surface.v_max() - margin)
    };
}

// Spawn a shared-equation particle (Leader or Chaser).
inline AnimatedCurve spawn_shared(
    glm::vec2 uv, AnimatedCurve::Role role, u32 slot,
    const SpawnContext& ctx)
{
    AnimatedCurve c(uv.x, uv.y, role, slot,
                    ctx.surface, ctx.shared_equation, ctx.shared_integrator);
    for (int i = 0; i < ctx.prewarm_frames; ++i)
        c.advance(1.f/60.f, ctx.sim_speed);
    return c;
}

// Spawn an owned-equation particle (Brownian, DelayPursuit, etc.).
inline AnimatedCurve spawn_owned(
    glm::vec2 uv, AnimatedCurve::Role role, u32 slot,
    std::unique_ptr<ndde::sim::IEquation> eq,
    const SpawnContext& ctx,
    bool prewarm = true)
{
    AnimatedCurve c = AnimatedCurve::with_equation(
        uv.x, uv.y, role, slot,
        ctx.surface, std::move(eq), ctx.milstein);
    if (prewarm)
        for (int i = 0; i < ctx.prewarm_frames; ++i)
            c.advance(1.f/60.f, ctx.sim_speed);
    return c;
}

} // namespace ndde::spawn
```

### Wiring in SurfaceSimScene

In `SurfaceSimScene.hpp`:
- Add `#include "app/SpawnStrategy.hpp"`

In `SurfaceSimScene.cpp`, rewrite the spawn blocks in `handle_hotkeys()` to
use the helpers.  For example, Ctrl+L becomes:

```cpp
const bool ctrl_l = io.KeyCtrl && ImGui::IsKeyDown(ImGuiKey_L);
if (ctrl_l && !m_ctrl_l_prev) {
    const glm::vec2 ref = spawn::reference_uv(m_curves, *m_surface);
    const glm::vec2 uv  = spawn::offset_spawn(ref, 1.5f,
        static_cast<float>(m_leader_count) * 1.1f, *m_surface);
    const spawn::SpawnContext ctx{ m_surface.get(), &m_equation,
                                   &m_integrator, &m_milstein, m_sim_speed };
    m_curves.push_back(spawn::spawn_shared(uv,
        AnimatedCurve::Role::Leader,
        m_leader_count % AnimatedCurve::MAX_SLOTS, ctx));
    ++m_leader_count;
}
m_ctrl_l_prev = ctrl_l;
```

Apply the same pattern to Ctrl+C, Ctrl+B, and Ctrl+R.

Also replace the panel button duplicates (the "Spawn Brownian [Ctrl+B]" button
body and the "Spawn Pursuer [Ctrl+R]" button body) with calls to the same
helpers so the duplication is gone.

**Verify:** Build.  All hotkeys and panel buttons spawn particles exactly as before.

---

## B4 — Shrink SurfaceSimScene to thin orchestrator

### Why

After B2 and B3, the responsibilities that remain in `SurfaceSimScene` are:
- Own the surface, curves, integrators, equations
- Call `advance_simulation(dt)`
- Call `m_particle_renderer.submit_all(...)`
- Call `draw_simulation_panel()`, `draw_hotkey_panel()`
- Dispatch hotkeys (now thin after B3)
- Handle surface-specific 3D window setup (camera orbit, canvas MVP, paused overlay)
- Handle the 2D contour second window

`SurfaceSimScene.cpp` should be around 300–400 lines after this step.

### What to do

After B2 and B3 are verified, do a final review:

1. Read `SurfaceSimScene.cpp` and identify any remaining blocks of rendering
   logic that were not moved in B2 (e.g., the `submit_contour_second_window`
   method still contains a rendering loop for trails — this is acceptable to
   leave, as the 2D contour window is a separate rendering context from the
   particle overlays).

2. Check that `draw_simulation_panel()` does not contain any `submit_*`
   calls.  If it does, those are bugs — the panel should only call `ImGui::*`
   functions.

3. Count the remaining lines.  If `SurfaceSimScene.cpp` is still over 500
   lines, identify the largest remaining block and determine whether it belongs
   in `ParticleRenderer`, `SpawnStrategy`, or a new `SurfaceRenderer` class.
   Report your findings to the user before making further changes.

4. Update the file-level comment at the top of `SurfaceSimScene.hpp` to
   accurately describe what the class now does.

---

## After completing all four items

### Update TODO.md

Read `F:\repos\Learning-Real-Analysis\nurbs_dde\TODO.md`.
Change the status of B1, B2, B3, B4 from `[ ]` to `[x]`.

### Update refactor_progress.md

Read `F:\repos\Learning-Real-Analysis\nurbs_dde\refactor_progress.md`.
Add a new section `## Category B -- DONE` describing:
- Which files were created
- Which methods moved where
- Which methods were removed from SurfaceSimScene
- The final line count of SurfaceSimScene.cpp

### Update CMakeLists.txt

Every new `.cpp` file created must be added to the source list.  Read the
CMakeLists.txt before editing it so you get the format exactly right.

---

## Key invariants to preserve throughout

**Never change existing behaviour.**  The decomposition is purely structural.
Every hotkey, every overlay, every panel slider, every surface must work
identically after each step.  After each item (B1, B2, B3, B4) build and
verify before proceeding.

**EngineAPI is a value type.**  It is a struct of `std::function` members.
It can be copied cheaply.  `ParticleRenderer` receives its own copy from
the constructor — this is correct and safe.

**`m_api.submit` vs `m_api.submit2`.**  `submit` goes to the primary 3D window.
`submit2` goes to the contour second window.  `ParticleRenderer` uses `submit`
only.  The `submit_contour_second_window` method stays in `SurfaceSimScene` because
it uses `submit2` and has scene-specific 2D MVP logic.

**Spawn function pre-warm convention.**  Leader and Chaser particles are
pre-warmed with 60 frames.  Delay-pursuit chasers are NOT pre-warmed (they
need the leader to accumulate history first).  The `prewarm` parameter in
`spawn_owned` handles this:  `spawn_owned(..., prewarm=false)` for Ctrl+R.

**`#include` order matters for circular dependency prevention.**
`AnimatedCurve.hpp` must NOT include `GaussianSurface.hpp`.
`GaussianSurface.hpp` includes `AnimatedCurve.hpp` (via the new header).
`SurfaceSimScene.hpp` includes `GaussianSurface.hpp` which transitively
brings in `AnimatedCurve.hpp` and `FrenetFrame.hpp`.

**`ParticleRenderer` does not own the surface pointer.**  It receives
`const ISurface&` as a parameter to `submit_all()` and the overlay methods.
The surface is owned by `SurfaceSimScene`.

<!-- END PROMPT -->

---

## Usage notes (for you, not for the assistant)

- Paste only the content between BEGIN PROMPT and END PROMPT.
- The assistant will read all seven source files before writing anything.
- After B1: build to verify — if it fails the assistant will fix the include graph.
- After B2: test each overlay (Ctrl+F, Ctrl+D, Ctrl+N, Ctrl+T) to confirm they
  still appear in the 3D window.
- After B3: test each spawn hotkey (Ctrl+L, Ctrl+C, Ctrl+B, Ctrl+R) and their
  corresponding panel buttons.
- After B4: the assistant reports the final line count of SurfaceSimScene.cpp.
  If it is still over 500 lines, ask what remains and decide whether to continue.
- CMakeLists.txt must be updated after B2 (ParticleRenderer.cpp added).
  B3 is header-only so no CMake change needed for it.

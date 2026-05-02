# Prompt: implement Category A (data correctness + export)

Paste everything between BEGIN PROMPT and END PROMPT into a new chat.

---
<!-- BEGIN PROMPT -->

You are an expert C++20 software engineer working on **nurbs_dde**, a real-time
Vulkan particle simulation.  You have Filesystem tools that let you read and
write files directly on the user's machine.  Before touching any file, always
read it first.  Never skip steps.  Never guess at existing code -- read it.

The user's repo root is:
```
F:\repos\Learning-Real-Analysis\nurbs_dde\
```

---

## Your task: implement Category A (4 items)

These are four independent, small changes.  Do them in this order.
Each one is self-contained; none depends on another.

---

### A1 — Retire WalkState: store ParticleState directly in AnimatedCurve

**Problem.**  `AnimatedCurve` has a private `WalkState { f32 x, y, phase, angle; }`
and `step()` manually copies those four fields into a `ParticleState` before
calling the integrator, then copies them back.  `ParticleState` has the same
four fields under different names.  The dual representation will become a bug
surface when RK4 or per-component Milstein is added.

**Files to read first:**
- `src/app/GaussianSurface.hpp` — find `struct WalkState` and `AnimatedCurve`
- `src/app/GaussianSurface.cpp` — find `AnimatedCurve::step()` and the
  constructor that initialises `m_walk`
- `src/sim/IEquation.hpp` — confirm the exact `ParticleState` fields

**What to do:**
1. In `GaussianSurface.hpp`: delete `struct WalkState`.  Replace the private
   member `WalkState m_walk` with `ndde::sim::ParticleState m_walk`.
2. In `GaussianSurface.cpp`: update the constructor — replace
   `m_walk = WalkState{ start_x, start_y }` with
   `m_walk = ndde::sim::ParticleState{ glm::vec2{start_x, start_y}, 0.f, 0.f }`.
3. In `GaussianSurface.cpp`: in `AnimatedCurve::step()`, remove the manual
   copy-in and copy-out around the integrator call.  The integrator already
   takes `ParticleState&` so `m_walk` can be passed directly.
4. Update `head_uv()` in the header: it currently returns
   `{ m_walk.x, m_walk.y }`.  After the change it returns `m_walk.uv`.
5. Update `push_history()` in the `.cpp`: it currently pushes
   `{ m_walk.x, m_walk.y }`.  After the change it pushes `m_walk.uv`.
6. Update `reset()` in the `.cpp`: it sets `m_walk = WalkState{...}`.
   After the change set `m_walk.uv = { m_start_x, m_start_y }; m_walk.phase = 0.f; m_walk.angle = 0.f;`.

**Verify:** the project should compile without errors.  No behaviour changes.

---

### A2 — Add HistoryBuffer::to_vector()

**Problem.**  `HistoryBuffer` is a ring buffer.  When full, the records are
non-contiguous in memory (split at `m_head`).  There is no way to iterate over
all records in chronological order without knowing the internal layout.
Export and debugging both need a linearised view.

**File to read first:**
- `src/sim/HistoryBuffer.hpp` — read the complete file.  Understand `m_records`,
  `m_head`, `m_capacity`, and the `get(i)` lambda inside `query()`.

**What to do.**  Add this public method to `HistoryBuffer` in
`src/sim/HistoryBuffer.hpp`, inside the class body after `clear()`:

```cpp
// Return all records in chronological order (oldest first).
// When the buffer has not yet wrapped: a simple copy of m_records.
// When wrapped: reorders the two halves around m_head.
// Cost: O(n) time and space.  Only call for export/debug, not per-frame.
[[nodiscard]] std::vector<Record> to_vector() const {
    std::vector<Record> out;
    const std::size_t n = m_records.size();
    out.reserve(n);
    for (std::size_t i = 0; i < n; ++i) {
        if (n < m_capacity)
            out.push_back(m_records[i]);           // not yet wrapped
        else
            out.push_back(m_records[(m_head + i) % m_capacity]);  // wrapped
    }
    return out;   // already chronological by construction
}
```

**Verify:** the method compiles.  Write a quick mental test: if capacity=3,
records=[B,C,A] with m_head=2 (A is oldest), `to_vector()` returns [A,B,C].

---

### A3 — Add SurfaceSimScene::export_session()

**Problem.**  Every simulation run produces particle trajectories in RAM that
are thrown away when the window closes.  There is no way to do offline analysis.

**Files to read first:**
- `src/app/SurfaceSimScene.hpp` — understand `m_curves`, `m_surface`,
  `m_sim_time`, `m_surface_type`, `m_grid_lines`, `m_sim_speed`
- `src/app/GaussianSurface.hpp` — confirm `AnimatedCurve` public interface:
  `trail_size()`, `trail_pt(i)`, `role()`, `equation()`, `history()`
- `src/sim/IEquation.hpp` — confirm `IEquation::name()`
- `src/app/SurfaceSimScene.cpp` — find `draw_simulation_panel()` to know where
  to add the Export button in the UI

**What to do.**

Step 1 — Declare the method in `src/app/SurfaceSimScene.hpp` in the private
methods block:
```cpp
void export_session(const std::string& path) const;
```

Step 2 — Add `#include <fstream>` and `#include <iomanip>` to the top of
`src/app/SurfaceSimScene.cpp` if not already present.

Step 3 — Implement in `src/app/SurfaceSimScene.cpp`:

```cpp
// ── export_session ────────────────────────────────────────────────────────────
// Writes particle trajectory data to a CSV file at `path`.
// Two record types, distinguished by the `record_type` column:
//   "trail"   -- world-space position sampled when the trail point was pushed
//   "history" -- parameter-space position sampled at fixed time intervals
//
// Trail records:   particle_id, role, equation, record_type, step, x, y, z
// History records: particle_id, role, equation, record_type, step, t, u, v
//
// Open the file in Python with:
//   import pandas as pd
//   df = pd.read_csv("session.csv")
//   trail = df[df.record_type == "trail"]
//   history = df[df.record_type == "history"]
void SurfaceSimScene::export_session(const std::string& path) const {
    std::ofstream f(path);
    if (!f.is_open()) return;

    f << std::fixed << std::setprecision(6);
    f << "particle_id,role,equation,record_type,step,a,b,c\n";
    // column meanings:
    //   trail:   a=world_x  b=world_y  c=world_z
    //   history: a=sim_t    b=param_u  c=param_v

    for (u32 ci = 0; ci < static_cast<u32>(m_curves.size()); ++ci) {
        const auto& c = m_curves[ci];
        const std::string role = (c.role() == AnimatedCurve::Role::Leader)
            ? "leader" : "chaser";
        const std::string eq = c.equation()
            ? c.equation()->name() : "unknown";

        // Trail: world-space positions
        for (u32 i = 0; i < c.trail_size(); ++i) {
            const Vec3 p = c.trail_pt(i);
            f << ci << ',' << role << ',' << eq << ',' << "trail" << ','
              << i << ',' << p.x << ',' << p.y << ',' << p.z << '\n';
        }

        // History: parameter-space positions with timestamps
        if (c.history()) {
            const auto recs = c.history()->to_vector();
            for (std::size_t i = 0; i < recs.size(); ++i) {
                f << ci << ',' << role << ',' << eq << ',' << "history" << ','
                  << i << ',' << recs[i].t << ','
                  << recs[i].uv.x << ',' << recs[i].uv.y << '\n';
            }
        }
    }
}
```

Step 4 — Add the Export button to the simulation panel.  Find the block in
`draw_simulation_panel()` that draws the "Clear all" button (near the particle
count line).  Add immediately after it:

```cpp
ImGui::SameLine();
if (ImGui::SmallButton("Export CSV")) {
    // Write to the executable's working directory.
    // Filename includes particle count so repeated exports don't overwrite.
    const std::string path = "session_" +
        std::to_string(m_curves.size()) + "p_" +
        std::to_string(static_cast<int>(m_sim_time)) + "s.csv";
    export_session(path);
    // Brief visual confirmation -- ImGui tooltip on next hover is enough.
}
if (ImGui::IsItemHovered())
    ImGui::SetTooltip("Write trail + history data to CSV in working directory");
```

**Verify:** clicking the button creates a `.csv` file.  Open it and confirm
the header row and at least one data row are present.

---

### A4 — Capture RNG seed in MilsteinIntegrator

**Problem.**  `MilsteinIntegrator::normal2()` uses:
```cpp
thread_local std::mt19937 rng{ std::random_device{}() };
```
The seed is drawn from a hardware source and never recorded.  Every stochastic
run is unreproducible.

**File to read first:**
- `src/sim/MilsteinIntegrator.hpp` — read the complete file, especially `normal2()`

**What to do.**

Step 1 — Add a static seed member and a setter to `MilsteinIntegrator`:

In the `public:` section add:
```cpp
// Set the seed used by all MilsteinIntegrator instances in this process.
// Call before spawning any Brownian particles for a reproducible run.
// Default: 0 = use std::random_device (non-reproducible).
static void set_global_seed(uint64_t seed) noexcept {
    s_global_seed = seed;
    s_seed_set    = (seed != 0);
}
[[nodiscard]] static uint64_t global_seed() noexcept { return s_global_seed; }
[[nodiscard]] static bool     seed_is_fixed() noexcept { return s_seed_set; }
```

In the `private:` section add:
```cpp
inline static uint64_t s_global_seed = 0;
inline static bool     s_seed_set    = false;
```

Step 2 — Update `normal2()` to use the seed when set:
```cpp
[[nodiscard]] static glm::vec2 normal2() {
    thread_local std::mt19937 rng{ []() -> uint32_t {
        if (s_seed_set)
            return static_cast<uint32_t>(s_global_seed);
        return std::random_device{}();
    }() };
    thread_local std::uniform_real_distribution<float> uniform(1e-7f, 1.f);

    const float u1 = uniform(rng);
    const float u2 = uniform(rng);
    const float r  = std::sqrt(-2.f * std::log(u1));
    const float th = 2.f * std::numbers::pi_v<float> * u2;
    return { r * std::cos(th), r * std::sin(th) };
}
```

Step 3 — Expose in the simulation panel so the seed can be set and read before
export.  In `draw_simulation_panel()`, in the Brownian motion section, add
after the existing sliders:

```cpp
ImGui::SeparatorText("RNG reproducibility");
{
    static int seed_input = 0;
    ImGui::InputInt("Seed (0=random)##rng", &seed_input);
    ImGui::SameLine();
    if (ImGui::SmallButton("Apply##rng")) {
        ndde::sim::MilsteinIntegrator::set_global_seed(
            static_cast<uint64_t>(seed_input));
    }
    if (ndde::sim::MilsteinIntegrator::seed_is_fixed())
        ImGui::TextColored({0.4f,1.f,0.4f,1.f}, "Seed fixed: %llu",
            (unsigned long long)ndde::sim::MilsteinIntegrator::global_seed());
    else
        ImGui::TextDisabled("Seed: random (hardware)");
    ImGui::TextDisabled("Set seed BEFORE spawning Brownian particles.");
}
```

**Verify:** set seed to 42, spawn a Brownian particle, let it run 5 seconds,
export.  Close.  Reopen, set seed to 42 again, spawn, run 5 seconds, export.
The two CSV files should be identical (or very close — thread_local init order
may differ per platform, which is acceptable for now).

---

## After completing all four items

Update `TODO.md` at the repo root.  Change the status of A1, A2, A3, A4 from
`[ ]` to `[x]`.  Also change C1 to `[x]` since it is the same item as A1.
Change E2 to `[x]` since it is the same item as A2.

Update `refactor_progress.md`:
- Add a new `## Category A -- DONE` section listing what was changed in each file.
- Change `A2`, `A3`, `A4` in the "New items from architecture review" list to DONE.

---

## Key invariants to preserve

- **Never break the build.**  Read before writing.  Compile mentally before committing.
- **`m_walk` in AnimatedCurve is the single source of truth** for particle position.
  After A1, it is a `ParticleState`.  Every place that reads `m_walk.x`/`m_walk.y`
  must become `m_walk.uv.x`/`m_walk.uv.y`.
- **`HistoryBuffer::to_vector()` must preserve chronological order.**
  The `get(i)` remapping `(m_head + i) % m_capacity` already does this — use it.
- **`export_session()` is a const method** — it must not modify any scene state.
- **The RNG seed applies to new `thread_local` instances only.**
  Particles already spawned keep their existing RNG stream.  Document this in
  the UI tooltip ("Set seed BEFORE spawning Brownian particles").

<!-- END PROMPT -->

---

## Usage notes (for you, not for the assistant)

- Paste only the content between BEGIN PROMPT and END PROMPT.
- After pasting, say nothing else — the prompt is self-contained.
- The assistant will read each file before editing it.
- After A1 there will be a compile step; if it fails the assistant will fix it.
- The Export CSV button writes to the working directory of the executable,
  which is typically `F:\repos\Learning-Real-Analysis\nurbs_dde\build\` or
  wherever VS/CLion puts the binary.  Check there for the output file.

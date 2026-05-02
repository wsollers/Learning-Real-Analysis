# Prompt: implement Category C (constraints + Ctrl+A feature)

Paste everything between BEGIN PROMPT and END PROMPT into a new chat.

---
<!-- BEGIN PROMPT -->

You are an expert C++20 software engineer and architect working on **nurbs_dde**,
a real-time Vulkan particle simulation.  You have Filesystem tools to read and
write files directly on the user's machine.  Before touching any file, always
read it first.  Never skip steps.  Never guess at existing code — read it.

The user's repo root is:
```
F:\repos\Learning-Real-Analysis\nurbs_dde\
```

---

## Your task: implement Category C (3 items)

Do the items in this exact order — each one is either a prerequisite for, or
logically cleanest before, the next.

---

## Before you start: mandatory reads

Read these files in full before writing a single line:

1. `src/sim/IEquation.hpp`              — ParticleState, IEquation interface
2. `src/sim/IIntegrator.hpp`            — IIntegrator interface
3. `src/app/AnimatedCurve.hpp`          — AnimatedCurve public interface + private members
4. `src/app/GaussianSurface.cpp`        — the `step()` boundary section (lines 1–80 are enough)
5. `src/sim/GradientWalker.hpp`         — exemplar ODE equation
6. `src/sim/BrownianMotion.hpp`         — exemplar SDE equation
7. `src/sim/DelayPursuitEquation.hpp`   — exemplar DDE equation with periodic wrapping
8. `src/app/SurfaceSimScene.hpp`        — current scene members and enum declarations
9. `src/math/Surfaces.hpp`             — ISurface, IDeformableSurface
10. `docs/ctrl_a_leader_seeker.md`      — full design doc for the Ctrl+A feature

Do not proceed until you have read all ten.

---

## C2 — DomainConfinement: IConstraint interface + concrete implementation

### Why

`AnimatedCurve::step()` in `GaussianSurface.cpp` currently contains ~20 lines of
hard-coded boundary logic — `if/while` blocks that reflect or wrap `ps.uv` after
every integration step.  This is correct but not extensible: it cannot express
elastic bounce with a velocity penalty, "stay on the upper hemisphere", or any
geometry other than a flat rectangle with optional periodic axes.

An `IConstraint` interface lets the boundary policy become a strategy object,
cleanly separated from the integration step.

### Target files

```
src/sim/IConstraint.hpp          new — interface
src/sim/DomainConfinement.hpp    new — concrete: reflect / wrap the existing behaviour
```

Both are header-only.

### IConstraint interface

```cpp
// src/sim/IConstraint.hpp
#pragma once
#include "sim/IEquation.hpp"    // ParticleState
#include "math/Surfaces.hpp"    // ISurface
#include <string>

namespace ndde::sim {

// IConstraint: post-integration correction applied to a ParticleState.
//
// Called by AnimatedCurve::step() AFTER the integrator has advanced uv,
// BEFORE the trail point is pushed.
//
// Contract:
//   apply() may modify state.uv freely.
//   apply() must NOT modify state.phase or state.angle (those are integrator
//   responsibilities).
//   apply() is called once per integration sub-step, not once per frame.
//
// Chained constraints: AnimatedCurve stores a vector<unique_ptr<IConstraint>>.
// They are applied in order.  Each sees the output of the previous.
class IConstraint {
public:
    virtual ~IConstraint() = default;

    // Apply the constraint: modify state.uv so the particle satisfies the
    // constraint after this call.
    virtual void apply(ParticleState&              state,
                       const ndde::math::ISurface& surface) const = 0;

    [[nodiscard]] virtual std::string name() const = 0;
};

} // namespace ndde::sim
```

### DomainConfinement implementation

`DomainConfinement` replaces the existing hard-coded boundary logic verbatim.
Read the current boundary code in `GaussianSurface.cpp` before writing this —
the logic must match exactly.

```cpp
// src/sim/DomainConfinement.hpp
#pragma once
#include "sim/IConstraint.hpp"
#include <cmath>
#include <algorithm>

namespace ndde::sim {

// DomainConfinement: keep the particle within [u_min, u_max] x [v_min, v_max].
//
// Periodic axes: wrap (modular arithmetic).
// Non-periodic axes: reflect (mirror velocity component — approximated by
//   mirroring position around the boundary, which is exact for a straight
//   trajectory and a good approximation for curved ones).
//
// This replicates the existing hard-coded boundary logic in AnimatedCurve::step()
// so that it can be swapped, composed, or overridden.
class DomainConfinement final : public IConstraint {
public:
    void apply(ParticleState&              state,
               const ndde::math::ISurface& surface) const override
    {
        const float u_min = surface.u_min();
        const float u_max = surface.u_max();
        const float v_min = surface.v_min();
        const float v_max = surface.v_max();

        if (surface.is_periodic_u()) {
            const float span = u_max - u_min;
            // Modular wrap: keep uv.x in [u_min, u_max)
            while (state.uv.x <  u_min) state.uv.x += span;
            while (state.uv.x >= u_max) state.uv.x -= span;
        } else {
            // Reflect: mirror position around boundary, clamp to domain
            if (state.uv.x < u_min) state.uv.x = u_min + (u_min - state.uv.x);
            if (state.uv.x > u_max) state.uv.x = u_max - (state.uv.x - u_max);
            state.uv.x = std::clamp(state.uv.x, u_min, u_max);
        }

        if (surface.is_periodic_v()) {
            const float span = v_max - v_min;
            while (state.uv.y <  v_min) state.uv.y += span;
            while (state.uv.y >= v_max) state.uv.y -= span;
        } else {
            if (state.uv.y < v_min) state.uv.y = v_min + (v_min - state.uv.y);
            if (state.uv.y > v_max) state.uv.y = v_max - (state.uv.y - v_max);
            state.uv.y = std::clamp(state.uv.y, v_min, v_max);
        }
    }

    [[nodiscard]] std::string name() const override { return "DomainConfinement"; }
};

} // namespace ndde::sim
```

### Wire into AnimatedCurve

**Step 1 — Update `src/app/AnimatedCurve.hpp`**

Add `#include "sim/IConstraint.hpp"` to the include block.

Add a private member:
```cpp
std::vector<std::unique_ptr<ndde::sim::IConstraint>> m_constraints;
```

Add a public method:
```cpp
// Add a constraint applied after every integration step.
// AnimatedCurve owns the constraint via unique_ptr.
// Constraints are applied in insertion order.
void add_constraint(std::unique_ptr<ndde::sim::IConstraint> c) {
    m_constraints.push_back(std::move(c));
}
```

**Step 2 — Update `GaussianSurface.cpp`: the `step()` method**

Read the `step()` method body in full before editing.  Find the boundary
correction block — the `if/while` statements that adjust `ps.uv` after the
integrator call.  Replace those ~20 lines with:

```cpp
// Apply constraints (boundary handling, pairwise, etc.)
for (const auto& c : m_constraints)
    c->apply(ps, *m_surface);
```

**Step 3 — Update the constructor and `with_equation` factory**

In `GaussianSurface.cpp`, both the standard constructor and `with_equation`
factory must add the default `DomainConfinement` constraint automatically:

```cpp
// At the end of AnimatedCurve::AnimatedCurve(...) constructor body:
m_constraints.push_back(std::make_unique<ndde::sim::DomainConfinement>());

// Same at the end of AnimatedCurve::with_equation(...) factory body:
result.m_constraints.push_back(std::make_unique<ndde::sim::DomainConfinement>());
```

This ensures every particle gets domain confinement by default, with zero changes
at every call site.

**Step 4 — Verify**

Build.  Every existing particle (Leader, Chaser, Brownian, DelayPursuit) must
behave identically — no particles should escape the domain.  The constraint is
doing exactly what the old hard-coded code did.

---

## C3 — Pairwise minimum-distance constraint

### Why

Without it, chasers can pass through leaders in parameter space.  This breaks
the delay-pursuit DDE semantics (the chaser can teleport to the leader's position)
and makes multi-agent pursuit visually degenerate.  A simple minimum-distance
spring-like repulsion prevents overlap.

### Target file

```
src/sim/MinDistConstraint.hpp    new — header-only pairwise constraint
```

### Interface problem: pairwise vs per-particle

`IConstraint::apply()` sees only one `ParticleState`.  A pairwise constraint
needs to see two.  The clean solution is a second interface:

```cpp
// src/sim/IConstraint.hpp — ADD below IConstraint class:

// IPairConstraint: post-integration correction applied to a pair of particles.
//
// Called by SurfaceSimScene::apply_pairwise_constraints() AFTER all per-particle
// IConstraint::apply() calls, once per distinct ordered pair (i, j) with i < j.
// Both states may be modified.
class IPairConstraint {
public:
    virtual ~IPairConstraint() = default;

    virtual void apply(ParticleState&              a,
                       ParticleState&              b,
                       const ndde::math::ISurface& surface) const = 0;

    [[nodiscard]] virtual std::string name() const = 0;
};
```

### MinDistConstraint implementation

```cpp
// src/sim/MinDistConstraint.hpp
#pragma once
#include "sim/IConstraint.hpp"   // IPairConstraint
#include <glm/glm.hpp>
#include <cmath>

namespace ndde::sim {

// MinDistConstraint: push two particles apart if they are closer than min_dist
// in parameter space.
//
// Collision resolution: elastic midpoint push.  Each particle is moved halfway
// toward min_dist along the separation vector.  This conserves the centre of
// mass in parameter space and is stable for small min_dist.
//
// Periodic wrapping: the shortest-path delta is used on periodic axes so the
// constraint works correctly on the torus.
//
// min_dist units: parameter-space units (same as uv coordinates).
//   For the Gaussian surface (u,v) = (x,y) in world metres: min_dist in metres.
//   For the torus (u,v) in radians: min_dist in radians.
class MinDistConstraint final : public IPairConstraint {
public:
    explicit MinDistConstraint(float min_dist = 0.3f) : m_min_dist(min_dist) {}

    void apply(ParticleState&              a,
               ParticleState&              b,
               const ndde::math::ISurface& surface) const override
    {
        glm::vec2 delta = b.uv - a.uv;

        // Shortest-path wrap for periodic surfaces
        if (surface.is_periodic_u()) {
            const float span = surface.u_max() - surface.u_min();
            if (delta.x >  span * 0.5f) delta.x -= span;
            if (delta.x < -span * 0.5f) delta.x += span;
        }
        if (surface.is_periodic_v()) {
            const float span = surface.v_max() - surface.v_min();
            if (delta.y >  span * 0.5f) delta.y -= span;
            if (delta.y < -span * 0.5f) delta.y += span;
        }

        const float dist = glm::length(delta);
        if (dist >= m_min_dist || dist < 1e-7f) return;

        // Push each particle half the overlap distance along the separation axis
        const float overlap  = m_min_dist - dist;
        const glm::vec2 push = (delta / dist) * (overlap * 0.5f);
        b.uv += push;
        a.uv -= push;
    }

    [[nodiscard]] std::string name() const override { return "MinDist"; }

    float min_dist() const noexcept { return m_min_dist; }
    void  set_min_dist(float d) noexcept { m_min_dist = d; }

private:
    float m_min_dist;
};

} // namespace ndde::sim
```

### Wire into SurfaceSimScene

**Step 1 — Update `src/app/SurfaceSimScene.hpp`**

Add includes:
```cpp
#include "sim/IConstraint.hpp"
#include "sim/MinDistConstraint.hpp"
```

Add members:
```cpp
// Pairwise constraints applied after per-particle step.
// Applied once per ordered pair (i, j) with i < j.
std::vector<std::unique_ptr<ndde::sim::IPairConstraint>> m_pair_constraints;
bool m_pair_collision = false;    ///< UI toggle for MinDistConstraint
float m_min_dist      = 0.3f;    ///< UI-editable minimum distance
```

Add private method declaration:
```cpp
void apply_pairwise_constraints();
```

**Step 2 — Implement `apply_pairwise_constraints()` in `SurfaceSimScene.cpp`**

```cpp
void SurfaceSimScene::apply_pairwise_constraints() {
    const std::size_t n = m_curves.size();
    for (std::size_t i = 0; i < n; ++i) {
        for (std::size_t j = i + 1; j < n; ++j) {
            // Access the mutable ParticleState through a friend or accessor.
            // NOTE: ParticleState is private in AnimatedCurve — see Step 3.
            for (const auto& pc : m_pair_constraints)
                pc->apply(m_curves[i].walk_state(),
                           m_curves[j].walk_state(),
                           *m_surface);
        }
    }
}
```

**Step 3 — Expose `walk_state()` accessor on AnimatedCurve**

`apply_pairwise_constraints()` needs mutable access to the `ParticleState` of
each curve.  Add a non-const accessor to `AnimatedCurve.hpp`:

```cpp
// Mutable access for pairwise constraint application.
// Called only by SurfaceSimScene::apply_pairwise_constraints().
// Do not use from equation or integrator code.
[[nodiscard]] ndde::sim::ParticleState& walk_state() noexcept { return m_walk; }
```

**Step 4 — Call `apply_pairwise_constraints()` in `advance_simulation()`**

In `SurfaceSimScene.cpp`, in `advance_simulation()`, after the per-particle
advance loop and before the history push:

```cpp
// Apply pairwise constraints (e.g. min-distance collision avoidance)
if (!m_pair_constraints.empty())
    apply_pairwise_constraints();
```

**Step 5 — Wire the UI toggle in `draw_simulation_panel()`**

In the "Overlays" or a new "Collision" section:

```cpp
ImGui::SeparatorText("Collision avoidance");
bool changed = ImGui::Checkbox("Min-distance push##col", &m_pair_collision);
if (m_pair_collision) {
    changed |= ImGui::SliderFloat("Min dist##col", &m_min_dist, 0.05f, 2.f, "%.2f");
}
if (changed) {
    m_pair_constraints.clear();
    if (m_pair_collision)
        m_pair_constraints.push_back(
            std::make_unique<ndde::sim::MinDistConstraint>(m_min_dist));
}
```

**Step 6 — Verify**

Build.  Spawn multiple particles close together (Ctrl+L several times).  With
collision enabled, particles should not overlap.  Without it, they should pass
through as before.

---

## C4 — Ctrl+A: ExtremumSurface + LeaderSeeker + BiasedBrownianLeader + pursuit

### Why

This is the primary new simulation feature: a leader particle that navigates
between the global maximum and minimum of a surface, with pursuit particles
chasing it using different bearing strategies.  The full design is in
`docs/ctrl_a_leader_seeker.md` — read it completely before starting.

### Order of implementation

Do the sub-items in this exact sequence. Each builds on the previous.

---

#### C4a — `ExtremumTable` and `ExtremumSurface`

**C4a-1 — Create `src/math/ExtremumTable.hpp`**

```cpp
#pragma once
// math/ExtremumTable.hpp
// ExtremumTable: cached locations of the global max and min of a surface.
// Built by a grid search followed by gradient refinement.
// Owned by SurfaceSimScene as a value member (stable address).
// Non-owning pointers to it are held by LeaderSeekerEquation and
// BiasedBrownianLeader — safe because the scene outlives all particles.

#include <glm/glm.hpp>

namespace ndde::math {

struct ExtremumTable {
    glm::vec2 max_uv = {0.f, 0.f};  ///< parameter-space location of global max
    float     max_z  = 0.f;          ///< f(max_uv)
    glm::vec2 min_uv = {0.f, 0.f};  ///< parameter-space location of global min
    float     min_z  = 0.f;          ///< f(min_uv)
    bool      valid  = false;         ///< false until first build

    // Build the table for any ISurface using a grid search + gradient refinement.
    // grid_n: number of grid points per axis (default 64 — O(4096) evaluations).
    // Called synchronously on surface swap; cheap enough.
    void build(const class ISurface& surface, float t = 0.f, u32 grid_n = 64u);

    // Invalidate (e.g. before rebuild on deforming surface).
    void invalidate() noexcept { valid = false; }
};

} // namespace ndde::math
```

**C4a-2 — Create `src/math/ExtremumTable.cpp`**

Implement `ExtremumTable::build()`:

```cpp
#include "math/ExtremumTable.hpp"
#include "math/Surfaces.hpp"
#include <limits>

namespace ndde::math {

void ExtremumTable::build(const ISurface& surface, float t, u32 grid_n) {
    valid = false;

    const float u0 = surface.u_min(t), u1 = surface.u_max(t);
    const float v0 = surface.v_min(t), v1 = surface.v_max(t);
    const float du = (u1 - u0) / static_cast<float>(grid_n);
    const float dv = (v1 - v0) / static_cast<float>(grid_n);

    float best_max = -std::numeric_limits<float>::max();
    float best_min =  std::numeric_limits<float>::max();

    // Grid search
    for (u32 i = 0; i <= grid_n; ++i) {
        for (u32 j = 0; j <= grid_n; ++j) {
            const float u = u0 + static_cast<float>(i) * du;
            const float v = v0 + static_cast<float>(j) * dv;
            const float z = surface.evaluate(u, v, t).z;
            if (z > best_max) { best_max = z; max_uv = {u, v}; max_z = z; }
            if (z < best_min) { best_min = z; min_uv = {u, v}; min_z = z; }
        }
    }

    // Gradient refinement (20 steps of gradient ascent/descent from candidate)
    constexpr float step = 0.02f;
    constexpr int   iters = 20;

    // Refine max
    {
        glm::vec2 p = max_uv;
        for (int k = 0; k < iters; ++k) {
            const Vec3 du_v = surface.du(p.x, p.y, t);
            const Vec3 dv_v = surface.dv(p.x, p.y, t);
            p.x = std::clamp(p.x + step * du_v.z, u0, u1);
            p.y = std::clamp(p.y + step * dv_v.z, v0, v1);
        }
        const float z = surface.evaluate(p.x, p.y, t).z;
        if (z > max_z) { max_uv = p; max_z = z; }
    }

    // Refine min
    {
        glm::vec2 p = min_uv;
        for (int k = 0; k < iters; ++k) {
            const Vec3 du_v = surface.du(p.x, p.y, t);
            const Vec3 dv_v = surface.dv(p.x, p.y, t);
            p.x = std::clamp(p.x - step * du_v.z, u0, u1);
            p.y = std::clamp(p.y - step * dv_v.z, v0, v1);
        }
        const float z = surface.evaluate(p.x, p.y, t).z;
        if (z < min_z) { min_uv = p; min_z = z; }
    }

    valid = true;
}

} // namespace ndde::math
```

Add `math/ExtremumTable.cpp` to the `ndde_math` STATIC library in
`src/CMakeLists.txt`.

**C4a-3 — Create `src/math/ExtremumSurface.hpp`**

A concrete ISurface: bimodal Gaussian height field with one peak and one pit.
Analytic gradient so `du()` and `dv()` are exact.

```cpp
#pragma once
// math/ExtremumSurface.hpp
// ExtremumSurface: bimodal Gaussian height field designed so that the
// ExtremumTable always finds a unique, well-separated global max and min.
//
// f(u,v) = A1 * exp(-r1^2 / (2*s1^2))   <- positive peak
//         - A2 * exp(-r2^2 / (2*s2^2))   <- negative pit
// where r1 = ||(u,v) - peak_centre||, r2 = ||(u,v) - pit_centre||
//
// Domain: [-5, 5] x [-5, 5] (non-periodic).
// The peak is placed at (+2, +2), the pit at (-2, -2) by default.

#include "math/Surfaces.hpp"
#include "math/Scalars.hpp"
#include <cmath>
#include <glm/glm.hpp>

namespace ndde::math {

class ExtremumSurface final : public ISurface {
public:
    struct Params {
        glm::vec2 peak_centre = { 2.f,  2.f};
        float     peak_amp    = 2.f;
        float     peak_sigma  = 1.8f;
        glm::vec2 pit_centre  = {-2.f, -2.f};
        float     pit_amp     = 1.8f;   // applied as negative
        float     pit_sigma   = 1.6f;
        float     extent      = 5.f;    // domain = [-extent, extent]^2
    };

    explicit ExtremumSurface(Params p = {}) : m_p(p) {}

    [[nodiscard]] Vec3 evaluate(float u, float v, float /*t*/) const override {
        return Vec3{ u, v, height(u, v) };
    }

    [[nodiscard]] Vec3 du(float u, float v, float /*t*/) const override {
        return Vec3{ 1.f, 0.f, dh_du(u, v) };
    }
    [[nodiscard]] Vec3 dv(float u, float v, float /*t*/) const override {
        return Vec3{ 0.f, 1.f, dh_dv(u, v) };
    }

    [[nodiscard]] float u_min(float = 0.f) const override { return -m_p.extent; }
    [[nodiscard]] float u_max(float = 0.f) const override { return  m_p.extent; }
    [[nodiscard]] float v_min(float = 0.f) const override { return -m_p.extent; }
    [[nodiscard]] float v_max(float = 0.f) const override { return  m_p.extent; }

    [[nodiscard]] bool is_periodic_u() const override { return false; }
    [[nodiscard]] bool is_periodic_v() const override { return false; }

    Params&       params()       noexcept { return m_p; }
    const Params& params() const noexcept { return m_p; }

private:
    Params m_p;

    [[nodiscard]] float height(float u, float v) const noexcept {
        const float du1 = u - m_p.peak_centre.x;
        const float dv1 = v - m_p.peak_centre.y;
        const float du2 = u - m_p.pit_centre.x;
        const float dv2 = v - m_p.pit_centre.y;
        const float r1sq = du1*du1 + dv1*dv1;
        const float r2sq = du2*du2 + dv2*dv2;
        const float s1   = 2.f * m_p.peak_sigma * m_p.peak_sigma;
        const float s2   = 2.f * m_p.pit_sigma  * m_p.pit_sigma;
        return  m_p.peak_amp * std::exp(-r1sq / s1)
              - m_p.pit_amp  * std::exp(-r2sq / s2);
    }

    // df/du
    [[nodiscard]] float dh_du(float u, float v) const noexcept {
        const float du1 = u - m_p.peak_centre.x;
        const float dv1 = v - m_p.peak_centre.y;
        const float du2 = u - m_p.pit_centre.x;
        const float dv2 = v - m_p.pit_centre.y;
        const float s1  = 2.f * m_p.peak_sigma * m_p.peak_sigma;
        const float s2  = 2.f * m_p.pit_sigma  * m_p.pit_sigma;
        return (-2.f * du1 / s1) * m_p.peak_amp * std::exp(-(du1*du1+dv1*dv1)/s1)
             + ( 2.f * du2 / s2) * m_p.pit_amp  * std::exp(-(du2*du2+dv2*dv2)/s2);
    }

    // df/dv
    [[nodiscard]] float dh_dv(float u, float v) const noexcept {
        const float du1 = u - m_p.peak_centre.x;
        const float dv1 = v - m_p.peak_centre.y;
        const float du2 = u - m_p.pit_centre.x;
        const float dv2 = v - m_p.pit_centre.y;
        const float s1  = 2.f * m_p.peak_sigma * m_p.peak_sigma;
        const float s2  = 2.f * m_p.pit_sigma  * m_p.pit_sigma;
        return (-2.f * dv1 / s1) * m_p.peak_amp * std::exp(-(du1*du1+dv1*dv1)/s1)
             + ( 2.f * dv2 / s2) * m_p.pit_amp  * std::exp(-(du2*du2+dv2*dv2)/s2);
    }
};

} // namespace ndde::math
```

---

#### C4b — Leader equations

**C4b-1 — Create `src/sim/LeaderSeekerEquation.hpp`**

Header-only deterministic state machine.  Read the full design in
`docs/ctrl_a_leader_seeker.md` (section "The deterministic leader") before writing.

The class must:
- Include `math/ExtremumTable.hpp`
- Hold `const ndde::math::ExtremumTable* m_table` (non-owning)
- Hold `mutable Goal m_goal = Goal::SeekMax` (mutable for state machine in const velocity())
- Implement `velocity()` following the 6-step logic in the design doc exactly
- Implement `noise_coefficient()` returning `{m_p.noise_sigma, m_p.noise_sigma}`
- Apply periodic shortest-path wrapping when computing delta (copy the pattern from
  `DelayPursuitEquation`)
- Return `name()` = `"LeaderSeeker"`

```cpp
// Key types to define inside the header:
enum class Goal { SeekMax, SeekMin };

struct Params {
    float target_grad_magnitude = 0.f;
    float epsilon               = 0.1f;
    float pursuit_speed         = 0.8f;
    float noise_sigma           = 0.15f;
    float arrival_radius        = 0.4f;
};
```

**C4b-2 — Create `src/sim/BiasedBrownianLeader.hpp`**

Header-only stochastic leader.  Read the full design in
`docs/ctrl_a_leader_seeker.md` (section "The stochastic leader") before writing.

The `velocity()` implementation from the design doc is complete and correct —
copy it verbatim.  The class must:
- Hold `const ndde::math::ExtremumTable* m_table` (non-owning)
- Hold `mutable Goal m_goal = Goal::SeekMax`
- Implement `noise_coefficient()` returning `{m_p.sigma, m_p.sigma}`
- Apply periodic shortest-path wrapping
- Support the optional `gradient_drift` parameter
- Return `name()` = `"BiasedBrownianLeader"`

`Goal` enum is the same as in `LeaderSeekerEquation`.  Put it in a shared
internal header or define it in both files (it's a 2-value enum, duplication
is acceptable at this scope).

---

#### C4c — Pursuit equations

**C4c-1 — Create `src/sim/DirectPursuitEquation.hpp`**

Strategy A: the pursuer always knows the leader's current position.

```cpp
// src/sim/DirectPursuitEquation.hpp
#pragma once
#include "sim/IEquation.hpp"
#include <glm/glm.hpp>
#include <cmath>

namespace ndde::sim {

// DirectPursuitEquation: steer toward the leader's current parameter-space
// position at full pursuit_speed.  Applies periodic shortest-path wrapping.
//
// The leader is identified by a non-owning pointer to its AnimatedCurve.
// The scene is responsible for keeping the leader alive while chasers using
// this equation are active.
class DirectPursuitEquation final : public IEquation {
public:
    struct Params {
        float pursuit_speed = 0.8f;
        float noise_sigma   = 0.0f;
    };

    // leader_uv: pointer to a function that returns the leader's current uv.
    // In practice: &m_curves[0], calling head_uv() each step.
    // Pass as a std::function<glm::vec2()> for type safety.
    explicit DirectPursuitEquation(std::function<glm::vec2()> leader_uv_fn,
                                   Params p = {})
        : m_leader_uv_fn(std::move(leader_uv_fn)), m_p(p) {}

    [[nodiscard]] glm::vec2 velocity(
        ParticleState&              state,
        const ndde::math::ISurface& surface,
        float                       /*t*/) const override
    {
        const glm::vec2 target = m_leader_uv_fn();
        glm::vec2 delta = target - state.uv;

        if (surface.is_periodic_u()) {
            const float span = surface.u_max() - surface.u_min();
            if (delta.x >  span * 0.5f) delta.x -= span;
            if (delta.x < -span * 0.5f) delta.x += span;
        }
        if (surface.is_periodic_v()) {
            const float span = surface.v_max() - surface.v_min();
            if (delta.y >  span * 0.5f) delta.y -= span;
            if (delta.y < -span * 0.5f) delta.y += span;
        }

        const float dist = glm::length(delta);
        if (dist < 1e-7f) return {0.f, 0.f};
        return (delta / dist) * m_p.pursuit_speed;
    }

    [[nodiscard]] glm::vec2 noise_coefficient(const ParticleState&,
                                              const ndde::math::ISurface&,
                                              float) const override
    { return { m_p.noise_sigma, m_p.noise_sigma }; }

    [[nodiscard]] float phase_rate() const override { return 0.f; }
    [[nodiscard]] std::string name() const override { return "DirectPursuit"; }

    Params& params() noexcept { return m_p; }

private:
    std::function<glm::vec2()> m_leader_uv_fn;
    Params m_p;
};

} // namespace ndde::sim
```

Note: `#include <functional>` is required. Add it.

**C4c-2 — Strategy B uses `DelayPursuitEquation` unchanged.**  No new file needed.

**C4c-3 — Create `src/sim/MomentumBearingEquation.hpp`**

Strategy C: infer the leader's goal direction from a moving average of its
recent velocity samples.

```cpp
// The leader's position history (HistoryBuffer) is queried over a time
// window to compute the average velocity direction.
// window_sec: the width of the averaging window.
// The result is normalised and scaled to pursuit_speed.

struct Params {
    float pursuit_speed = 0.8f;
    float window_sec    = 1.5f;  // averaging window in seconds
    float noise_sigma   = 0.f;
};
```

Implementation: query `history->query(t - window_sec)` and
`history->query(t)`, take the difference, normalise, scale by `pursuit_speed`.
Apply periodic shortest-path wrapping to the velocity vector before normalising.

---

#### C4d — Wire into SurfaceSimScene

**Read `src/app/SurfaceSimScene.hpp` again before making these edits** — it has
changed since the mandatory reads at the top.

**Step 1 — New includes in `SurfaceSimScene.hpp`**

```cpp
#include "math/ExtremumTable.hpp"
#include "math/ExtremumSurface.hpp"
#include "sim/LeaderSeekerEquation.hpp"
#include "sim/BiasedBrownianLeader.hpp"
#include "sim/DirectPursuitEquation.hpp"
#include "sim/MomentumBearingEquation.hpp"
```

**Step 2 — New members in `SurfaceSimScene` (private)**

```cpp
// Ctrl+A: ExtremumSurface + leader seeker
ndde::math::ExtremumTable          m_extremum_table;  // stable address (value member)
u32 m_extremum_rebuild_countdown = 0;                 // ticks until next rebuild

// Leader mode
enum class LeaderMode : u8 { Deterministic = 0, StochasticBiased = 1 };
LeaderMode m_leader_mode = LeaderMode::Deterministic;
ndde::sim::LeaderSeekerEquation::Params m_ls_params;
ndde::sim::BiasedBrownianLeader::Params m_bbl_params;

// Pursuit mode
enum class PursuitMode : u8 { Direct = 0, Delayed = 1, Momentum = 2 };
PursuitMode m_pursuit_mode  = PursuitMode::Direct;
float       m_pursuit_tau   = 1.5f;   // for Strategy B
float       m_pursuit_window = 1.5f;  // for Strategy C

bool m_ctrl_a_prev = false;           // edge detection
bool m_spawning_pursuer = false;      // true after first Ctrl+A, false resets
```

Add the enum value to `SurfaceType`:
```cpp
enum class SurfaceType : u8 { Gaussian = 0, Torus = 1, GaussianRipple = 2, Extremum = 3 };
```

**Step 3 — New private methods in `SurfaceSimScene.hpp`**

```cpp
void spawn_leader_seeker();
void spawn_pursuit_particle();
void rebuild_extremum_table_if_needed();
void draw_leader_seeker_panel();     // called from draw_simulation_panel()
```

**Step 4 — Implement in `SurfaceSimScene.cpp`**

`rebuild_extremum_table_if_needed()`:
- If `m_extremum_rebuild_countdown == 0` and surface is `ExtremumSurface`:
  call `m_extremum_table.build(*m_surface, m_sim_time)`
  reset counter to 30
- Decrement counter each frame in `advance_simulation()`

`spawn_leader_seeker()`:
- Follow the design doc `spawn_leader_seeker()` code exactly
- Uses `spawn::spawn_owned()` from `SpawnStrategy`
- Sets `m_spawning_pursuer = true` after spawning the leader

`spawn_pursuit_particle()`:
- Checks `!m_curves.empty()` (leader must exist)
- Creates the appropriate equation based on `m_pursuit_mode`:
  - Direct: `std::make_unique<DirectPursuitEquation>([this]{ return m_curves[0].head_uv(); }, ...)`
  - Delayed: `std::make_unique<DelayPursuitEquation>(m_curves[0].history(), m_surface.get(), ...)`
  - Momentum: `std::make_unique<MomentumBearingEquation>(m_curves[0].history(), ...)`
- Ensures leader history is enabled (same as Ctrl+R pattern)
- Spawns with `spawn::spawn_owned(..., prewarm=false)`

Ctrl+A hotkey in `handle_hotkeys()`:
```cpp
const bool ctrl_a = io.KeyCtrl && ImGui::IsKeyDown(ImGuiKey_A);
if (ctrl_a && !m_ctrl_a_prev) {
    if (!m_spawning_pursuer)
        spawn_leader_seeker();
    else
        spawn_pursuit_particle();
}
m_ctrl_a_prev = ctrl_a;
```

`swap_surface()` case for `SurfaceType::Extremum`:
```cpp
case SurfaceType::Extremum:
    m_surface = std::make_unique<ndde::math::ExtremumSurface>();
    m_extremum_table.build(*m_surface);
    break;
```

**Step 5 — Add radio button to `draw_simulation_panel()`**

In the surface selector block:
```cpp
ImGui::SameLine();
changed |= ImGui::RadioButton("Extremum##surf", &sel, 3);
```

**Step 6 — `draw_leader_seeker_panel()`**

Implement the UI from the design doc's "Panel UI additions" section.
Call it from `draw_simulation_panel()` with:
```cpp
draw_leader_seeker_panel();
```

Also add to hotkey panel in `draw_hotkey_panel()`:
```cpp
row("Ctrl+A", "Spawn leader / pursuer (LeaderSeeker)");
```

**Step 7 — Add `m_ctrl_a_prev` to hotkey comment in SurfaceSimScene.hpp**

Update the hotkey list comment to include:
```
//   Ctrl+A  -- spawn leader seeker (first press) / pursuit particle (subsequent)
```

---

## After completing all three items

### Update CMakeLists.txt

Read `src/CMakeLists.txt` before editing.  Add `math/ExtremumTable.cpp` to
the `ndde_math` STATIC library source list.  No other `.cpp` files are added
(all other new files are header-only).

### Update TODO.md

Read `F:\repos\Learning-Real-Analysis\nurbs_dde\TODO.md`.
Change C2, C3, C4 from `[ ]` to `[x]`.

### Update refactor_progress.md

Read `F:\repos\Learning-Real-Analysis\nurbs_dde\refactor_progress.md`.
Add a `## Category C -- DONE` section listing:
- New files created
- Existing files modified
- The key invariants (DomainConfinement is the default constraint for all particles;
  ExtremumTable address is stable as a scene value member; Ctrl+A first press spawns
  leader, subsequent presses spawn pursuers)

---

## Key invariants to preserve throughout

**DomainConfinement is always-on.**  Every `AnimatedCurve` must have a
`DomainConfinement` constraint added in its constructor and `with_equation`
factory.  No particle should ever escape the domain.

**Periodic wrapping in constraints must match the surface.**  `DomainConfinement`
reads `is_periodic_u/v()` and `u_min/max/v_min/max()` from the surface — not
hardcoded values.  `MinDistConstraint` uses the same shortest-path pattern as
`DelayPursuitEquation`.

**`ExtremumTable` address is invariant.**  It is a value member of
`SurfaceSimScene`, not a `unique_ptr`.  All equation pointers to it remain valid
across `swap_surface()` and `m_curves` vector reallocations.

**`m_goal` is `mutable` in both leader equations.**  This is intentional — the
equations are const-correct at the `IEquation` interface level, but the
goal-switching state is owned by the equation instance.  The pattern is identical
to `GradientWalker::state.angle`.

**Leader must exist before pursuers.**  `spawn_pursuit_particle()` must check
`m_curves.empty()` and do nothing if there is no leader.  History must be enabled
on the leader before any delayed or momentum pursuer spawns.

**Ctrl+A is a two-phase hotkey.**  First press: spawn leader (and set
`m_spawning_pursuer = true`).  All subsequent presses: spawn pursuers.
`m_spawning_pursuer` resets to `false` on `Clear all` and `swap_surface()`.

<!-- END PROMPT -->

---

## Usage notes (for you, not for the assistant)

- Paste only the content between BEGIN PROMPT and END PROMPT.
- The assistant reads all ten mandatory files before writing anything.
- Do C2 first (constraints) — it changes `AnimatedCurve::step()` which C4
  also depends on implicitly.
- After C2: build and verify no particles escape the domain.
- After C3: test collision avoidance by spawning several leaders close together.
- After C4: test the full Ctrl+A workflow: switch to Extremum surface, Ctrl+A
  to spawn leader, watch it navigate, Ctrl+A again to spawn a pursuer.
- The `ExtremumTable.cpp` is the only new `.cpp` file — add it to `ndde_math`
  in CMakeLists.txt, not to `nurbs_dde`.

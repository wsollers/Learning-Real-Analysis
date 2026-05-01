# Ctrl+A — LeaderSeeker: extremum-hunting leader with pursuit particles

## What this feature does

Pressing Ctrl+A spawns a **leader particle** governed by a new equation,
`LeaderSeekerEquation`, on a new surface type, `ExtremumSurface`.  The leader
is a state machine: it climbs toward the global maximum of `f(x,y)`, pauses when
the gradient is nearly flat, then switches goal to the global minimum, and so on,
with Brownian noise perturbing its path throughout.

Pressing Ctrl+A again spawns **pursuit particles** at random locations on the
surface.  Each pursuer runs a different bearing strategy toward the leader, and the
strategy is selected at spawn time so different runs can compare behaviours.

---

## Mathematical setup

### The surface: `ExtremumSurface`

`ExtremumSurface` is a new concrete `ISurface` — a height-field graph
`p(u,v) = (u, v, f(u,v))` chosen to have a unique global maximum and a unique
global minimum inside the domain.  A canonical choice is a bimodal Gaussian:

```
f(u,v) = A1 * exp(-((u-u1)^2 + (v-v1)^2) / (2*s1^2))
        - A2 * exp(-((u-u2)^2 + (v-v2)^2) / (2*s2^2))
```

The positive Gaussian creates a single peak (the max); the negative Gaussian
creates a single pit (the min).  Both have analytic gradients, so `du` and `dv`
are exact.

**Why require a well-posed surface?**  The leader's state machine needs a
meaningful `max_uv` and `min_uv` to steer toward.  A surface with multiple
local maxima of equal height would produce ambiguous navigation.  The caller
is responsible for supplying a surface that satisfies this; the system flags a
warning at construction time if the sampled extrema are degenerate.

### The extremum lookup table

Every time a surface is **constructed or deformed** (i.e., whenever
`swap_surface()` is called, or when a deforming surface's `advance(dt)` produces
a geometry change that might shift the extrema), the extremum table is rebuilt:

```
struct ExtremumTable {
    glm::vec2 max_uv;    // parameter-space location of the global max
    float     max_z;     // f(max_uv)
    glm::vec2 min_uv;    // parameter-space location of the global min
    float     min_z;     // f(min_uv)
    bool      valid;
};
```

**Building the table** — a grid search over the parameter domain at resolution
`(N x N)` where `N = 64` by default finds the candidate extrema, then a short
gradient ascent/descent refines each to sub-pixel accuracy.  The whole operation
is O(N²) = O(4096) evaluations — cheap enough to run synchronously on surface
swap.

For `IDeformableSurface` the table is rebuilt at a lower rate (every `k` frames,
configurable) since a deforming surface's extrema drift slowly.

**Why a lookup table and not just re-search every step?**  The leader queries
`max_uv` and `min_uv` every frame.  A full grid search every frame at 60fps and
4096 evaluations each would cost ~245k `f(x,y)` calls per second — negligible
for a simple polynomial surface but wasteful.  Caching the result and invalidating
only on geometry change is the correct pattern.

---

## The leader: `LeaderSeekerEquation`

### State machine

```
enum class Goal { SeekMax, SeekMin };
```

The equation carries one mutable field, `m_goal`, which starts as `SeekMax`.

**Transition condition** — the goal flips when the gradient magnitude at the
current position falls below a threshold:

```
|∇f(u,v)| < epsilon
```

where `epsilon` defaults to `0.1` and is exposed as a UI slider so it can be
adjusted between runs without recompiling.  The condition `|f'(x,y) - 1| < 0.1`
from the original spec is a special case: it measures how close the gradient
magnitude is to 1 rather than to 0.  Both are supported as variants of the same
slider — a `target_grad_magnitude` parameter whose default is `0.0` (flat) and
whose threshold half-width is `epsilon`.

```cpp
struct Params {
    float target_grad_magnitude = 0.f;  // 0 = flat summit/pit, 1 = unit-gradient contour
    float epsilon               = 0.1f; // half-width of the flatness band
    float pursuit_speed         = 0.8f; // approach speed toward extremum (param-units/s)
    float noise_sigma           = 0.15f;// Brownian perturbation
};

bool near_target_gradient(float grad_mag) const {
    return std::abs(grad_mag - m_p.target_grad_magnitude) < m_p.epsilon;
}
```

### `velocity()` logic

```
1. query ExtremumTable -> goal_uv  (max_uv if SeekMax, min_uv if SeekMin)
2. delta = goal_uv - state.uv       (wrapping if periodic, straight if not)
3. dist  = |delta|
4. if dist < arrival_radius: flip goal  (already there — transition early)
5. bearing = delta / dist * pursuit_speed
6. if near_target_gradient(grad_mag): flip goal
7. return bearing
```

`noise_coefficient()` returns `{noise_sigma, noise_sigma}`, so the
`MilsteinIntegrator` adds Brownian kicks that prevent the leader from getting
stuck on a ridge line on the way to the extremum.

### Why Milstein for the leader?

The leader uses noise to avoid degeneracy.  For constant `noise_sigma` the
Milstein correction is zero (identical to Euler-Maruyama), so there is no
computational overhead.  But once curvature-adaptive noise is added — making
`sigma` vary with `K(u,v)` — Milstein provides the correct strong-order-1 path,
which matters for a particle whose behaviour is being visually inspected.

---

## The pursuit particles: three bearing strategies

Each strategy is a distinct `IEquation` implementation.  The strategy is chosen
at spawn time by a dropdown in the Simulation panel ("Pursuit mode"), so
different runs can be compared without restarting.

### Strategy A — Direct bearing

```cpp
// DirectPursuitEquation
velocity(state, surface, t):
    target = leader.head_uv()           // current leader position in (u,v)
    delta  = shortest_path(target, state.uv, surface)
    return (delta / |delta|) * speed
```

The shortest-path wrapping from `DelayPursuitEquation` is reused verbatim.
This is the baseline: the pursuer always knows where the leader is right now.

### Strategy B — Delayed bearing (n steps ago)

```cpp
// DelayedBearingEquation
velocity(state, surface, t):
    t_past = t - tau                    // tau seconds of delay
    target = leader.history().query(t_past)
    delta  = shortest_path(target, state.uv, surface)
    return (delta / |delta|) * speed
```

This reuses `HistoryBuffer` directly — the same mechanism as
`DelayPursuitEquation`.  The difference from the existing delay-pursuit is that
this pursuer targets the *leader seeker* rather than a generic curve 0.

**Adjustable parameter**: `tau` in seconds (maps to `n` steps via
`tau = n * sim_dt`).  Exposed as a slider: "Delay (s)".

**Geometric note**: as `tau` grows relative to the leader's transit time between
extrema, the pursuer's target lags behind by more and more.  At `tau` close to
the transit time the pursuer is heading toward the previous extremum while the
leader has already reached the next one.  This creates the characteristic spirals
visible in multi-agent delay-pursuit on bounded domains.

### Strategy C — Averaged-history bearing (momentum extrapolation)

This strategy does not steer toward a past *position* of the leader.  Instead
it estimates the leader's recent *velocity vector* by differencing recent history
entries, averages that velocity over a short window, and steers in the direction
that velocity is pointing — i.e., toward where the leader is *going*, not where
it was.

```cpp
// MomentumBearingEquation
velocity(state, surface, t):
    // collect k history samples spanning window_sec seconds
    v_avg = {0, 0}
    for i in 0..k-1:
        p0 = history.query(t - (i+1)*sample_dt)
        p1 = history.query(t - i    *sample_dt)
        v_avg += shortest_path(p1, p0, surface) / sample_dt
    v_avg /= k

    // steer in the direction the leader is moving
    if |v_avg| < 1e-5: return {0,0}   // leader is stationary
    return (v_avg / |v_avg|) * speed
```

**Adjustable parameters**:
- `window_sec`: how far back the average looks (default 0.5 s)
- `k`: number of samples in the average (default 8)
- `speed`: pursuit speed scalar

**Why this is interesting**: if the leader is heading toward the maximum in a
straight line, Strategy C points the pursuer at the maximum too — without
needing to know where the maximum is.  The pursuer is inferring the leader's
*goal* from its *behaviour*.  If the leader has just flipped goal and is now
heading to the minimum, the pursuer's averaged bearing will lag by `window_sec`
before correcting — a kind of informational inertia.

---

## Implementation plan

### New files

```
src/math/ExtremumSurface.hpp      — bimodal Gaussian ISurface + analytic grad
src/math/ExtremumTable.hpp        — ExtremumTable struct + grid-search builder
src/sim/LeaderSeekerEquation.hpp  — state-machine IEquation
src/sim/DirectPursuitEquation.hpp — Strategy A
src/sim/MomentumBearingEquation.hpp — Strategy C
                                     (Strategy B reuses DelayPursuitEquation)
```

`ExtremumTable` is built by a free function:

```cpp
namespace ndde::math {
ExtremumTable build_extremum_table(const ISurface& surface,
                                   u32 grid_n = 64,
                                   float t    = 0.f);
}
```

### Changes to existing files

#### `SurfaceSimScene.hpp`

```cpp
// New members
SurfaceType    m_surface_type;        // gains ExtremumSurface = 3
ExtremumTable  m_extremum_table;      // cached extrema, rebuilt on swap
LeaderSeekerEquation::Params m_ls_params;
PursuitMode    m_pursuit_mode;        // enum {Direct, Delayed, Momentum}
float          m_pursuit_tau;         // delay for Strategy B
float          m_pursuit_window;      // window for Strategy C
bool           m_ctrl_a_prev;

// New methods
void rebuild_extremum_table();
void spawn_leader_seeker();           // Ctrl+A, first press
void spawn_pursuit_particle();        // Ctrl+A, subsequent presses
```

`rebuild_extremum_table()` is called inside `swap_surface()` after the surface
pointer is updated, and also once per `k` frames when the active surface
`is_time_varying()`.

#### `SurfaceSimScene.cpp` — `handle_hotkeys()`

```cpp
const bool ctrl_a = io.KeyCtrl && ImGui::IsKeyDown(ImGuiKey_A);
if (ctrl_a && !m_ctrl_a_prev) {
    if (m_curves.empty() || !has_leader_seeker()) {
        spawn_leader_seeker();
    } else {
        spawn_pursuit_particle();
    }
}
m_ctrl_a_prev = ctrl_a;
```

`has_leader_seeker()` scans `m_curves` for a particle whose equation name is
`"LeaderSeeker"`.

#### `spawn_leader_seeker()`

```cpp
void SurfaceSimScene::spawn_leader_seeker() {
    // Place at a mid-domain position (not at the max — let it navigate there)
    const float u_mid = 0.5f*(m_surface->u_min() + m_surface->u_max());
    const float v_mid = 0.5f*(m_surface->v_min() + m_surface->v_max());

    // Enable history so pursuit particles can query it
    AnimatedCurve c = AnimatedCurve::with_equation(
        u_mid, v_mid,
        AnimatedCurve::Role::Leader, m_leader_count % AnimatedCurve::MAX_SLOTS,
        m_surface.get(),
        std::make_unique<LeaderSeekerEquation>(&m_extremum_table, m_ls_params),
        &m_milstein);

    const std::size_t cap = static_cast<std::size_t>(
        std::ceil(m_pursuit_tau * 120.f * 1.5f)) + 256;
    c.enable_history(cap, 1.f / 120.f);

    m_curves.push_back(std::move(c));
    ++m_leader_count;
}
```

#### `spawn_pursuit_particle()`

```cpp
void SurfaceSimScene::spawn_pursuit_particle() {
    // Find the leader seeker
    AnimatedCurve* leader = find_leader_seeker();
    if (!leader || !leader->history()) return;

    // Random spawn location
    thread_local std::mt19937 rng{std::random_device{}()};
    std::uniform_real_distribution<float>
        ru(m_surface->u_min(), m_surface->u_max()),
        rv(m_surface->v_min(), m_surface->v_max());
    const float sx = ru(rng), sy = rv(rng);

    std::unique_ptr<ndde::sim::IEquation> eq;
    switch (m_pursuit_mode) {
        case PursuitMode::Direct:
            eq = std::make_unique<DirectPursuitEquation>(
                leader, m_surface.get(), m_dp_params);
            break;
        case PursuitMode::Delayed:
            eq = std::make_unique<DelayPursuitEquation>(
                leader->history(), m_surface.get(), m_dp_params);
            break;
        case PursuitMode::Momentum:
            eq = std::make_unique<MomentumBearingEquation>(
                leader->history(), m_surface.get(),
                m_pursuit_window, m_dp_params.pursuit_speed);
            break;
    }

    m_curves.push_back(AnimatedCurve::with_equation(
        sx, sy,
        AnimatedCurve::Role::Chaser, m_chaser_count % AnimatedCurve::MAX_SLOTS,
        m_surface.get(), std::move(eq), &m_milstein));
    ++m_chaser_count;
}
```

### Panel UI additions

New section "Leader seeker [Ctrl+A]":

```
[ ExtremumSurface radio button ]
Target gradient magnitude:  [slider 0.0 – 2.0]   default 0.0
Flatness epsilon:           [slider 0.01 – 0.5]   default 0.10
Leader noise sigma:         [slider 0.0  – 1.0]   default 0.15

[ Spawn leader [Ctrl+A] ]   [ Spawn pursuer [Ctrl+A] ]

Pursuit mode:  (o) Direct   ( ) Delayed   ( ) Momentum
Delay tau (s): [slider]     (visible for Delayed)
Window (s):    [slider]     (visible for Momentum)
Pursuit speed: [slider]

Extremum table:
  max at (u= X.XX, v= X.XX)  z = X.XX
  min at (u= X.XX, v= X.XX)  z = X.XX
  [ Rebuild now ]
```

---

## Ownership and pointer safety

`LeaderSeekerEquation` holds a `const ExtremumTable*` pointing into
`SurfaceSimScene::m_extremum_table`.  `m_extremum_table` is a value member of
the scene — its address never changes regardless of `swap_surface()`.  When the
table is rebuilt (its contents change), the pointer remains valid; the equation
simply reads the new values on the next `velocity()` call.  This is safe.

`DirectPursuitEquation` holds an `AnimatedCurve*` to the leader, not a raw
`ParticleState*`.  It calls `leader->head_uv()` (a new accessor that returns
`{m_walk.x, m_walk.y}`) each frame.  The leader must not be erased while pursuit
particles reference it — the same contract as `DelayPursuitEquation`.

`MomentumBearingEquation` holds a `const HistoryBuffer*` into the leader's
`m_history`.  This is the same pointer-stability argument as `DelayPursuitEquation`:
`unique_ptr<HistoryBuffer>` move does not move the heap object, so the pointer
survives vector reallocation.

---

## Stability notes

**Goal-flip with no global extremum**: if the surface does not have a well-defined
global max (e.g., a flat plane), the leader will reach the `arrival_radius` of
wherever the grid search placed `max_uv` and flip.  The `arrival_radius` is
configurable (`default = 0.3` param-units).  For degenerate surfaces the leader
will oscillate between two nearby points — noisy but not a crash.

**Flatness condition without a true maximum**: the condition
`|∇f| < epsilon` can trigger mid-slope on a very shallow surface.  Setting
`target_grad_magnitude` to the typical mid-slope value (say 0.3) and `epsilon`
to 0.05 gives a tighter band that only fires near true saddle points or at the
summit.  The slider makes this directly controllable.

**Strategy C with a stationary leader**: if the leader has just arrived at an
extremum and its averaged velocity is nearly zero, `MomentumBearingEquation`
returns `{0,0}` rather than a garbage direction from dividing by a near-zero
magnitude.  The pursuer coasts under its own inertia (no velocity update) until
the leader moves again.

---

## Resolved decisions

### Q1 — ExtremumSurface as a standalone ISurface subclass

Yes, standalone.  `ExtremumSurface` is a new concrete class in
`src/math/ExtremumSurface.hpp`, not an extension of `GaussianSurface`.
Reason: single responsibility.  `GaussianSurface` is already a
specific landscape function; folding peak-and-pit parameterisation into it
would conflate two unrelated shapes.  A new type in the surface selector
(`SurfaceType::Extremum = 3`) is the correct expression of the fact that this
is a different surface.

### Q2 — head_uv() accessor

Added to `AnimatedCurve` in `GaussianSurface.hpp`:

```cpp
// head_uv: the particle's current position in parameter space (u,v).
// Used by DirectPursuitEquation to steer toward the leader's live position.
// Note: this is the navigation target, not the world-space position --
// the pursuer aims at the parameter-space coordinate, not the peak itself.
// Arrival is detected by neighbourhood radius, not exact equality.
[[nodiscard]] glm::vec2 head_uv() const noexcept { return { m_walk.x, m_walk.y }; }
```

The comment is explicit that the pursuer is not seeking the exact
extremum — it is steering toward the *leader's current position*, which
is itself navigating toward the extremum.  Arrival uses a neighbourhood
radius (`arrival_radius` param), not exact equality, precisely because
there is no global max/min guarantee on every surface.

### Q3 — Extremum table rebuild rate for deforming surfaces

The k=30 frame rebuild (0.5s at 60fps) is acceptable.  For `GaussianRipple`,
the extrema shift by at most `amplitude * dt_rebuild / domain_size` per cycle,
which is a small fraction of the domain for typical parameter values.  The
leader is navigating toward a *neighbourhood* of the extremum anyway, so
a slowly drifting target just adds a gentle steering correction — it is a
feature, not a bug.

Code comment to add in the rebuild logic:
```cpp
// We rebuild the extremum table periodically rather than every frame.
// The leader steers toward a neighbourhood of the extremum, not the exact
// peak, so a slightly stale table is acceptable.  The rebuild rate k=30
// (every 0.5s at 60fps) keeps the target position smooth.
```

### Q4 — MomentumBearingEquation query cost

Accepted as-is.  32 binary searches per frame per pursuer at particle
counts below 50 is immaterial on a modern GPU workstation.  Profile first,
optimise if needed.  If it ever becomes a hotspot, the fix is a one-line
cache: store the averaged velocity and recompute it only once per frame
rather than once per sub-step.

---

## Ctrl+H — hotkey reference panel

Added as part of this implementation.  Ctrl+H toggles a floating,
non-docked ImGui panel positioned top-right on first open.  It lists
every hotkey grouped by category (spawn, overlays, panels).

**Implementation**: `draw_hotkey_panel()` in `SurfaceSimScene.cpp`,
declared in `SurfaceSimScene.hpp`.  The panel uses `ImGuiWindowFlags_AlwaysAutoResize`
so it grows/shrinks as hotkeys are added.  `ImGuiCond_FirstUseEver` on
position means the user can drag it and it will stay put.

New hotkeys added to the comment block in `SurfaceSimScene.hpp`:
- `Ctrl+B` — Brownian particle (was already implemented, now documented)
- `Ctrl+R` — delay-pursuit chaser (same)
- `Ctrl+H` — hotkey reference panel (new)

---

## Files changed in this session

```
src/app/GaussianSurface.hpp    head_uv() added with comment
src/app/SurfaceSimScene.hpp    hotkey comment block, m_hotkey_panel_open,
                               m_ctrl_h_prev, draw_hotkey_panel() decl
src/app/SurfaceSimScene.cpp    Ctrl+H handling in handle_hotkeys(),
                               draw_hotkey_panel() call in on_frame(),
                               draw_hotkey_panel() implementation
docs/ctrl_a_leader_seeker.md   this file, open questions resolved
```

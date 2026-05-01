# Ctrl+A — LeaderSeeker: extremum-hunting leader with pursuit particles

## What this feature does

Pressing Ctrl+A spawns a **leader particle** governed by a choice of two
equations on the `ExtremumSurface`:

- **`LeaderSeekerEquation`** — a deterministic state machine that climbs toward
  the global maximum, detects arrival by gradient flatness, then switches goal to
  the global minimum and repeats.  Small Brownian noise prevents it from getting
  stuck on ridgelines.

- **`BiasedBrownianLeader`** — a stochastic leader with no hard goal-switching.
  It diffuses freely (pure Brownian motion) but with a tunable drift bias that
  points toward whichever extremum is currently active as its goal.  The goal
  still flips on gradient flatness, but the path is genuinely random rather than
  deterministic-with-noise.  This is the physically correct model of a particle
  in a potential well subject to thermal fluctuations.

Both share the same `ExtremumTable` and the same pursuit particle machinery.
The choice between them is a spawn-time UI option.

Pressing Ctrl+A again spawns **pursuit particles** at random locations on the
surface.  Each pursuer runs a different bearing strategy toward the leader, and
the strategy is selected at spawn time so different runs can compare behaviours.

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

Every time a surface is **constructed or deformed**, the extremum table is rebuilt:

```cpp
struct ExtremumTable {
    glm::vec2 max_uv;    // parameter-space location of the global max
    float     max_z;     // f(max_uv)
    glm::vec2 min_uv;    // parameter-space location of the global min
    float     min_z;     // f(min_uv)
    bool      valid;
};
```

**Building the table** — a grid search over the parameter domain at resolution
`(N x N)` where `N = 64` finds the candidate extrema, then a short gradient
ascent/descent refines each to sub-pixel accuracy.  The whole operation is
O(N²) = O(4096) evaluations — cheap enough to run synchronously on surface swap.

For `IDeformableSurface` the table is rebuilt every `k = 30` frames (0.5 s at
60fps) since the extrema drift slowly with the deformation.

```cpp
// We rebuild the extremum table periodically rather than every frame.
// The leader steers toward a neighbourhood of the extremum, not the exact
// peak, so a slightly stale table is acceptable.  The rebuild rate k=30
// (every 0.5s at 60fps) keeps the target position smooth.
```

---

## The deterministic leader: `LeaderSeekerEquation`

### State machine

```cpp
enum class Goal { SeekMax, SeekMin };
```

The equation carries one mutable field, `m_goal`, which starts as `SeekMax`.

**Transition condition** — the goal flips when the gradient magnitude at the
current position falls below a threshold:

```
|∇f(u,v)| < epsilon
```

where `epsilon` defaults to `0.1` and is exposed as a UI slider.  The condition
`|f'(x,y) - 1| < 0.1` is a special case: it measures how close the gradient
magnitude is to 1 rather than 0.  Both are supported as variants of the same
slider — a `target_grad_magnitude` parameter (default `0.0` = flat) with
threshold half-width `epsilon`.

```cpp
struct Params {
    float target_grad_magnitude = 0.f;  // 0 = flat summit/pit
    float epsilon               = 0.1f; // half-width of the flatness band
    float pursuit_speed         = 0.8f; // approach speed (param-units/s)
    float noise_sigma           = 0.15f;// Brownian perturbation (keeps it off ridges)
};
```

### `velocity()` logic

```
1. query ExtremumTable -> goal_uv  (max_uv if SeekMax, min_uv if SeekMin)
2. delta = goal_uv - state.uv
3. if dist < arrival_radius: flip goal
4. bearing = delta / dist * pursuit_speed
5. if near_target_gradient(grad_mag): flip goal
6. return bearing
```

`noise_coefficient()` returns `{noise_sigma, noise_sigma}` so the
`MilsteinIntegrator` adds Brownian kicks preventing ridge-lock.

---

## The stochastic leader: `BiasedBrownianLeader`

### Physical intuition

A `LeaderSeekerEquation` particle is deterministic with noise sprinkled on top.
A `BiasedBrownianLeader` is the opposite: it is fundamentally stochastic, with a
deterministic drift bias pointing it toward its current goal.

The Itô SDE governing its parameter-space position is:

```
dX_t = mu(X_t) dt + sigma dW_t
```

where:
- `dW_t ~ N(0, dt·I)` is the 2D Wiener increment — genuine thermal noise
- `sigma` is the isotropic diffusion coefficient — controls how "hot" the particle is
- `mu(X_t)` is the drift — points toward the current goal extremum

This is the Smoluchowski equation for a particle in a potential well subject to
thermal fluctuations.  In the limit `sigma → 0` the path converges to the
deterministic gradient-descent solution.  In the limit `drift_strength → 0` with
`sigma > 0` you recover pure Brownian motion with no preferred direction.

### The drift vector

The drift `mu` is the unit vector from the current position toward the goal
extremum, scaled by `drift_strength`:

```cpp
mu(X) = drift_strength * (goal_uv - X.uv) / |goal_uv - X.uv|
```

This is *not* the surface gradient — it is a straight line in parameter space
toward the target.  For the `ExtremumSurface` where the extrema are well-separated,
this gives clean directed diffusion.

**Comparison with `BrownianMotion::drift_strength`:**
The existing `BrownianMotion` equation has a `drift_strength` parameter that
biases the walk *along the surface gradient* — it goes uphill or downhill.
`BiasedBrownianLeader` is different: it biases the walk *toward a specific
parameter-space target* (the max or min UV coordinates), regardless of which
direction is uphill at the current position.  These are two different drift
geometries:

```
BrownianMotion:       mu = drift * grad(f) / |grad(f)|
                      (follows the slope; knows nothing about the global extremum)

BiasedBrownianLeader: mu = drift * (goal_uv - uv) / |goal_uv - uv|
                      (points at the target directly; ignores local slope)
```

You could combine them: a `BiasedBrownianLeader` with both a goal-directed drift
and a gradient-following drift.  See the `Params` struct below.

### Goal-switching

The goal flip condition is identical to `LeaderSeekerEquation`:

```cpp
if (std::abs(grad_mag - m_p.target_grad_magnitude) < m_p.epsilon)
    flip_goal();
```

The difference is what happens immediately after flipping.  In
`LeaderSeekerEquation`, the bearing snaps to the new target instantly.  In
`BiasedBrownianLeader`, the drift vector rotates smoothly to point at the new
target on the next `velocity()` call — there is no discontinuity, because the
drift is just one component of a noisy SDE.

### Steady-state distribution

The Fokker-Planck equation for this SDE has a stationary solution:

```
p_∞(x) ∝ exp( 2 * drift_strength * phi(x) / sigma^2 )
```

where `phi(x) = -|goal_uv - x|` is the negative distance to the goal (acting
as a potential).  This means:
- High `drift_strength / sigma^2` → particle concentrates tightly near the goal
- Low ratio → particle diffuses broadly, goal has weak pull
- `drift_strength = 0` → uniform distribution (pure Brownian motion)

The ratio `drift_strength / sigma^2` is the **Péclet number** for this flow.
You can tune it with the two sliders independently.

### Implementation

```cpp
// sim/BiasedBrownianLeader.hpp

class BiasedBrownianLeader final : public IEquation {
public:
    struct Params {
        float sigma              = 0.3f;  // diffusion coefficient
        float drift_strength     = 0.6f;  // goal-directed drift magnitude (param-units/s)
        float gradient_drift     = 0.0f;  // optional additional gradient-following drift
                                          // (positive = uphill, negative = downhill)
                                          // set to 0 for pure goal-directed diffusion
        float target_grad_magnitude = 0.f;
        float epsilon               = 0.1f;
        float arrival_radius        = 0.4f;
    };

    // Constructor takes a non-owning pointer to the scene's ExtremumTable.
    // The table address is stable (value member of SurfaceSimScene) so this
    // pointer survives swap_surface() and vector reallocation.
    explicit BiasedBrownianLeader(const ExtremumTable* table, Params p = {});

    glm::vec2 velocity(ParticleState& state,
                       const ISurface& surface,
                       float t) const override;

    glm::vec2 noise_coefficient(const ParticleState&,
                                const ISurface&,
                                float) const override
    {
        return { m_p.sigma, m_p.sigma };
    }

    float phase_rate() const override { return 0.f; }
    std::string name() const override { return "BiasedBrownianLeader"; }

    Params& params() noexcept { return m_p; }

private:
    const ExtremumTable* m_table;
    Params               m_p;
    mutable Goal         m_goal = Goal::SeekMax;  // mutable: toggled in velocity()
};
```

### `velocity()` implementation

```cpp
glm::vec2 BiasedBrownianLeader::velocity(
    ParticleState& state, const ISurface& surface, float t) const
{
    if (!m_table || !m_table->valid) return {0.f, 0.f};

    const glm::vec2 goal_uv = (m_goal == Goal::SeekMax)
        ? m_table->max_uv : m_table->min_uv;

    glm::vec2 delta = goal_uv - state.uv;

    // Shortest-path wrap for periodic surfaces (torus)
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

    // Goal flip: arrival neighbourhood
    if (dist < m_p.arrival_radius) {
        m_goal = (m_goal == Goal::SeekMax) ? Goal::SeekMin : Goal::SeekMax;
        return {0.f, 0.f};  // pause for one step at the goal
    }

    // Goal flip: gradient flatness
    const glm::vec3 du_vec = surface.du(state.uv.x, state.uv.y);
    const glm::vec3 dv_vec = surface.dv(state.uv.x, state.uv.y);
    const float grad_mag = std::sqrt(du_vec.z*du_vec.z + dv_vec.z*dv_vec.z);
    if (std::abs(grad_mag - m_p.target_grad_magnitude) < m_p.epsilon) {
        m_goal = (m_goal == Goal::SeekMax) ? Goal::SeekMin : Goal::SeekMax;
    }

    // Goal-directed drift: unit vector toward goal, scaled by drift_strength
    glm::vec2 mu = (delta / dist) * m_p.drift_strength;

    // Optional gradient drift (same as BrownianMotion::drift_strength)
    if (std::abs(m_p.gradient_drift) > 1e-7f) {
        const float fu = du_vec.z;
        const float fv = dv_vec.z;
        const float gn = std::sqrt(fu*fu + fv*fv) + 1e-7f;
        mu += glm::vec2{ m_p.gradient_drift * fu / gn,
                         m_p.gradient_drift * fv / gn };
    }

    return mu;
}
```

### Relationship to existing `BrownianMotion`

The existing `BrownianMotion` equation (in `src/sim/BrownianMotion.hpp`) is a
general-purpose SDE with an optional gradient-following drift.  It does not know
about the `ExtremumTable` or goal-switching.

`BiasedBrownianLeader` is a specialisation with goal-directed drift and a built-in
state machine.  Its `velocity()` is the drift `mu`; the noise term `sigma * dW`
is handled by `MilsteinIntegrator` via `noise_coefficient()`.

The two are composable: set `BiasedBrownianLeader::Params::gradient_drift` to a
nonzero value and the particle simultaneously drifts toward the goal *and* uphill
(or downhill).  This creates interesting competition between local and global
information: the gradient drift knows the local slope; the goal drift knows where
the global target is.

### Integrator choice

`BiasedBrownianLeader` must be used with `MilsteinIntegrator` (not
`EulerIntegrator`).  The sigma is constant so the Milstein correction is zero
(identical to Euler-Maruyama for the stochastic part), but the distinction matters
once you add curvature-adaptive noise (`sigma ~ 1/sqrt(K(u,v))`), where the
correction term is significant.  Using Milstein from the start means no integrator
swap is needed when you upgrade the noise model.

---

## The pursuit particles: three bearing strategies

Each strategy is a distinct `IEquation` implementation.  The strategy is chosen
at spawn time by a dropdown in the Simulation panel ("Pursuit mode"), so
different runs can compare behaviours.

### Strategy A — Direct bearing

```cpp
velocity(state, surface, t):
    target = leader.head_uv()
    delta  = shortest_path(target, state.uv, surface)
    return (delta / |delta|) * speed
```

Baseline: the pursuer always knows where the leader is right now.

### Strategy B — Delayed bearing

```cpp
velocity(state, surface, t):
    target = leader.history().query(t - tau)
    delta  = shortest_path(target, state.uv, surface)
    return (delta / |delta|) * speed
```

Reuses `HistoryBuffer`.  As `tau` grows relative to the leader's transit time
between extrema, the pursuer heads toward the previous extremum while the leader
has already left — creating characteristic spirals in multi-agent pursuit.

### Strategy C — Averaged-history bearing (momentum extrapolation)

```cpp
velocity(state, surface, t):
    v_avg = average of k velocity samples over window_sec seconds
    return (v_avg / |v_avg|) * speed
```

The pursuer infers the leader's *goal* from its *trajectory*.  If the leader is
heading toward the maximum, this strategy points the pursuer there too, without
needing to know the maximum's location.

**Note**: when the leader is a `BiasedBrownianLeader`, its path is noisy.
Strategy C's moving average smooths that noise before extrapolating — `window_sec`
acts as the bandwidth of a low-pass filter on the leader's velocity.  Longer
windows give cleaner direction estimates but slower response to goal flips.

---

## Implementation plan

### New files

```
src/math/ExtremumSurface.hpp        — bimodal Gaussian ISurface + analytic grad
src/math/ExtremumTable.hpp          — ExtremumTable struct + grid-search builder
src/sim/LeaderSeekerEquation.hpp    — deterministic state-machine IEquation
src/sim/BiasedBrownianLeader.hpp    — stochastic SDE leader with goal drift
src/sim/DirectPursuitEquation.hpp   — Strategy A
src/sim/MomentumBearingEquation.hpp — Strategy C
                                      (Strategy B reuses DelayPursuitEquation)
```

### Changes to existing files

#### `SurfaceSimScene.hpp` — new members

```cpp
SurfaceType                    m_surface_type;       // gains ExtremumSurface = 3
ExtremumTable                  m_extremum_table;     // owned by scene, stable address
LeaderSeekerEquation::Params   m_ls_params;
BiasedBrownianLeader::Params   m_bbl_params;
enum class LeaderMode { Deterministic, StochasticBiased };
LeaderMode                     m_leader_mode = LeaderMode::Deterministic;
PursuitMode                    m_pursuit_mode;
float                          m_pursuit_tau;
float                          m_pursuit_window;
bool                           m_ctrl_a_prev;
```

#### `spawn_leader_seeker()`

```cpp
void SurfaceSimScene::spawn_leader_seeker() {
    const float u_mid = 0.5f*(m_surface->u_min() + m_surface->u_max());
    const float v_mid = 0.5f*(m_surface->v_min() + m_surface->v_max());

    std::unique_ptr<IEquation> eq;
    if (m_leader_mode == LeaderMode::Deterministic)
        eq = std::make_unique<LeaderSeekerEquation>(&m_extremum_table, m_ls_params);
    else
        eq = std::make_unique<BiasedBrownianLeader>(&m_extremum_table, m_bbl_params);

    AnimatedCurve c = AnimatedCurve::with_equation(
        u_mid, v_mid, Role::Leader,
        m_leader_count % AnimatedCurve::MAX_SLOTS,
        m_surface.get(), std::move(eq), &m_milstein);

    const std::size_t cap =
        static_cast<std::size_t>(std::ceil(m_pursuit_tau * 120.f * 1.5f)) + 256;
    c.enable_history(cap, 1.f / 120.f);
    m_curves.push_back(std::move(c));
    ++m_leader_count;
}
```

### Panel UI additions

New section "Leader seeker [Ctrl+A]":

```
Leader mode:   (o) Deterministic   ( ) Stochastic (biased Brownian)

-- Deterministic params (visible when Deterministic selected) --
Target gradient magnitude:  [slider 0.0 – 2.0]  default 0.0
Flatness epsilon:           [slider 0.01 – 0.5]  default 0.10
Leader noise sigma:         [slider 0.0  – 1.0]  default 0.15

-- Stochastic params (visible when Stochastic selected) --
Sigma (diffusion):          [slider 0.01 – 2.0]  default 0.30
Goal drift strength:        [slider 0.0  – 2.0]  default 0.60
Gradient drift:             [slider -1.0 – 1.0]  default 0.00
Flatness epsilon:           [slider 0.01 – 0.5]  default 0.10
Arrival radius:             [slider 0.1  – 2.0]  default 0.40
  Péclet number:  [readout: drift_strength / sigma^2]

[ Spawn leader [Ctrl+A] ]   [ Spawn pursuer [Ctrl+A] ]

Pursuit mode:  (o) Direct   ( ) Delayed   ( ) Momentum
...

Extremum table:
  max at (u= X.XX, v= X.XX)  z = X.XX
  min at (u= X.XX, v= X.XX)  z = X.XX
  [ Rebuild now ]
```

The Péclet number readout `Pe = drift_strength / sigma^2` is the single most
useful diagnostic: it tells you at a glance whether the particle is in the
diffusion-dominated regime (Pe << 1, random walk with weak bias) or the
drift-dominated regime (Pe >> 1, nearly deterministic approach to the goal).

---

## Ownership and pointer safety

`BiasedBrownianLeader` holds a `const ExtremumTable*` pointing into
`SurfaceSimScene::m_extremum_table`.  `m_extremum_table` is a value member of
the scene — its address is invariant regardless of `swap_surface()`.  When the
table contents are updated, the pointer remains valid.

`BiasedBrownianLeader` also holds `mutable Goal m_goal` for the state machine.
This is the same pattern as `GradientWalker::state.angle` — mutable state that
the integrator owns conceptually but the equation stores for continuity across
steps.  It is safe because `AnimatedCurve` owns one `BiasedBrownianLeader`
instance per particle and they are never shared.

---

## Stability notes

**Pe >> 1 (drift dominated)**: the path looks nearly deterministic, converging
smoothly to the goal.  Noise produces small meanders.  Goal-flip is sharp because
the particle reliably reaches the arrival neighbourhood.

**Pe ~ 1**: the path is genuinely stochastic.  The particle may circle the goal
before arriving, or occasionally drift toward the wrong extremum before correcting.
This is the most visually interesting regime.

**Pe << 1 (diffusion dominated)**: the particle performs a near-random walk with
a very weak bias.  It will eventually reach the goal by random chance but may
explore large portions of the domain first.  Goal-flip is unreliable because the
arrival condition may never be triggered.  The `arrival_radius` slider helps:
increasing it makes goal-flip more likely even when the particle never quite
reaches the exact extremum.

**Goal-flip on a deforming surface (`GaussianRipple`)**: the extremum table is
rebuilt every k=30 frames.  Between rebuilds, the goal UV drifts.  The leader
steers toward a moving target, which produces spiral or orbiting paths around the
true extremum.  This is a feature — it makes the biased Brownian leader's path
qualitatively richer on deforming surfaces than on static ones.

---

## Resolved decisions (from previous session)

### Q1 — ExtremumSurface: standalone subclass
Yes. `src/math/ExtremumSurface.hpp`, `SurfaceType::Extremum = 3`.

### Q2 — `head_uv()` accessor
Added to `AnimatedCurve`.  Returns `{m_walk.x, m_walk.y}`.  Comment clarifies
the pursuer targets the leader's position, not the extremum directly.

### Q3 — Extremum table rebuild rate
k=30 frames (0.5s at 60fps) is acceptable.  The leader navigates to a
neighbourhood, not the exact point, so a stale table is fine.

### Q4 — `MomentumBearingEquation` query cost
Accepted as-is.  Profile before optimising.

---

## Files to create / change

```
src/math/ExtremumSurface.hpp          new
src/math/ExtremumTable.hpp            new
src/sim/LeaderSeekerEquation.hpp      new
src/sim/BiasedBrownianLeader.hpp      new   ← this session
src/sim/DirectPursuitEquation.hpp     new
src/sim/MomentumBearingEquation.hpp   new
src/app/GaussianSurface.hpp           head_uv() added (done)
src/app/SurfaceSimScene.hpp           LeaderMode enum, m_bbl_params, m_leader_mode
src/app/SurfaceSimScene.cpp           spawn_leader_seeker() branches on mode,
                                      panel UI: leader mode radio + stochastic params
docs/ctrl_a_leader_seeker.md          this file
```

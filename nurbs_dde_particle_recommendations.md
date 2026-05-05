# `nurbs_dde` — Particle, Equation, Constraint & Goal Recommendations

**Context:** This document recommends new additions to the simulation layer, motivated by two criteria: (1) mathematical depth — each addition should teach you something concrete about Analysis, Differential Geometry, or Stochastic Analysis; (2) direct relevance to the stated end-goal of multi-point Brownian motion on a torus in a delay-pursuit chase.

The existing system is already rich. What it currently has:

| Category | Existing |
|----------|---------|
| Equations (ODE) | `GradientWalker`, `LevelCurveWalker`, `LeaderSeekerEquation`, `DirectPursuitEquation`, `MomentumBearingEquation` |
| Equations (SDE) | `BrownianMotion`, `BiasedBrownianLeader` |
| Equations (DDE) | `DelayPursuitEquation` |
| Behaviors | `SeekParticleBehavior`, `AvoidParticleBehavior`, `CentroidSeekBehavior`, `GradientDriftBehavior`, `OrbitBehavior`, `FlockingBehavior`, `BrownianBehavior`, `ConstantDriftBehavior` |
| Constraints | `DomainConfinement`, `MinDistConstraint` |
| Goals | `CaptureGoal`, `SurvivalGoal` |

What follows is a **prioritised** list of additions grouped by mathematical theme.

---

## Priority 1 — Directly Needed for the Torus Chase Simulation

These are gaps in the existing system that will block or significantly degrade the quality of the core research goal.

---

### E-1. `GeodericPursuitEquation` (ODE)

**What it is:** Replace the parameter-space straight-line pursuit in `DirectPursuitEquation` with pursuit along the *geodesic* — the shortest path on the surface itself.

**Why the current approach is wrong on a torus:**
`DirectPursuitEquation` computes `delta = target - chaser` in the flat $(u,v)$ parameter plane. On a torus parameterised by $(\theta, \phi) \in [0, 2\pi)^2$, the straight-line shortest path in parameter space corresponds to the *flat* shortest path on the rectangle — which is the correct geodesic *only when* the torus has zero Gaussian curvature everywhere (the flat torus $\mathbb{R}^2 / \mathbb{Z}^2$). The moment you render a torus with a major radius $R$ and minor radius $r$, the Gaussian curvature is $K(\theta) = \cos\theta / (r(R + r\cos\theta))$, and the actual geodesics curve. The pursuer will appear to take the "wrong" route near the inner equator.

**Mathematical content:** The geodesic equation on a surface with metric $g_{ij}$ is:
$$\ddot{u}^k + \Gamma^k_{ij} \dot{u}^i \dot{u}^j = 0$$
where $\Gamma^k_{ij}$ are the Christoffel symbols of the second kind. For a chase scenario you do not need to solve this ODE to completion — you only need the **initial tangent direction** of the geodesic from chaser to leader, which is the gradient of the geodesic distance function. This can be approximated by a few steps of gradient descent on the squared geodesic distance estimated via a fast marching method or — for a first implementation — by a short RK4 integration of the geodesic ODE to get the initial bearing.

**Implementation sketch:**
```cpp
class GeodesicPursuitEquation final : public IEquation {
    // Approximate initial geodesic direction via short forward integration
    // of the geodesic ODE using Christoffel symbols from the ISurface metric.
    // ISurface needs: g11, g12, g22, and optionally Gamma_ijk.
};
```

This requires extending `ISurface` with metric tensor and Christoffel symbol queries — which is the right investment since the torus simulation needs them anyway.

---

### E-2. `MultiDelayPursuitEquation` (DDE)

**What it is:** A chaser that tracks a *weighted average* of the leader's position at multiple past times $t - \tau_1$, $t - \tau_2$, $\ldots$, $t - \tau_k$.

**Why you need it:**
The single-delay version produces a characteristic oscillatory lag — the chaser spirals around the leader's past position. Multiple delays produce richer quasi-periodic and potentially chaotic trajectories, which are the mathematically interesting cases for your torus simulation. The $k=2$ case corresponds to a neutral functional differential equation.

**Mathematical content:** The multi-delay DDE is:
$$\dot{X}_c(t) = \sum_{i=1}^{k} w_i \cdot \frac{X_l(t - \tau_i) - X_c(t)}{|X_l(t - \tau_i) - X_c(t)|} \cdot v_i$$

The weights $w_i$ and speeds $v_i$ determine whether the trajectory converges, oscillates, or is chaotic (cf. Mackey-Glass equation structure).

**Implementation note:** Requires no changes to `IEquation` — just a `HistoryBuffer` query at $k$ different past times. The `HistoryBuffer::query` method already supports arbitrary past times.

---

### E-3. `TorusGeodesicLeader` (ODE on the torus specifically)

**What it is:** A leader particle that navigates the torus along *exact* geodesics, bouncing between parametrically specified target angles.

**Why it matters:** The geodesics on a flat torus are straight lines in the covering space $\mathbb{R}^2$, which wrap helically on the torus. A geodesic with rational slope $p/q$ closes after winding $p$ times in $\phi$ and $q$ times in $\theta$; irrational slope produces a dense orbit (equidistribution theorem). This is a beautiful concrete example of the relationship between rational/irrational numbers and topology.

**Mathematical content:** On the flat torus, the geodesic through $(\theta_0, \phi_0)$ with direction $(\alpha, \beta)$ is:
$$(\theta(t), \phi(t)) = (\theta_0 + \alpha t \mod 2\pi,\ \phi_0 + \beta t \mod 2\pi)$$

The orbit is periodic iff $\alpha/\beta \in \mathbb{Q}$, and dense iff $\alpha/\beta \in \mathbb{R} \setminus \mathbb{Q}$.

```cpp
class TorusGeodesicLeader final : public IEquation {
    float m_alpha;  // u-velocity (angular, rad/s)
    float m_beta;   // v-velocity (angular, rad/s)
    // update() simply returns {m_alpha, m_beta} — constant velocity
    // DomainConfinement handles the wrapping
};
```

This is trivial to implement but pedagogically rich: it directly demonstrates the Weyl equidistribution theorem on the torus.

---

### C-1. `TorusGeodesicConfinement` (Constraint — replaces `DomainConfinement` on the torus)

**What it is:** A constraint that wraps UV coordinates correctly on the torus with *configurable domain*, not just the hardcoded `while (uv < u_min) uv += span` loop.

**Issue with `DomainConfinement`:**
The current implementation uses a `while` loop for wrapping, which is correct but runs in $O(k)$ where $k$ is the number of full wraps — a fast particle with a large $dt$ could wrap multiple times per step. The correct implementation is:
```cpp
state.uv.x = u_min + std::fmod(state.uv.x - u_min, span_u);
if (state.uv.x < u_min) state.uv.x += span_u;
```
This is $O(1)$ and handles arbitrarily large steps. Also, the hardcoded `margin = 0.3f` for non-periodic axes is a magic number — it should be a configurable parameter.

---

## Priority 2 — Mathematically Rich Additions

These add significant pedagogical value for the analysis/geometry curriculum and naturally extend the simulation vocabulary.

---

### E-4. `CurvatureAdaptiveBrownian` (SDE)

**What it is:** A Brownian motion equation where the diffusion coefficient $\sigma$ depends on the Gaussian curvature $K$ at the particle's current position.

**Mathematical content:** Brownian motion on a Riemannian manifold is generated by the Laplace-Beltrami operator $\Delta_g$. In local coordinates $(u,v)$ with metric determinant $\det g = g_{11}g_{22} - g_{12}^2$:
$$\sigma_{\text{intrinsic}}(u,v) = \frac{\sigma_0}{(\det g)^{1/4}}$$

This compensates for the stretching of the metric: regions where the surface is heavily curved (large $\det g$) get *less* diffusion per parameter unit because each parameter-unit corresponds to more arc-length. The result is a particle that diffuses uniformly with respect to arc-length, not parameter distance.

For the torus with $R=2, r=1$: the metric is $g_{11} = r^2 = 1$, $g_{22} = (R + r\cos\theta)^2$, so $\det g = r^2(R + r\cos\theta)^2$ and the correction factor is $1/(R + r\cos\theta)^{1/2}$. The outer equator is more stretched, so particles diffuse more slowly there in world space — exactly correct for honest Brownian motion on the embedded torus.

**Why it matters for the Milstein integrator:** With state-dependent $\sigma$, the Milstein correction term $\frac{1}{2}\sigma \frac{\partial\sigma}{\partial u}(dW^2 - dt)$ is no longer zero — this is when the Milstein integrator genuinely outperforms Euler-Maruyama. The infrastructure is already there; this is the equation that justifies it.

---

### E-5. `LangevinEquation` (SDE with inertia)

**What it is:** Add a *velocity state* to the particle so that it obeys the Langevin equation rather than overdamped Brownian motion:
$$m\ddot{X} = -\gamma \dot{X} + F(X) + \sigma \dot{W}$$

In the overdamped limit ($m \to 0$, $\gamma = 1$): $\dot{X} = F(X)/\gamma + (\sigma/\gamma)\dot{W}$ — the current `BrownianMotion`. The underdamped Langevin equation gives the particle *momentum*: it continues in its current direction after a kick, rather than randomising direction every step.

**Why it matters:**
- The overdamped limit is a first-order SDE; the Langevin equation is a second-order SDE (in position), or equivalently a first-order SDE on the phase space $(X, V)$.
- This requires extending `ParticleState` with a `glm::vec2 velocity` field — the first time the simulation state is more than a position.
- On the torus, the underdamped particle can exhibit *stochastic resonance* if combined with a periodic potential — a phenomenon bridging SDE theory and nonlinear dynamics.

**Implementation note:** `ParticleState` currently only stores `uv`, `phase`, `angle`. Adding `glm::vec2 momentum` to `ParticleState` is a breaking change to the layout. The cleanest approach is an extended state type that is passed through a specialised equation:

```cpp
struct LangevinState {
    glm::vec2 uv{0.f, 0.f};
    glm::vec2 velocity{0.f, 0.f};  // momentum / mass
    float phase = 0.f;
    float angle = 0.f;
};
```

Alternatively, encode `velocity` in the existing `(angle, phase)` pair using polar form — hacky but backward-compatible.

---

### E-6. `NormalCurvatureEquation` (ODE — Geometry)

**What it is:** A particle that steers to maintain a constant *normal curvature* $\kappa_n$ as it moves along the surface.

**Mathematical content:** For a curve $\gamma$ on a surface, the normal curvature in direction $\mathbf{v}$ is:
$$\kappa_n(\mathbf{v}) = \frac{II(\mathbf{v}, \mathbf{v})}{I(\mathbf{v}, \mathbf{v})} = \frac{L\,du^2 + 2M\,du\,dv + N\,dv^2}{E\,du^2 + 2F\,du\,dv + G\,dv^2}$$

where $I$ is the first fundamental form and $II$ is the second. Euler's formula states that $\kappa_n$ varies between the principal curvatures $\kappa_1 \leq \kappa_n \leq \kappa_2$ as the direction rotates, with extrema along the *principal directions*.

A particle steered to maintain $\kappa_n = c$ moves along a *line of curvature* (if $c = \kappa_1$ or $c = \kappa_2$) or a general curve with constant normal curvature. On the torus:
- $\kappa_1 = -1/r$ (along the meridian circles, inner curvature)
- $\kappa_2 = \cos\theta / (r(R + r\cos\theta))$ (along the latitude circles)

Lines of curvature on the torus are exactly the latitude and longitude circles — following them gives a particle that traverses the "ribs" of the donut.

---

### E-7. `PrincipalDirectionWalker` (ODE — Geometry)

**What it is:** A walker that always moves along one of the two *principal directions* at its current location.

**Mathematical content:** The principal directions are the eigenvectors of the shape operator (Weingarten map) $S: T_pM \to T_pM$. In local coordinates, they are found by solving:
$$\det(II - \kappa I) = 0 \implies (LN - M^2) - \kappa(LG - 2MF + NE) + \kappa^2(EG - F^2) = 0$$

where $I = (E, F, G)$ and $II = (L, M, N)$ are the fundamental forms. This requires `ISurface` to expose the second fundamental form coefficients — currently it only exposes `du`, `dv`, `evaluate`, and curvatures. Extending `ISurface` with `L()`, `M()`, `N()` (or equivalently `shape_operator()`) would enable this and `E-6` above.

On the Gaussian surface, principal directions align with the gradient and its perpendicular — the `GradientWalker` is already a proxy for this. On the torus, they are the meridian and latitude directions — which are always orthogonal and axis-aligned in parameter space, so this reduces to constant-velocity walkers along $u$ or $v$.

---

### E-8. `StochasticDelaySystem` (SDE + DDE — the target simulation)

**What it is:** The full delay-pursuit chase equation with *both* Brownian noise on the leader and Brownian noise on the chaser, but with independent noise processes.

**Mathematical formulation:**
$$dX_l = \mu_l(X_l, t)\,dt + \sigma_l\,dW_l$$
$$dX_c = v_s \frac{X_l(t-\tau) - X_c}{|X_l(t-\tau) - X_c|}\,dt + \sigma_c\,dW_c$$

where $W_l$ and $W_c$ are **independent** Wiener processes. Currently `BrownianMotion` and `DelayPursuitEquation` are separate equation types assigned to separate particles — which is already correct. The gap is that there is no scenario that simultaneously:
1. Runs the leader as a `BiasedBrownianLeader` (stochastic leader)
2. Runs the chaser as a `DelayPursuitEquation` with `noise_sigma > 0` (stochastic chaser)
3. Does this on the torus domain

This is not a new equation — it is a **spawn scenario** that exercises existing infrastructure. Adding `SimulationTorusChase` as a new `ISimulation` would demonstrate the complete system and is the natural culmination of the roadmap.

---

## Priority 3 — Constraints

---

### C-2. `MaxSpeedConstraint` (IConstraint)

**What it is:** A post-step constraint that clamps `|Δuv| / dt` to a maximum speed.

**Why it matters:** When a pursuer first spawns close to its target, it receives a very large velocity impulse. On a coarse time step this can cause the pursuer to overshoot the leader, then oscillate. A max-speed clamp is the simplest stabiliser.

```cpp
class MaxSpeedConstraint final : public IConstraint {
    float m_max_speed;
    float m_dt;  // set each frame, or pass as part of state
    void apply(ParticleState& state, const ISurface&) const override;
};
```

Note: the constraint system operates on `uv` after integration, not on velocity directly — so "max speed" must be expressed as `max |Δuv|` per step. This requires the constraint to know `dt`, which `IConstraint::apply` does not currently receive. Options: (a) bake `max_speed * dt` into the constraint at construction, refreshed each tick; (b) extend `IConstraint::apply` to receive `dt`.

---

### C-3. `ExclusionZoneConstraint` (IConstraint)

**What it is:** A constraint that keeps a particle outside a circular region in parameter space (centred at a fixed UV point, with radius $r$).

**Mathematical content:** This implements a *reflecting boundary* around an obstacle. On a Riemannian manifold, reflecting boundaries are well-defined but non-trivial — the reflection is with respect to the geodesic normal to the boundary, not the parameter-space normal. For the simple case of a circular obstacle in parameter space, the parameter-space normal is sufficient.

```cpp
class ExclusionZoneConstraint final : public IConstraint {
    glm::vec2 m_center;
    float m_radius;
    // If |uv - center| < radius: push uv radially outward to the boundary circle
};
```

This adds meaningful complexity to the chase dynamics: the leader can hide "behind" an obstacle, creating scenarios where the optimal pursuit strategy must navigate around the zone.

---

### C-4. `CurvatureBoundConstraint` (IPairConstraint)

**What it is:** A pair constraint that prevents two particles from being further apart than `max_dist` in *world space* (using the surface metric), as opposed to the existing `MinDistConstraint` which uses parameter-space distance.

**Why the distinction matters:** On the torus, parameter-space distance is not proportional to world-space distance. Near the inner equator ($\theta = \pi$), a unit step in $\phi$ moves a much shorter world-space distance than near the outer equator. A constraint on parameter-space distance is geometrically incoherent — it is not rotationally symmetric on the torus.

**Implementation:** World-space distance between two surface points is:
$$d_{\text{world}} = |p(\theta_a, \phi_a) - p(\theta_b, \phi_b)|_{\mathbb{R}^3}$$

The constraint checks this value against `max_dist` and applies a parameter-space correction that is the inverse of the pull-back through the parameterisation — which is approximately a correction by $1/\sqrt{\det g}$ in the constrained direction. For a first implementation, simply compute the world-space positions using `ISurface::evaluate` and use the Euclidean distance in $\mathbb{R}^3$.

---

### C-5. `LeashConstraint` (IPairConstraint)

**What it is:** A constraint that prevents two particles from being further than `max_dist` in parameter space, by pulling the farther particle toward the nearer one if the distance exceeds the leash. The complement of `MinDistConstraint`.

**Scenario motivation:** In a multi-pursuer chase, you want the pack of chasers to stay loosely cohesive — no individual pursuer should stray more than some radius from the pack centroid. `LeashConstraint` between each chaser and the centroid implements this without needing a `CentroidSeekBehavior` velocity term — it is a hard geometric constraint rather than a velocity bias.

---

## Priority 4 — Goals

---

### G-1. `EncirclementGoal`

**What it is:** Succeeds when a set of `Chaser` particles forms a ring around a `Leader` particle — all chasers are within an annulus $[r_{\min}, r_{\max}]$ of the leader and are angularly distributed (no two chasers occupy the same angular sector).

**Why it matters:** This tests a cooperative multi-agent pursuit objective, not just the binary capture of `CaptureGoal`. It requires measuring *angular* distribution of the chasers around the leader in parameter space, which is non-trivial on a periodic domain.

**Mathematical sketch:**
```
For each chaser i: compute angle_i = atan2(delta_v_i, delta_u_i)
Sort angles. Goal succeeds when:
  - All |delta_i| in [r_min, r_max]  (annulus constraint)
  - max(angle_gaps) < 2*pi / n * slack  (uniform distribution constraint)
```

---

### G-2. `EscapeGoal`

**What it is:** Succeeds when the `Leader` stays further than `escape_radius` from all `Chaser` particles for a sustained duration `duration_seconds`.

**Mathematical content:** This is the *dual* of `CaptureGoal` — the leader's objective. Together with `CaptureGoal` for the chasers you have a well-defined two-team game with adversarial objectives. The sustained-duration requirement prevents momentary luck from counting as success.

```cpp
class EscapeGoal final : public IParticleGoal {
    float m_escape_radius;
    float m_required_duration;
    float m_current_streak = 0.f;
    GoalStatus evaluate(const SimulationContext& context) override;
};
```

Note: this requires the goal to be *stateful* between ticks (accumulating `m_current_streak`), which `IParticleGoal::evaluate` supports since the goal object persists for the simulation lifetime.

---

### G-3. `TrajectoryDensityGoal`

**What it is:** Succeeds when a Brownian particle has visited every cell of a uniform grid on the surface at least once — demonstrating *ergodicity* of the random walk.

**Mathematical content:** The ergodic theorem for Brownian motion on a compact Riemannian manifold states that the empirical occupation measure converges to the Riemannian volume measure almost surely. This goal makes that theorem *visible*: the simulation runs until the particle has covered the surface, then reports the time taken.

**Implementation:**
```cpp
class TrajectoryDensityGoal final : public IParticleGoal {
    std::vector<bool> m_visited;  // grid of n_u * n_v cells
    u32 m_n_u, m_n_v;
    u32 m_unvisited_count;
    GoalStatus evaluate(const SimulationContext& context) override {
        // For each particle, mark the cell at its head_uv() as visited
        // Succeed when m_unvisited_count == 0
    }
};
```

For the torus at $120$ Hz, ergodicity kicks in visibly within seconds — it makes a compelling live visualisation. Compare the time-to-coverage for different $\sigma$ values: the relationship is $T \sim (L^2 / \sigma^2) \log(n)$ where $L$ is the linear domain size — directly measuring the diffusion constant from the simulation.

---

### G-4. `LipschitzGoal`

**What it is:** A diagnostic goal (never "fails", never fully "succeeds") that continuously measures the *Lipschitz constant* of the current velocity field by sampling finite differences, then displays it as a running metric in the panel.

**Mathematical content:** The Lipschitz constant of the velocity field $f: M \to TM$ is:
$$L = \sup_{x \neq y} \frac{|f(x) - f(y)|}{d(x,y)}$$

For the delay-pursuit equation, $L$ bounds the sensitivity of the trajectory to perturbations in initial conditions. It is directly relevant to GPU stability analysis: if $L \cdot dt > 1$, the Euler integrator is unstable. Displaying $L$ live makes the stability criterion visible and teaches why $dt$ must be chosen proportionally to $1/L$.

```cpp
class LipschitzGoal final : public IParticleGoal {
    // Sample n_samples pairs of particles,
    // compute |f(x) - f(y)| / |x - y| for each,
    // report the empirical maximum as metadata.
    GoalStatus evaluate(const SimulationContext& context) override {
        // Always returns GoalStatus::Running — it's a diagnostic, not a win condition
    }
};
```

---

## Summary Table

| ID | Type | Name | Priority | Mathematical Theme |
|----|------|------|----------|--------------------|
| E-1 | Equation (ODE) | `GeodesicPursuitEquation` | 🔴 Critical | Differential Geometry, Christoffel symbols |
| E-2 | Equation (DDE) | `MultiDelayPursuitEquation` | 🔴 Critical | Functional DEs, chaos |
| E-3 | Equation (ODE) | `TorusGeodesicLeader` | 🔴 Critical | Weyl equidistribution, irrational orbits |
| C-1 | Constraint | `TorusGeodesicConfinement` | 🔴 Critical | Modular arithmetic, wrapping correctness |
| E-4 | Equation (SDE) | `CurvatureAdaptiveBrownian` | 🟠 Major | Riemannian BM, Laplace-Beltrami operator |
| E-5 | Equation (SDE) | `LangevinEquation` | 🟠 Major | Second-order SDE, inertia, phase space |
| E-6 | Equation (ODE) | `NormalCurvatureEquation` | 🟠 Major | Second fundamental form, lines of curvature |
| E-7 | Equation (ODE) | `PrincipalDirectionWalker` | 🟠 Major | Shape operator, principal curvatures |
| E-8 | Scenario | `SimulationTorusChase` | 🟠 Major | Full stochastic delay-pursuit on torus |
| C-2 | Constraint | `MaxSpeedConstraint` | 🟡 Minor | Numerical stability, step-size analysis |
| C-3 | Constraint | `ExclusionZoneConstraint` | 🟡 Minor | Reflecting boundaries, obstacle avoidance |
| C-4 | Constraint | `CurvatureBoundConstraint` | 🟡 Minor | Metric geometry, world-space vs param-space |
| C-5 | Constraint | `LeashConstraint` | 🟡 Minor | Multi-agent coherence |
| G-1 | Goal | `EncirclementGoal` | 🟡 Minor | Cooperative pursuit, angular distribution |
| G-2 | Goal | `EscapeGoal` | 🟡 Minor | Adversarial objectives, sustained conditions |
| G-3 | Goal | `TrajectoryDensityGoal` | 🟡 Minor | Ergodic theorem, diffusion constant measurement |
| G-4 | Goal | `LipschitzGoal` | 🟡 Minor | Stability analysis, Lipschitz constants |

---

## Suggested `ISurface` Extensions

Several of the above require surface queries that `ISurface` does not currently expose. These are worth adding once rather than hacking around them per-equation:

```cpp
class ISurface {
public:
    // ... existing interface ...

    // First fundamental form coefficients at (u, v)
    virtual float E(float u, float v) const;  // g_11 = du·du
    virtual float F_coeff(float u, float v) const;  // g_12 = du·dv
    virtual float G(float u, float v) const;  // g_22 = dv·dv

    // Second fundamental form coefficients at (u, v)
    // Requires the unit normal: n = (du × dv) / |du × dv|
    virtual float L_coeff(float u, float v) const;  // II_11 = -dn/du · du
    virtual float M_coeff(float u, float v) const;  // II_12 = -dn/du · dv
    virtual float N_coeff(float u, float v) const;  // II_22 = -dn/dv · dv

    // Christoffel symbols Gamma^k_{ij} — needed for geodesic ODE
    // Only 8 independent components (symmetry in lower indices).
    virtual void christoffel(float u, float v,
                             float out_Gamma[2][2][2]) const;
};
```

Default implementations can compute these from finite differences using the existing `du()`, `dv()`, and `evaluate()` methods — so concrete surfaces only need to override them for analytic performance.

---

## Recommended Implementation Order

Following your learning curriculum, this ordering maps each addition to a concept before you need it:

1. **`TorusGeodesicLeader`** — implement the flat torus geodesic first. Trivial code, profound topology.
2. **`TorusGeodesicConfinement`** — fix the wrapping bug while you're thinking about the torus domain.
3. **`CurvatureAdaptiveBrownian`** — first use of the metric tensor. Adds `E`, `F`, `G` to `ISurface`.
4. **`MultiDelayPursuitEquation`** — natural extension of the existing DDE infrastructure. No API changes.
5. **`SimulationTorusChase`** — assemble E-3, E-4, and E-2 into the target simulation.
6. **`TrajectoryDensityGoal`** — attach to the torus simulation to make ergodicity measurable.
7. **`GeodesicPursuitEquation`** — requires Christoffel symbols. Adds `christoffel()` to `ISurface`.
8. **`LangevinEquation`** — requires extending `ParticleState`. Plan this as a deliberate refactor.
9. **`LipschitzGoal`** — diagnostic; add whenever the stability analysis becomes relevant.

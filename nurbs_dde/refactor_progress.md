# nurbs_dde Refactor Progress

## Roadmap

Step 1  [DONE] GaussianSurface implements ISurface
Step 2  [DONE] Thread ISurface* into AnimatedCurve
Step 3  [DONE] m_surface (unique_ptr<ISurface>) owns the surface in SurfaceSimScene
Step 3d [DONE] Torus + surface selector UI
Step 3b [DONE] float t on all ISurface methods (time-varying preparation)
Step 3c [DONE] IDeformableSurface + GaussianRipple
Step 3e [DONE] Thread m_sim_time to all geometry query call sites
Step 4  [DONE] IEquation + GradientWalker
Step 5  [DONE] IIntegrator + EulerIntegrator
Step 5b [DONE] Per-particle equation ownership (unique_ptr via with_equation)
Step 9  [DONE] BrownianMotion equation + MilsteinIntegrator
Step 10 [DONE] HistoryBuffer + DelayPursuitEquation
Step 2b [ ]    Particle state moves to parameter space (uv)
Step 2c [ ]    DomainConfinement constraint replaces hard-coded boundary reflection
Step 5c [ ]    IConstraint + pairwise constraints
Step 6  [ ]    SpawnStrategy
Step 7  [ ]    Extract GeometrySubmitter
Step 8  [ ]    Shrink SurfaceSimScene to orchestrator

---

## Step 10 -- DONE

### Files created

- src/sim/HistoryBuffer.hpp
    class HistoryBuffer
    Params: capacity (ring buffer size), dt_min (rate-limiter)
    push(t, uv): append record; overwrites oldest when full
    query(t_past) -> glm::vec2: binary search + linear interpolation
    oldest_t(), newest_t(), size(), empty(), clear()
    Invariant: records stored in chronological order via ring buffer with m_head
    Predicate: query(t) = lerp(r[lo].uv, r[hi].uv, alpha) where r[lo].t <= t <= r[hi].t

- src/sim/DelayPursuitEquation.hpp
    class DelayPursuitEquation : public IEquation
    Params: tau (delay s), pursuit_speed (param-units/s), noise_sigma
    Constructor: takes const HistoryBuffer* (non-owning) + const ISurface*
    velocity(state, surface, t):
        target = history.query(t - tau)
        delta  = target - state.uv                 [naive difference]
        if periodic: wrap delta to shortest path    [torus-correct]
        return (delta / |delta|) * pursuit_speed
    noise_coefficient(): {noise_sigma, noise_sigma}
    Geometric note: periodic shortest-path wrapping implemented per axis

### Files changed

- src/app/GaussianSurface.hpp
    Added #include "sim/HistoryBuffer.hpp"
    AnimatedCurve public interface:
        enable_history(capacity, dt_min): creates m_history
        push_history(t): pushes (t, {x,y}) into m_history if set
        query_history(t_past) -> glm::vec2: delegates to m_history
        history() const -> const HistoryBuffer*: accessor for scene
    AnimatedCurve private:
        Added std::unique_ptr<HistoryBuffer> m_history (null by default)

- src/app/GaussianSurface.cpp
    Added implementations: enable_history, push_history, query_history
    push_history uses m_walk.x, m_walk.y (parameter-space position, not trail)

- src/app/SurfaceSimScene.hpp
    Added includes: HistoryBuffer.hpp, DelayPursuitEquation.hpp
    Added m_dp_params: DelayPursuitEquation::Params (UI state)
    Added m_dp_count: u32 (spawn counter for angle offsets)
    Added m_ctrl_r_prev: bool (edge detection for Ctrl+R)

- src/app/SurfaceSimScene.cpp
    advance_simulation(): after particle advance loop, calls push_history(m_sim_time)
      on all curves (push_history is a no-op when m_history == nullptr)
    handle_hotkeys(): Ctrl+R spawn logic:
        ensures m_curves[0].history() != null (calls enable_history if needed)
        spawns AnimatedCurve::with_equation(DelayPursuitEquation(...), &m_milstein)
        capacity = ceil(tau * 120 * 1.5) + 256
    draw_simulation_panel(): new "Delay Pursuit [Ctrl+R]" section
        Tau slider (0.1 - 10s), Speed slider, Noise sigma slider
        "Spawn Pursuer" button
        History window readout: "N.Ns window  N records"
        Warm-up indicator: yellow text when window < tau

### Pointer safety after vector reallocation
    DelayPursuitEquation holds: const HistoryBuffer* -> m_curves[0].m_history.get()
    When m_curves.push_back() reallocates, AnimatedCurves are MOVED.
    unique_ptr<HistoryBuffer> move does NOT change the heap address of the
    HistoryBuffer object -- only the unique_ptr's internal raw pointer is moved.
    Therefore the DelayPursuitEquation's stored pointer remains valid. CORRECT.

### Usage protocol
    1. Ctrl+L: ensure leader (curve 0) exists
    2. Let it run for at least tau seconds (warm-up period)
    3. Ctrl+R: spawn delay-pursuit chaser
       - Panel shows "Warming up: N.Ns / tau s" until history window >= tau
       - Once warm, the chaser traces the leader's path from tau seconds ago
    4. Multiple chasers can target the same leader (each with independent DDE)
    5. On torus: shortest-path wrapping ensures correct pursuit across seams

### Mathematical notes
    This is a Delay Differential Equation (DDE).
    The solution depends on the entire history X_l(s), s in [t-tau, t].
    The initial condition is a FUNCTION on [-tau, 0], not a point.
    In simulation: HistoryBuffer provides this function via linear interpolation.
    The cold-start (window < tau): query clamps to oldest record (constant init condition).
    Stability: for pursuit speed v and delay tau, the system is stable when
      v * tau < pi/2 (Lotka-Volterra type stability bound).
      At v*tau = pi/2 the chaser enters a stable limit cycle (orbit around leader).
      Above this threshold: divergence / oscillation amplification.

---

## Remaining steps

### Original roadmap (plumbing)

Step 2b  [ ]  Particle state moves to parameter space — retire WalkState, store ParticleState directly
Step 2c  [ ]  DomainConfinement IConstraint replaces hard-coded boundary reflection
Step 5c  [ ]  IConstraint + pairwise minimum-distance constraints
Step 6   [ ]  SpawnStrategy — extract spawn logic from handle_hotkeys()
Step 7   [ ]  Extract GeometrySubmitter (ParticleRenderer + SurfaceRenderer)
Step 8   [ ]  Shrink SurfaceSimScene to thin orchestrator (~200 lines)

### New items from architecture review

A2  [ ]  HistoryBuffer::to_vector() — linearise ring buffer for export and iteration (10 lines)
A3  [ ]  SurfaceSimScene::export_session() — CSV/JSON export of trail + history + metadata (50 lines)
A4  [ ]  RNG seed capture in MilsteinIntegrator — expose global_seed in config + export metadata
B1  [ ]  Split GaussianSurface.hpp into AnimatedCurve.hpp, FrenetFrame.hpp, math/GaussianSurface.hpp
C4  [ ]  Ctrl+A feature — ExtremumSurface, ExtremumTable, LeaderSeekerEquation, BiasedBrownianLeader,
             DirectPursuitEquation, MomentumBearingEquation (designed in docs/ctrl_a_leader_seeker.md)
D1  [ ]  Rename IEquation::velocity() -> update() (signals mutation, not pure computation)
D2  [ ]  Replace submit/submit2 with named render targets
E1  [ ]  Move Scene.cpp / AnalysisPanel.cpp to legacy/ (dead code, m_scene->on_frame() is commented out)

### Full priority order — see TODO.md at repo root

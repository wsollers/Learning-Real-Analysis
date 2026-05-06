# Repository To Do

This is the living refactor and feature checklist for the repository, with the current focus on `nurbs_dde`.

## Highest Priority

- [ ] Verify current dirty work before committing.
  - Review all modified `nurbs_dde` files from the recent refactor.
  - Decide whether to keep or delete the root review notes:
    - `nurbs_dde_code_review.md`
    - `nurbs_dde_particle_recommendations.md`

- [ ] Finish particle/history lifetime cleanup.
  - Continue replacing raw PMR resource rebinding with `MemoryService` scope APIs.
  - Audit service/view/cache containers that still reconstruct vectors manually.
  - Keep generation-aware assertions for simulation and history data.
  - Add death tests when stale particle/history handles are used after scope reset.

- [ ] Finish allocation policy enforcement.
  - Tighten CI/static checks for forbidden `new`, `delete`, `malloc`, `free`.
  - Decide where STL containers are acceptable and where arena-backed containers are mandatory.
  - Move remaining broad `std::unique_ptr` factories under engine-owned `MemoryService` where practical.

## Architecture Cleanup

- [ ] Keep the engine as lifecycle owner and service root.
  - `Engine` owns services.
  - Simulations receive a narrowed host/service facade.
  - Simulations register panels, hotkeys, views, and commands through services.
  - Simulations roll back registrations on destruction.

- [ ] Continue shrinking app simulations toward the current architecture.
  - `ISimulation` owns simulation-specific math state and recalculation.
  - `ISurface` owns equations, derivatives, metadata, and deformability.
  - `SimulationContext` stores math state/history using engine memory scopes.
  - Renderer receives geometry packets only; no simulation logic belongs in renderer.

- [ ] Replace any remaining concrete-sim panel calls with a simulation command API.
  - Use named commands: `reset`, `pause`, `step`, `spawn recipe`, `clear`, `set speed`, etc.
  - Keep panel lambdas thin and route through command structs where possible.

- [ ] Normalize panel architecture.
  - Global panels:
    - coordinates/debug
    - engine log
    - simulation metadata
    - selection/hover debug
  - Per-simulation panels:
    - controls
    - particles
    - swarms/recipes
    - goals
    - equation/system parameters

## Rendering And View System

- [ ] Mature typed alternate views.
  - Contour
  - Level curves
  - Vector field
  - Isoclines
  - Flow / streamlines
  - Phase space

- [ ] Restore and improve hover overlays across all views.
  - Surface views should support surface point hover, particle snap, and trail sample snap.
  - 2D curve/phase views should support nearest curve sample hover.
  - Global hover debug panel should show:
    - view id
    - view kind
    - coordinates
    - particle id
    - trail index
    - curvature/torsion when available
    - ODE/DDE state when available

- [ ] Improve renderer robustness.
  - Add explicit PNG capture format handling instead of assuming BGRA forever.
  - RAII-wrap capture staging buffers.
  - Keep Vulkan image barriers precise and validation-clean.
  - Keep unknown topology failures loud.

## Camera And Interaction

- [ ] Continue centralizing camera handling in the engine-owned camera service.
  - Per-view camera state.
  - Perspective surface camera profile.
  - Orthographic 2D profile.
  - Reset/home/top/front/side presets.
  - Frame selection.
  - Later: follow selection.

- [ ] Finish interaction/selection service behavior.
  - Store hover target and selected target per view.
  - Target types:
    - surface point
    - 2D view point
    - particle
    - trail sample
    - none
  - Double-click semantics:
    - surface view: perturb deformable surface
    - phase-space view: set/reset ODE initial condition
    - particle/trail view: select particle or trail sample

- [ ] Implement real surface picking.
  - Replace rough screen-to-domain mapping with ray to surface intersection.
  - Use surface hit payload for perturbation, selection, and hover diagnostics.

## Surface And Equation Features

- [ ] Expand surface metadata.
  - name
  - formula
  - domain
  - analytic derivative support
  - deformable/static
  - time-varying/static
  - parameter list

- [ ] Finish deformable surface path.
  - `IDeformableSurface`
  - double-click perturbation
  - bounded random perturbation magnitude
  - ripple propagation over time
  - dirty mesh/cache invalidation

- [ ] Clarify symbolic equation support.
  - Formula strings are currently metadata unless backed by typed systems.
  - Decide whether/when to add a real parser for strings like `x' = f(x(t), x(t - tau))`.
  - If added, route parsed math through project math/ops redirectors.

- [ ] Continue ODE/DDE foundation.
  - `IDifferentialSystem`
  - `IDelayDifferentialSystem`
  - `InitialValueProblem`
  - `DelayInitialValueProblem`
  - Euler, RK4, and DDE solvers
  - bounded history retention
  - equation metadata panels

## Analysis Panel And Overlay Upgrades

### Universal Darboux Frame

- [ ] Add Darboux frame overlay as the surface-aware replacement/complement to Frenet.
  - `T`: particle unit heading.
  - `n`: unit surface normal at the particle foot point.
  - `g = T x n`: geodesic normal in the tangent plane.
  - Render `g` as a third arrow in a distinct yellow-green color.

- [ ] Add Darboux readouts to hover/analysis panels.
  - `kappa_n`: normal curvature.
  - `kappa_g`: geodesic curvature.
  - `kappa_g = 0` indicates geodesic behavior.
  - Useful for validating torus geodesic leaders and other surface-aware walkers.

- [ ] Add overlay flags in `RenderViewOverlayFlags`.
  - `show_darboux_frame`
  - `show_diffusion_ellipse`
  - `show_ghost_marker`
  - `show_metric_ellipse`

### ODE Walker Overlays

- [ ] For deterministic walkers, show velocity vector at hover.
  - Render `v(u,v)` at hover point.
  - Direction should be `du/dt`, `dv/dt` remapped to world space through the surface Jacobian.
  - Magnitude should be shown by arrow length.
  - Add optional Lipschitz cone / wireframe disk later.

### SDE / Brownian Overlays

- [ ] Add diffusion ellipse overlay.
  - This should replace the osculating circle for Brownian/SDE particles.
  - Render a 32-segment wireframe ellipse around particle head.
  - Use trail color at about 40 percent alpha.
  - Parameter-space semi-axes:
    - `sigma_u * sqrt(dt)`
    - `sigma_v * sqrt(dt)`
  - Push forward to world space via the surface Jacobian:
    - `|dp/du| * sigma_u * sqrt(dt)`
    - `|dp/dv| * sigma_v * sqrt(dt)`

- [ ] Support curvature-adaptive Brownian diffusion ellipse.
  - Semi-axis scale:
    - `sigma_0 / (det g)^(1/4) * sqrt(dt)`
  - Use the ellipse to visualize metric-distorted noise.

### DDE / Pursuit Overlays

- [ ] Add ghost marker overlay.
  - Query leader history at `t - tau`.
  - Draw dashed circle at the delayed leader position.
  - Draw pursuit arrow from chaser to ghost.
  - Draw comparison arrow from chaser to current leader.
  - Display delay error angle between the two directions.

- [ ] Add delay cone overlay.
  - Center at chaser position.
  - Radius `pursuit_speed * tau`.
  - Show whether current leader position lies inside the one-delay reachable set.
  - Add panel readout for `v_s * tau / d`.

### Metric Tensor Overlay

- [ ] Add metric tensor ellipse at hover point for all surface equations.
  - Thin neutral grey wireframe.
  - Shows local arc-length cost per parameter step.
  - For diagonal metrics, axes proportional to:
    - `1 / sqrt(E)` along `u`
    - `1 / sqrt(G)` along `v`
  - Later generalize to non-diagonal metrics using eigenvectors/eigenvalues of `g_ij`.

### Implementation Notes

- [ ] Add overlay rendering to:
  - `ProjectedParticleOverlay` for 3D views.
  - `Curve2DOverlay` for 2D curve/phase panels.
  - Shared helpers where the math is view-independent.

- [ ] Keep overlays hover-driven.
  - Show at hover snap.
  - Hide when mouse leaves the relevant trail/particle/surface target.
  - Toggle through per-overlay flags.

## Simulation Ideas

- [ ] Differential Systems sim improvements.
  - Better ODE panel:
    - system selector
    - solver selector
    - step size
    - initial values
    - current state vector
    - parameters
    - reset/step/run
    - history length
    - diagnostics such as energy/error when available

- [ ] DDE simulation improvements.
  - Bounded delayed feedback is present; improve panels and overlays.
  - Add visual delay diagnostics:
    - delayed state
    - current state
    - delay error
    - boundedness envelope

- [ ] Lyapunov stability 3D work.
  - Lorenz attractor pair trajectories.
  - Initial perturbation controls.
  - Hover distance between base curve and perturbed curve.
  - Draw norm segment between paired trajectory samples.
  - Panel readout for current/hover norm.

## Documentation

- [ ] Keep architecture docs current.
  - `nurbs_dde/docs/CURRENT_ARCHITECTURE_DIAGRAM.md`
  - `nurbs_dde/docs/PROPOSED_ARCHITECTURE_DIAGRAM.md`
  - `nurbs_dde/docs/ALLOCATION_POLICY.md`
  - `nurbs_dde/docs/TIME_AND_SIMULATION.md`
  - `nurbs_dde/docs/COORDINATE_SYSTEMS.md`
  - `nurbs_dde/docs/ADDING_EQUATION_SURFACE_SIMULATION.md`
  - `nurbs_dde/docs/ADDING_DAMPED_OSCILLATOR.md`

- [ ] Add docs for:
  - adding an ODE system
  - adding a DDE system
  - adding a surface
  - adding an overlay
  - adding a simulation
  - adding a panel

## Tests To Add Or Keep Expanding

- [ ] Memory/lifetime tests.
  - Scope reset assertions for sim/history/view/cache objects.
  - Particle/history stale-use tests.
  - Service handle stale-use tests.

- [ ] Overlay geometry tests.
  - Darboux frame vectors are orthonormal or correctly normalized.
  - `g = T x n`.
  - `kappa_g = 0` for known geodesic cases where practical.
  - Diffusion ellipse axes match flat-surface expected circle/ellipse.
  - Metric ellipse axes match simple surfaces.
  - DDE ghost marker uses `t - tau` history query.
  - Delay cone radius equals `speed * tau`.

- [ ] ODE/DDE tests.
  - `y' = y`, `y(0) = 1`, compare to `e^t`.
  - harmonic oscillator energy behavior.
  - Van der Pol qualitative behavior.
  - Lorenz pair perturbation divergence.
  - bounded DDE remains inside expected envelope.
  - bounded DDE history compaction retains usable delay window.

- [ ] Renderer tests where feasible.
  - Alternate view dispatch emits correct packet types.
  - PNG capture handles expected swapchain format or fails explicitly.


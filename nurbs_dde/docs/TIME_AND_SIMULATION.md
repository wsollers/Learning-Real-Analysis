# Time and Simulation in NDDE

## How the Engine Handles Time

The engine does **not** own a clock or a `dt`. It is a rendering loop, not
a simulation loop. Time is the application's responsibility.

The frame loop calls `Scene::on_frame()` as fast as the GPU will present
frames (vsync-limited by default, so 60 fps on most monitors). It does not
pass `dt` to `on_frame()`. If you need time, measure it yourself.

The canonical pattern is:

```cpp
// In Scene.hpp (private):
double m_sim_time     = 0.0;   // simulation clock in seconds
double m_wall_last    = 0.0;   // wall clock at previous frame

// In Scene::on_frame():
const double wall_now = ImGui::GetTime();         // seconds since ImGui init
const double dt       = wall_now - m_wall_last;   // real elapsed seconds
m_wall_last           = wall_now;

if (!m_paused)
    m_sim_time += dt * m_speed;                   // scaled simulation clock
```

`ImGui::GetTime()` returns the wall clock in seconds with double precision.
It is the cheapest available high-resolution timer from within `on_frame()`
because ImGui snapshots it at the start of each frame.

---

## Time for a DDE Simulation

A delay-differential equation of the form

    x'(t) = f(x(t), x(t - τ))

requires access to the state at a **past time** `t - τ`. The engine provides
no history buffer — that is your simulation object's responsibility.

The standard architecture is a **ring buffer of (time, state) pairs**:

```cpp
struct StateRecord { double t; Vec3 pos; };

// In your DDE component:
std::vector<StateRecord> m_history;   // ring buffer, size = ceil(tau / dt) + margin
std::size_t              m_head = 0;  // index of the most recent record
```

Each frame, after advancing the integrator:

1. Write `{ m_sim_time, current_pos }` to `m_history[m_head]`
2. Advance `m_head = (m_head + 1) % m_history.size()`
3. To evaluate `x(t - τ)`, walk backwards from `m_head` until you find
   the record with timestamp closest to `m_sim_time - tau`, then
   linearly interpolate between the two bracketing records

The buffer needs at least `ceil(tau / dt_min) + 2` entries. With `tau = 0.5s`
and `dt_min ≈ 1/60 s`, that is about 32 entries. Allocate 4× headroom.

---

## Simulation Speed and Pausing

Add to `Scene.hpp`:

```cpp
double m_speed  = 1.0;    // 1.0 = real time, 2.0 = double speed
bool   m_paused = false;
```

Expose them in `draw_main_panel()`:

```cpp
ImGui::SliderFloat("Speed", &m_speed, 0.1f, 10.f, "%.2fx");
ImGui::Checkbox("Pause", &m_paused);
```

Advance the clock only when not paused:

```cpp
if (!m_paused) m_sim_time += dt * m_speed;
```

The rendered trajectory is whatever the history buffer contains up to
`m_sim_time`. Pausing freezes the simulation clock; the render loop
continues drawing the last computed state at 60 fps.

---

## Threading Model

The engine is currently single-threaded. The render loop and simulation
advance both run on the main thread inside `on_frame()`.

For a computationally heavy DDE integrator (e.g. RK4 with many particles
on a torus), the recommended upgrade is:

```
Main thread (render)          Worker thread (simulation)
─────────────────────         ──────────────────────────
on_frame() reads front_buf    integrator writes back_buf
at end of frame:              at fixed dt:
  swap front/back              step integrator
  (atomic flag or mutex)       write trajectory points
```

The swap is a single `std::atomic<int>` flip (0 or 1 for double-buffering)
or a `std::mutex`-guarded pointer swap. The render thread never blocks for
more than the cost of a mutex lock. The worker thread advances at whatever
rate the integrator allows, independent of vsync.

This design preserves the engine contract: `on_frame()` still calls
`m_api.acquire() → tessellate() → submit()` with whatever trajectory
data is currently in the front buffer. The simulation thread is completely
outside the engine layer.

---

## The Torus Use Case

For Brownian motion on a torus with coordinates `(θ, φ) ∈ [0, 2π)²`,
each particle needs:

- A `Vec2` state `(theta, phi)` updated each frame
- Wrapping via `std::fmod(theta, 2π)` after each step
- Conversion to `Vec3` for rendering:
  ```
  x = (R + r cos φ) cos θ
  y = (R + r cos φ) sin θ
  z = r sin φ
  ```

The DDE pursuit term `x(t - τ)` requires the history buffer above. The
stochastic term `dW` is a Wiener increment: `N(0,1) * sqrt(dt)` per
component, generated from `std::normal_distribution<double>`.

Time handling is unchanged — `m_sim_time` advances, the history ring
buffer stores `(t, theta, phi)` records, and the renderer reads the
current buffer every frame.

# Adding A Damped Oscillator

This is the intended pattern for adding a bounded two-dimensional ODE system to the engine.

## System

Implement `IDifferentialSystem`:

```cpp
class DampedOscillatorSystem final : public IDifferentialSystem {
public:
    DampedOscillatorSystem(f64 omega, f64 gamma);
    std::size_t dimension() const override { return 2; }
    EquationSystemMetadata metadata() const override;

    void evaluate(f64 t, std::span<const f64> state, std::span<f64> derivative) const override {
        const f64 x = state[0];
        const f64 v = state[1];
        derivative[0] = v;
        derivative[1] = -2.0 * gamma * v - omega * omega * x;
    }
};
```

The state vector is `(x, v)`. The system object owns parameters and formulas; evolving state belongs to `InitialValueProblem`.

## Problem State

Create the IVP through the engine memory service:

```cpp
auto system = memory.simulation()
    .make_unique_as<IDifferentialSystem, DampedOscillatorSystem>(omega, gamma);

double initial[] = {x0, v0};
auto problem = memory.simulation()
    .make_unique<InitialValueProblem>(memory, *system, std::span<const double>{initial});
```

`InitialValueProblem` stores:

- current state in simulation memory
- solver scratch in simulation memory
- trajectory history in history memory

## Rendering

For a 2D phase-space view:

- x-axis: position `x`
- y-axis: velocity `v`
- vector field: sample `evaluate(t, {x, v})`
- trajectory: draw `problem.history_state(i)`
- current point: draw a small cross at `problem.state()`

## Diagnostics

For the undamped oscillator, energy is:

```cpp
E = 0.5 * (v*v + omega*omega*x*x)
```

For the damped oscillator, `E` should decay monotonically for positive `gamma`.

## Tests

Useful tests:

- RK4 matches `x(t)=cos(t), v(t)=-sin(t)` when `gamma=0, omega=1`.
- Energy decays when `gamma > 0`.
- State/history buffers survive normal stepping and are MemoryService-backed.

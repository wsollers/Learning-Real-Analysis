#pragma once
// sim/DifferentialSystem.hpp
// General differential equation systems and fixed-step ODE solvers.

#include "math/Scalars.hpp"
#include "memory/MemoryService.hpp"

#include <span>
#include <string>
#include <string_view>

namespace ndde::sim {

struct EquationSystemMetadata {
    std::string name;
    std::string formula;
    std::string variables;
};

class IDifferentialSystem {
public:
    virtual ~IDifferentialSystem() = default;

    [[nodiscard]] virtual EquationSystemMetadata metadata() const = 0;
    [[nodiscard]] virtual std::size_t dimension() const = 0;

    virtual void evaluate(f64 t,
                          std::span<const f64> state,
                          std::span<f64> derivative) const = 0;
};

class IOdeSolver {
public:
    virtual ~IOdeSolver() = default;

    [[nodiscard]] virtual std::string_view name() const = 0;
    [[nodiscard]] virtual std::size_t required_scratch_values(std::size_t dimension) const noexcept = 0;

    virtual void step(const IDifferentialSystem& system,
                      f64 t,
                      f64 dt,
                      std::span<f64> state,
                      std::span<f64> scratch) const = 0;
};

struct OdeHistorySample {
    f64 t = 0.0;
    std::size_t offset = 0;
};

class InitialValueProblem {
public:
    InitialValueProblem(memory::MemoryService& memory,
                        const IDifferentialSystem& system,
                        std::span<const f64> initial_state,
                        f64 t0 = 0.0)
        : m_memory(memory)
        , m_system(system)
        , m_initial(memory.simulation().make_vector<f64>())
        , m_state(memory.simulation().make_vector<f64>())
        , m_scratch(memory.simulation().make_vector<f64>())
        , m_history(memory.history().make_vector<OdeHistorySample>())
        , m_history_values(memory.history().make_vector<f64>())
        , m_t(t0)
        , m_t0(t0)
    {
        m_initial.assign(initial_state.begin(), initial_state.end());
        m_state.assign(initial_state.begin(), initial_state.end());
        record_history();
    }

    void reset() {
        m_state.assign(m_initial.begin(), m_initial.end());
        m_scratch.clear();
        m_history.clear();
        m_history_values.clear();
        m_t = m_t0;
        record_history();
    }

    void step(const IOdeSolver& solver, f64 dt) {
        const std::size_t required = solver.required_scratch_values(m_state.size());
        if (m_scratch.size() < required)
            m_scratch.resize(required);
        solver.step(m_system, m_t, dt, m_state, m_scratch);
        m_t += dt;
        record_history();
    }

    [[nodiscard]] const IDifferentialSystem& system() const noexcept { return m_system; }
    [[nodiscard]] std::span<const f64> initial_state() const noexcept { return m_initial; }
    [[nodiscard]] std::span<const f64> state() const noexcept { return m_state; }
    [[nodiscard]] f64 time() const noexcept { return m_t; }
    [[nodiscard]] std::size_t dimension() const noexcept { return m_state.size(); }
    [[nodiscard]] std::size_t history_size() const noexcept { return m_history.size(); }

    [[nodiscard]] std::span<const f64> history_state(std::size_t sample) const noexcept {
        if (sample >= m_history.size()) return {};
        const OdeHistorySample& h = m_history[sample];
        return std::span<const f64>{m_history_values.data() + h.offset, m_state.size()};
    }

    [[nodiscard]] f64 history_time(std::size_t sample) const noexcept {
        return sample < m_history.size() ? m_history[sample].t : 0.0;
    }

private:
    memory::MemoryService& m_memory;
    const IDifferentialSystem& m_system;
    memory::SimVector<f64> m_initial;
    memory::SimVector<f64> m_state;
    memory::SimVector<f64> m_scratch;
    memory::HistoryVector<OdeHistorySample> m_history;
    memory::HistoryVector<f64> m_history_values;
    f64 m_t = 0.0;
    f64 m_t0 = 0.0;

    void record_history() {
        const std::size_t offset = m_history_values.size();
        m_history.push_back(OdeHistorySample{.t = m_t, .offset = offset});
        for (const f64 value : m_state)
            m_history_values.push_back(value);
    }
};

class EulerOdeSolver final : public IOdeSolver {
public:
    [[nodiscard]] std::string_view name() const override { return "Euler"; }
    [[nodiscard]] std::size_t required_scratch_values(std::size_t dimension) const noexcept override {
        return dimension;
    }

    void step(const IDifferentialSystem& system,
              f64 t,
              f64 dt,
              std::span<f64> state,
              std::span<f64> scratch) const override
    {
        const std::size_t n = state.size();
        auto dydt = scratch.first(n);
        system.evaluate(t, state, dydt);
        for (std::size_t i = 0; i < n; ++i)
            state[i] += dt * dydt[i];
    }
};

class Rk4OdeSolver final : public IOdeSolver {
public:
    [[nodiscard]] std::string_view name() const override { return "RK4"; }
    [[nodiscard]] std::size_t required_scratch_values(std::size_t dimension) const noexcept override {
        return dimension * 5u;
    }

    void step(const IDifferentialSystem& system,
              f64 t,
              f64 dt,
              std::span<f64> state,
              std::span<f64> scratch) const override
    {
        const std::size_t n = state.size();
        auto k1 = scratch.subspan(0u, n);
        auto k2 = scratch.subspan(n, n);
        auto k3 = scratch.subspan(2u * n, n);
        auto k4 = scratch.subspan(3u * n, n);
        auto tmp = scratch.subspan(4u * n, n);

        system.evaluate(t, state, k1);

        for (std::size_t i = 0; i < n; ++i)
            tmp[i] = state[i] + 0.5 * dt * k1[i];
        system.evaluate(t + 0.5 * dt, tmp, k2);

        for (std::size_t i = 0; i < n; ++i)
            tmp[i] = state[i] + 0.5 * dt * k2[i];
        system.evaluate(t + 0.5 * dt, tmp, k3);

        for (std::size_t i = 0; i < n; ++i)
            tmp[i] = state[i] + dt * k3[i];
        system.evaluate(t + dt, tmp, k4);

        for (std::size_t i = 0; i < n; ++i)
            state[i] += (dt / 6.0) * (k1[i] + 2.0 * k2[i] + 2.0 * k3[i] + k4[i]);
    }
};

class ExponentialGrowthSystem final : public IDifferentialSystem {
public:
    explicit ExponentialGrowthSystem(f64 rate = 1.0) : m_rate(rate) {}

    [[nodiscard]] EquationSystemMetadata metadata() const override {
        return {
            .name = "Exponential growth",
            .formula = "y' = r y",
            .variables = "y"
        };
    }

    [[nodiscard]] std::size_t dimension() const override { return 1u; }

    void evaluate(f64, std::span<const f64> state, std::span<f64> derivative) const override {
        derivative[0] = m_rate * state[0];
    }

private:
    f64 m_rate = 1.0;
};

class HarmonicOscillatorSystem final : public IDifferentialSystem {
public:
    explicit HarmonicOscillatorSystem(f64 omega = 1.0) : m_omega(omega) {}

    [[nodiscard]] EquationSystemMetadata metadata() const override {
        return {
            .name = "Harmonic oscillator",
            .formula = "x' = v, v' = -omega^2 x",
            .variables = "x, v"
        };
    }

    [[nodiscard]] std::size_t dimension() const override { return 2u; }

    void evaluate(f64, std::span<const f64> state, std::span<f64> derivative) const override {
        derivative[0] = state[1];
        derivative[1] = -(m_omega * m_omega) * state[0];
    }

    [[nodiscard]] f64 omega() const noexcept { return m_omega; }

private:
    f64 m_omega = 1.0;
};

class DampedOscillatorSystem final : public IDifferentialSystem {
public:
    DampedOscillatorSystem(f64 omega = 1.0, f64 gamma = 0.15)
        : m_omega(omega)
        , m_gamma(gamma)
    {}

    [[nodiscard]] EquationSystemMetadata metadata() const override {
        return {
            .name = "Damped oscillator",
            .formula = "x' = v, v' = -2 gamma v - omega^2 x",
            .variables = "x, v"
        };
    }

    [[nodiscard]] std::size_t dimension() const override { return 2u; }

    void evaluate(f64, std::span<const f64> state, std::span<f64> derivative) const override {
        derivative[0] = state[1];
        derivative[1] = -2.0 * m_gamma * state[1] - (m_omega * m_omega) * state[0];
    }

    [[nodiscard]] f64 omega() const noexcept { return m_omega; }
    [[nodiscard]] f64 gamma() const noexcept { return m_gamma; }

private:
    f64 m_omega = 1.0;
    f64 m_gamma = 0.15;
};

class VanDerPolSystem final : public IDifferentialSystem {
public:
    explicit VanDerPolSystem(f64 mu = 1.4) : m_mu(mu) {}

    [[nodiscard]] EquationSystemMetadata metadata() const override {
        return {
            .name = "Van der Pol oscillator",
            .formula = "x' = v, v' = mu(1 - x^2)v - x",
            .variables = "x, v"
        };
    }

    [[nodiscard]] std::size_t dimension() const override { return 2u; }

    void evaluate(f64, std::span<const f64> state, std::span<f64> derivative) const override {
        derivative[0] = state[1];
        derivative[1] = m_mu * (1.0 - state[0] * state[0]) * state[1] - state[0];
    }

    [[nodiscard]] f64 mu() const noexcept { return m_mu; }

private:
    f64 m_mu = 1.4;
};

class PredatorPreySystem final : public IDifferentialSystem {
public:
    PredatorPreySystem(f64 alpha = 1.1, f64 beta = 0.4, f64 delta = 0.25, f64 gamma = 0.9)
        : m_alpha(alpha)
        , m_beta(beta)
        , m_delta(delta)
        , m_gamma(gamma)
    {}

    [[nodiscard]] EquationSystemMetadata metadata() const override {
        return {
            .name = "Predator-prey",
            .formula = "prey' = alpha prey - beta prey predator, predator' = delta prey predator - gamma predator",
            .variables = "prey, predator"
        };
    }

    [[nodiscard]] std::size_t dimension() const override { return 2u; }

    void evaluate(f64, std::span<const f64> state, std::span<f64> derivative) const override {
        const f64 prey = state[0];
        const f64 predator = state[1];
        derivative[0] = m_alpha * prey - m_beta * prey * predator;
        derivative[1] = m_delta * prey * predator - m_gamma * predator;
    }

private:
    f64 m_alpha = 1.1;
    f64 m_beta = 0.4;
    f64 m_delta = 0.25;
    f64 m_gamma = 0.9;
};

} // namespace ndde::sim

#include "sim/DifferentialSystem.hpp"

#include <gtest/gtest.h>

#include <cmath>
#include <span>

namespace {

using namespace ndde;

class HarmonicOscillatorSystem final : public sim::IDifferentialSystem {
public:
    [[nodiscard]] sim::EquationSystemMetadata metadata() const override {
        return {
            .name = "Harmonic oscillator",
            .formula = "x' = v, v' = -x",
            .variables = "x, v"
        };
    }

    [[nodiscard]] std::size_t dimension() const override { return 2u; }

    void evaluate(f64, std::span<const f64> state, std::span<f64> derivative) const override {
        derivative[0] = state[1];
        derivative[1] = -state[0];
    }
};

class LinearSystem2D final : public sim::IDifferentialSystem {
public:
    [[nodiscard]] sim::EquationSystemMetadata metadata() const override {
        return {
            .name = "Linear saddle",
            .formula = "x' = x, y' = -2y",
            .variables = "x, y"
        };
    }

    [[nodiscard]] std::size_t dimension() const override { return 2u; }

    void evaluate(f64, std::span<const f64> state, std::span<f64> derivative) const override {
        derivative[0] = state[0];
        derivative[1] = -2.0 * state[1];
    }
};

TEST(DifferentialSystem, Rk4SolvesExponentialGrowthInitialValueProblem) {
    memory::MemoryService memory;
    sim::ExponentialGrowthSystem system{1.0};
    sim::Rk4OdeSolver solver;
    const f64 initial[] = {1.0};
    sim::InitialValueProblem problem{memory, system, initial, 0.0};

    for (int i = 0; i < 10; ++i)
        problem.step(solver, 0.1);

    ASSERT_EQ(problem.dimension(), 1u);
    EXPECT_NEAR(problem.state()[0], std::exp(1.0), 2.5e-6);
    EXPECT_EQ(problem.history_size(), 11u);
    EXPECT_NEAR(problem.history_time(10), 1.0, 1e-12);
    EXPECT_NEAR(problem.history_state(0)[0], 1.0, 1e-12);
}

TEST(DifferentialSystem, EulerSolvesExponentialGrowthLessAccuratelyButInRightDirection) {
    memory::MemoryService memory;
    sim::ExponentialGrowthSystem system{1.0};
    sim::EulerOdeSolver solver;
    const f64 initial[] = {1.0};
    sim::InitialValueProblem problem{memory, system, initial, 0.0};

    for (int i = 0; i < 10; ++i)
        problem.step(solver, 0.1);

    EXPECT_NEAR(problem.state()[0], std::pow(1.1, 10.0), 1e-12);
    EXPECT_LT(problem.state()[0], std::exp(1.0));
}

TEST(DifferentialSystem, Rk4PreservesHarmonicOscillatorEnergyOverShortRun) {
    memory::MemoryService memory;
    HarmonicOscillatorSystem system;
    sim::Rk4OdeSolver solver;
    const f64 initial[] = {1.0, 0.0};
    sim::InitialValueProblem problem{memory, system, initial, 0.0};

    for (int i = 0; i < 100; ++i)
        problem.step(solver, 0.01);

    const f64 x = problem.state()[0];
    const f64 v = problem.state()[1];
    const f64 energy = 0.5 * (x * x + v * v);
    EXPECT_NEAR(energy, 0.5, 1e-9);
    EXPECT_NEAR(x, std::cos(1.0), 1e-9);
    EXPECT_NEAR(v, -std::sin(1.0), 1e-9);
}

TEST(DifferentialSystem, Rk4SolvesTwoDimensionalLinearSystem) {
    memory::MemoryService memory;
    LinearSystem2D system;
    sim::Rk4OdeSolver solver;
    const f64 initial[] = {1.0, 1.0};
    sim::InitialValueProblem problem{memory, system, initial, 0.0};

    for (int i = 0; i < 20; ++i)
        problem.step(solver, 0.05);

    EXPECT_NEAR(problem.state()[0], std::exp(1.0), 2.5e-7);
    EXPECT_NEAR(problem.state()[1], std::exp(-2.0), 2.5e-7);
}

TEST(DifferentialSystem, ProblemBuffersBindToMemoryServiceScopes) {
    memory::MemoryService memory;
    sim::ExponentialGrowthSystem system{1.0};
    sim::Rk4OdeSolver solver;
    const f64 initial[] = {1.0};
    sim::InitialValueProblem problem{memory, system, initial, 0.0};

    problem.step(solver, 0.1);

    EXPECT_EQ(problem.history_size(), 2u);
    EXPECT_EQ(problem.initial_state()[0], 1.0);
    EXPECT_GT(problem.state()[0], problem.initial_state()[0]);
}

} // namespace

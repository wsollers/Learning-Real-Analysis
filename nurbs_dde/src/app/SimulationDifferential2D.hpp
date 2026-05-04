#pragma once

#include "engine/ISimulation.hpp"
#include "engine/ScopedServiceHandles.hpp"
#include "memory/Unique.hpp"
#include "sim/DifferentialSystem.hpp"

#include <array>
#include <string_view>

namespace ndde {

class SimulationDifferential2D final : public ISimulation {
public:
    explicit SimulationDifferential2D(memory::MemoryService* memory = nullptr);

    [[nodiscard]] std::string_view name() const override { return "Differential Systems"; }
    void on_register(SimulationHost& host) override;
    void on_start() override;
    void on_tick(const TickInfo& tick) override;
    void on_stop() override;

    [[nodiscard]] SceneSnapshot snapshot() const override;
    [[nodiscard]] SimulationMetadata metadata() const override;

    [[nodiscard]] RenderViewId main_view_id() const noexcept { return m_main_view; }
    [[nodiscard]] RenderViewId alternate_view_id() const noexcept { return m_phase_view; }
    [[nodiscard]] std::size_t particle_count() const noexcept {
        return m_problem ? m_problem->history_size() : 0u;
    }

private:
    enum class SystemKind : int {
        Harmonic,
        Damped,
        VanDerPol,
        PredatorPrey
    };

    enum class SolverKind : int {
        Euler,
        RK4
    };

    SimulationHost* m_host = nullptr;
    ScopedServiceHandles<PanelHandle> m_panels;
    HotkeyHandle m_reset_hotkey;
    HotkeyHandle m_run_hotkey;
    RenderViewHandle m_main_handle;
    RenderViewHandle m_phase_handle;
    RenderViewId m_main_view = 0;
    RenderViewId m_phase_view = 0;

    memory::Unique<sim::IDifferentialSystem> m_system;
    memory::Unique<sim::InitialValueProblem> m_problem;
    sim::EulerOdeSolver m_euler;
    sim::Rk4OdeSolver m_rk4;

    SystemKind m_system_kind = SystemKind::VanDerPol;
    SolverKind m_solver_kind = SolverKind::RK4;
    std::array<double, 2> m_initial{2.0, 0.0};
    double m_step_size = 0.02;
    double m_sim_time = 0.0;
    double m_sim_speed = 1.0;
    bool m_paused = false;
    bool m_running = true;

    double m_omega = 1.0;
    double m_gamma = 0.15;
    double m_mu = 1.4;
    double m_alpha = 1.1;
    double m_beta = 0.4;
    double m_delta = 0.25;
    double m_predator_gamma = 0.9;

    void reset_problem();
    void step_problem(double dt);
    void draw_controls_panel();
    void draw_phase_panel();
    void submit_geometry();

    [[nodiscard]] const sim::IOdeSolver& solver() const noexcept;
    [[nodiscard]] RenderViewDomain phase_domain() const noexcept;
    [[nodiscard]] Mat4 phase_mvp() const noexcept;
    [[nodiscard]] Vec2 derivative_at(Vec2 state) const;
};

} // namespace ndde

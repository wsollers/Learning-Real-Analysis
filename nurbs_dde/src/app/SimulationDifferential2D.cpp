#include "app/SimulationDifferential2D.hpp"

#include "app/SimulationRenderPackets.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <imgui.h>

namespace ndde {

namespace {

constexpr const char* kSystemNames[] = {
    "Harmonic oscillator",
    "Damped oscillator",
    "Van der Pol",
    "Predator-prey"
};

constexpr const char* kSolverNames[] = {
    "Euler",
    "RK4"
};

void add_line(memory::FrameVector<Vertex>& out, Vec2 a, Vec2 b, Vec4 color) {
    out.push_back(Vertex{.pos = {a.x, a.y, 0.f}, .color = color});
    out.push_back(Vertex{.pos = {b.x, b.y, 0.f}, .color = color});
}

} // namespace

SimulationDifferential2D::SimulationDifferential2D(memory::MemoryService*) {}

void SimulationDifferential2D::on_register(SimulationHost& host) {
    m_host = &host;
    m_panels.add(host.panels().register_panel(PanelDescriptor{
        .title = "Sim - ODE Controls",
        .category = "Simulation",
        .scope = PanelScope::Simulation,
        .draw = [this] { draw_controls_panel(); }
    }));
    m_panels.add(host.panels().register_panel(PanelDescriptor{
        .title = "Sim - Phase Space",
        .category = "Simulation",
        .scope = PanelScope::Simulation,
        .draw = [this] { draw_phase_panel(); }
    }));

    m_reset_hotkey = host.hotkeys().register_action(HotkeyDescriptor{
        .chord = {.key = 'R', .mods = 2},
        .label = "Reset differential system",
        .callback = [this] { reset_problem(); }
    });
    m_run_hotkey = host.hotkeys().register_action(HotkeyDescriptor{
        .chord = {.key = ' ', .mods = 2},
        .label = "Run differential system",
        .callback = [this] { m_running = !m_running; }
    });

    m_main_handle = host.render().register_view(RenderViewDescriptor{
        .title = "Differential 2D",
        .kind = RenderViewKind::Main,
        .projection = CameraProjection::Orthographic,
        .camera_profile = CameraViewProfile::Orthographic2D,
        .overlays = {.show_axes = true, .show_grid = true}
    }, &m_main_view);
    m_phase_handle = host.render().register_view(RenderViewDescriptor{
        .title = "Phase Space",
        .kind = RenderViewKind::Alternate,
        .alternate_mode = AlternateViewMode::VectorField,
        .projection = CameraProjection::Orthographic,
        .camera_profile = CameraViewProfile::Orthographic2D,
        .overlays = {.show_axes = true}
    }, &m_phase_view);

    reset_problem();
}

void SimulationDifferential2D::on_start() {
    if (!m_problem)
        reset_problem();
}

void SimulationDifferential2D::on_tick(const TickInfo& tick) {
    apply_phase_pick_commands();
    if (!tick.paused && !m_paused && m_running) {
        const double scaled_dt = static_cast<double>(tick.dt) * m_sim_speed;
        step_problem(scaled_dt);
        m_sim_time += scaled_dt;
    }
    submit_geometry();
}

void SimulationDifferential2D::on_stop() {
    m_panels.clear();
    m_reset_hotkey.reset();
    m_run_hotkey.reset();
    m_main_handle.reset();
    m_phase_handle.reset();
    m_problem.reset();
    m_system.reset();
    m_host = nullptr;
}

SceneSnapshot SimulationDifferential2D::snapshot() const {
    memory::FrameVector<ParticleSnapshot> particles =
        m_host ? m_host->memory().frame().make_vector<ParticleSnapshot>() : memory::FrameVector<ParticleSnapshot>{};
    if (m_problem && m_problem->state().size() >= 2u) {
        const auto state = m_problem->state();
        particles.push_back(ParticleSnapshot{
            .id = 1,
            .role = "State",
            .label = "ODE state",
            .u = static_cast<float>(state[0]),
            .v = static_cast<float>(state[1]),
            .x = static_cast<float>(state[0]),
            .y = static_cast<float>(state[1]),
            .z = 0.f
        });
    }
    return SceneSnapshot{
        .name = std::string(name()),
        .paused = m_paused || !m_running,
        .sim_time = static_cast<float>(m_problem ? m_problem->time() : m_sim_time),
        .sim_speed = static_cast<float>(m_sim_speed),
        .particle_count = particle_count(),
        .status = m_running ? "Running" : "Paused",
        .particles = std::move(particles)
    };
}

SimulationMetadata SimulationDifferential2D::metadata() const {
    const sim::EquationSystemMetadata meta = m_system ? m_system->metadata() : sim::EquationSystemMetadata{};
    return SimulationMetadata{
        .name = std::string(name()),
        .surface_name = "Phase space",
        .surface_formula = meta.formula,
        .status = m_running ? "Running" : "Paused",
        .sim_time = static_cast<float>(m_problem ? m_problem->time() : m_sim_time),
        .sim_speed = static_cast<float>(m_sim_speed),
        .particle_count = particle_count(),
        .paused = m_paused || !m_running,
        .surface_has_analytic_derivatives = true,
        .surface_deformable = false,
        .surface_time_varying = false
    };
}

void SimulationDifferential2D::reset_problem() {
    if (!m_host) return;
    auto& sim_memory = m_host->memory().simulation();
    m_problem.reset();
    m_system.reset();

    switch (m_system_kind) {
        case SystemKind::Harmonic:
            m_system = sim_memory.make_unique_as<sim::IDifferentialSystem, sim::HarmonicOscillatorSystem>(m_omega);
            break;
        case SystemKind::Damped:
            m_system = sim_memory.make_unique_as<sim::IDifferentialSystem, sim::DampedOscillatorSystem>(m_omega, m_gamma);
            break;
        case SystemKind::VanDerPol:
            m_system = sim_memory.make_unique_as<sim::IDifferentialSystem, sim::VanDerPolSystem>(m_mu);
            break;
        case SystemKind::PredatorPrey:
            m_initial[0] = std::max(m_initial[0], 0.05);
            m_initial[1] = std::max(m_initial[1], 0.05);
            m_system = sim_memory.make_unique_as<sim::IDifferentialSystem, sim::PredatorPreySystem>(
                m_alpha, m_beta, m_delta, m_predator_gamma);
            break;
    }

    m_problem = sim_memory.make_unique<sim::InitialValueProblem>(
        m_host->memory(), *m_system, std::span<const double>{m_initial.data(), m_initial.size()});
    m_sim_time = 0.0;
}

void SimulationDifferential2D::step_problem(double dt) {
    if (!m_problem || dt <= 0.0) return;
    double remaining = dt;
    const double max_step = std::max(m_step_size, 1.0e-4);
    while (remaining > 1.0e-12) {
        const double h = std::min(max_step, remaining);
        m_problem->step(solver(), h);
        remaining -= h;
    }
}

void SimulationDifferential2D::draw_controls_panel() {
    ImGui::SetNextWindowPos(ImVec2(24.f, 72.f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(340.f, 360.f), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Sim - ODE Controls")) { ImGui::End(); return; }

    int system_index = static_cast<int>(m_system_kind);
    if (ImGui::Combo("System", &system_index, kSystemNames, IM_ARRAYSIZE(kSystemNames)))
        m_system_kind = static_cast<SystemKind>(system_index);

    int solver_index = static_cast<int>(m_solver_kind);
    if (ImGui::Combo("Solver", &solver_index, kSolverNames, IM_ARRAYSIZE(kSolverNames)))
        m_solver_kind = static_cast<SolverKind>(solver_index);

    ImGui::Checkbox("Run", &m_running);
    ImGui::Checkbox("Paused", &m_paused);
    float speed = static_cast<float>(m_sim_speed);
    if (ImGui::SliderFloat("Sim speed", &speed, 0.05f, 4.f, "%.2f"))
        m_sim_speed = speed;
    float step = static_cast<float>(m_step_size);
    if (ImGui::SliderFloat("Step size", &step, 0.001f, 0.12f, "%.3f"))
        m_step_size = step;

    float initial_x = static_cast<float>(m_initial[0]);
    float initial_y = static_cast<float>(m_initial[1]);
    if (ImGui::SliderFloat("Initial x", &initial_x, -4.f, 6.f, "%.2f")) m_initial[0] = initial_x;
    if (ImGui::SliderFloat("Initial y", &initial_y, -4.f, 6.f, "%.2f")) m_initial[1] = initial_y;

    ImGui::SeparatorText("Parameters");
    float omega = static_cast<float>(m_omega);
    float gamma = static_cast<float>(m_gamma);
    float mu = static_cast<float>(m_mu);
    if (m_system_kind == SystemKind::Harmonic || m_system_kind == SystemKind::Damped) {
        if (ImGui::SliderFloat("omega", &omega, 0.1f, 3.f, "%.2f")) m_omega = omega;
    }
    if (m_system_kind == SystemKind::Damped) {
        if (ImGui::SliderFloat("gamma", &gamma, 0.f, 1.2f, "%.2f")) m_gamma = gamma;
    }
    if (m_system_kind == SystemKind::VanDerPol) {
        if (ImGui::SliderFloat("mu", &mu, 0.1f, 5.f, "%.2f")) m_mu = mu;
    }
    if (m_system_kind == SystemKind::PredatorPrey) {
        float alpha = static_cast<float>(m_alpha);
        float beta = static_cast<float>(m_beta);
        float delta = static_cast<float>(m_delta);
        float pred_gamma = static_cast<float>(m_predator_gamma);
        if (ImGui::SliderFloat("alpha", &alpha, 0.1f, 3.f, "%.2f")) m_alpha = alpha;
        if (ImGui::SliderFloat("beta", &beta, 0.05f, 2.f, "%.2f")) m_beta = beta;
        if (ImGui::SliderFloat("delta", &delta, 0.05f, 2.f, "%.2f")) m_delta = delta;
        if (ImGui::SliderFloat("pred gamma", &pred_gamma, 0.1f, 3.f, "%.2f")) m_predator_gamma = pred_gamma;
    }

    if (ImGui::Button("Reset [Ctrl+R]")) reset_problem();
    ImGui::SameLine();
    if (ImGui::Button("Step")) step_problem(m_step_size);

    ImGui::End();
}

void SimulationDifferential2D::draw_phase_panel() {
    ImGui::SetNextWindowPos(ImVec2(386.f, 72.f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(360.f, 250.f), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Sim - Phase Space")) { ImGui::End(); return; }

    if (m_problem && m_system) {
        const auto meta = m_system->metadata();
        ImGui::TextUnformatted(meta.name.c_str());
        ImGui::TextWrapped("%s", meta.formula.c_str());
        const auto state = m_problem->state();
        if (state.size() >= 2u) {
            ImGui::Text("t %.3f", m_problem->time());
            ImGui::Text("state (%.6f, %.6f)", state[0], state[1]);
            ImGui::Text("history %zu", m_problem->history_size());
            if (m_system_kind == SystemKind::Harmonic || m_system_kind == SystemKind::Damped) {
                const double energy = 0.5 * (state[1] * state[1] + m_omega * m_omega * state[0] * state[0]);
                ImGui::Text("energy %.6f", energy);
            }
        }
    }

    ImGui::End();
}

void SimulationDifferential2D::submit_geometry() {
    if (!m_host || !m_problem || !m_system) return;
    auto& render = m_host->render();
    auto& memory = m_host->memory();
    const RenderViewDomain domain = phase_domain();
    render.set_view_domain(m_main_view, domain);
    render.set_view_domain(m_phase_view, domain);

    update_hover();
    const Mat4 main_mvp = phase_mvp(m_main_view);
    const Mat4 phase_view_mvp = phase_mvp(m_phase_view);
    memory::FrameVector<Vertex> axes = memory.frame().make_vector<Vertex>();
    add_line(axes, {domain.u_min, 0.f}, {domain.u_max, 0.f}, {0.86f, 0.20f, 0.18f, 1.f});
    add_line(axes, {0.f, domain.v_min}, {0.f, domain.v_max}, {0.20f, 0.82f, 0.34f, 1.f});
    render.submit(m_main_view, axes, Topology::LineList, DrawMode::VertexColor, {1, 1, 1, 1}, main_mvp);
    render.submit(m_phase_view, axes, Topology::LineList, DrawMode::VertexColor, {1, 1, 1, 1}, phase_view_mvp);

    memory::FrameVector<Vertex> field = memory.frame().make_vector<Vertex>();
    constexpr int samples = 17;
    const float du = (domain.u_max - domain.u_min) / static_cast<float>(samples - 1);
    const float dv = (domain.v_max - domain.v_min) / static_cast<float>(samples - 1);
    for (int iy = 0; iy < samples; ++iy) {
        for (int ix = 0; ix < samples; ++ix) {
            const Vec2 p{domain.u_min + du * ix, domain.v_min + dv * iy};
            Vec2 d = derivative_at(p);
            const float len = std::sqrt(d.x * d.x + d.y * d.y);
            if (len < 1.0e-5f) continue;
            d = d / len * std::min(du, dv) * 0.34f;
            add_line(field, p - d * 0.5f, p + d * 0.5f, {0.32f, 0.58f, 0.95f, 0.58f});
        }
    }
    render.submit(m_phase_view, field, Topology::LineList, DrawMode::VertexColor, {1, 1, 1, 1}, phase_view_mvp);

    memory::FrameVector<Vertex> trajectory = memory.frame().make_vector<Vertex>();
    trajectory.reserve(m_problem->history_size());
    for (std::size_t i = 0; i < m_problem->history_size(); ++i) {
        const auto sample = m_problem->history_state(i);
        if (sample.size() < 2u) continue;
        trajectory.push_back(Vertex{
            .pos = {static_cast<float>(sample[0]), static_cast<float>(sample[1]), 0.f},
            .color = {1.f, 0.72f, 0.18f, 1.f}
        });
    }
    render.submit(m_main_view, trajectory, Topology::LineStrip, DrawMode::VertexColor, {1, 1, 1, 1}, main_mvp);
    render.submit(m_phase_view, trajectory, Topology::LineStrip, DrawMode::VertexColor, {1, 1, 1, 1}, phase_view_mvp);

    const auto state = m_problem->state();
    if (state.size() >= 2u) {
        const Vec2 p{static_cast<float>(state[0]), static_cast<float>(state[1])};
        const float r = std::min(domain.u_max - domain.u_min, domain.v_max - domain.v_min) * 0.018f;
        memory::FrameVector<Vertex> marker = memory.frame().make_vector<Vertex>();
        add_line(marker, p + Vec2{-r, 0.f}, p + Vec2{r, 0.f}, {1.f, 0.95f, 0.20f, 1.f});
        add_line(marker, p + Vec2{0.f, -r}, p + Vec2{0.f, r}, {1.f, 0.95f, 0.20f, 1.f});
        render.submit(m_main_view, marker, Topology::LineList, DrawMode::VertexColor, {1, 1, 1, 1}, main_mvp);
        render.submit(m_phase_view, marker, Topology::LineList, DrawMode::VertexColor, {1, 1, 1, 1}, phase_view_mvp);
    }
}

void SimulationDifferential2D::update_hover() {
    if (!m_host || !m_problem) return;
    auto& interaction = m_host->interaction();
    auto& render = m_host->render();
    RenderViewId view = m_phase_view;
    ViewMouseState mouse = interaction.mouse_state(view);
    if (!mouse.enabled) {
        view = m_main_view;
        mouse = interaction.mouse_state(view);
    }
    if (!mouse.enabled) {
        interaction.set_hover_hits(SurfaceHit{.view = view}, ParticleTrailHit{.view = view});
        return;
    }

    const Mat4 mvp = phase_mvp(view);
    const Mat4 inv = glm::inverse(mvp);
    const glm::vec4 world4 = inv * glm::vec4(mouse.ndc.x, mouse.ndc.y, 0.f, 1.f);
    SurfaceHit surface{.view = view};
    if (std::abs(world4.w) > 1.0e-6f) {
        const Vec3 world = Vec3{world4.x, world4.y, world4.z} / world4.w;
        const RenderViewDomain d = phase_domain();
        surface.uv = {std::clamp(world.x, d.u_min, d.u_max), std::clamp(world.y, d.v_min, d.v_max)};
        surface.world = {surface.uv.x, surface.uv.y, 0.f};
        surface.hit = true;
    }

    ParticleTrailHit trail{.view = view};
    const RenderViewDescriptor* desc = render.descriptor(view);
    const Vec2 viewport = desc ? desc->viewport_size : Vec2{1.f, 1.f};
    float best = std::max(mouse.snap_radius_px, 1.f);
    for (std::size_t i = 0; i < m_problem->history_size(); ++i) {
        const auto sample = m_problem->history_state(i);
        if (sample.size() < 2u) continue;
        const Vec3 world{static_cast<float>(sample[0]), static_cast<float>(sample[1]), 0.f};
        const glm::vec4 clip = mvp * glm::vec4(world, 1.f);
        if (std::abs(clip.w) < 1.0e-6f) continue;
        const Vec2 pixel{
            (clip.x / clip.w + 1.f) * 0.5f * viewport.x,
            (1.f - clip.y / clip.w) * 0.5f * viewport.y
        };
        const float dx = pixel.x - mouse.pixel.x;
        const float dy = pixel.y - mouse.pixel.y;
        const float distance = std::sqrt(dx * dx + dy * dy);
        if (distance < best) {
            best = distance;
            trail.particle_id = 1;
            trail.particle_index = 0;
            trail.trail_index = static_cast<u32>(i);
            trail.world = world;
            trail.pixel_distance = distance;
            trail.hit = true;
        }
    }

    interaction.set_hover_hits(surface, trail);
}

void SimulationDifferential2D::apply_phase_pick_commands() {
    if (!m_host) return;
    const RenderViewDomain domain = phase_domain();
    m_host->render().set_view_domain(m_main_view, domain);
    m_host->render().set_view_domain(m_phase_view, domain);
    auto consume = [this](RenderViewId view) {
        for (const ViewPointPickRequest& pick : m_host->interaction().consume_view_point_picks(view))
            set_initial_from_phase_point(view, pick.screen_ndc);
    };
    consume(m_main_view);
    consume(m_phase_view);
}

void SimulationDifferential2D::set_initial_from_phase_point(RenderViewId view, Vec2 screen_ndc) {
    if (!m_host || view == 0) return;
    const Mat4 inv = glm::inverse(phase_mvp(view));
    const glm::vec4 world4 = inv * glm::vec4(screen_ndc.x, screen_ndc.y, 0.f, 1.f);
    if (std::abs(world4.w) < 1.0e-6f)
        return;

    const Vec3 world = Vec3{world4.x, world4.y, world4.z} / world4.w;
    const RenderViewDomain d = phase_domain();
    const float x = std::clamp(world.x, d.u_min, d.u_max);
    const float y = std::clamp(world.y, d.v_min, d.v_max);
    m_initial[0] = x;
    m_initial[1] = y;
    if (m_system_kind == SystemKind::PredatorPrey) {
        m_initial[0] = std::max(m_initial[0], 0.05);
        m_initial[1] = std::max(m_initial[1], 0.05);
    }
    reset_problem();
}

const sim::IOdeSolver& SimulationDifferential2D::solver() const noexcept {
    return m_solver_kind == SolverKind::RK4
        ? static_cast<const sim::IOdeSolver&>(m_rk4)
        : static_cast<const sim::IOdeSolver&>(m_euler);
}

RenderViewDomain SimulationDifferential2D::phase_domain() const noexcept {
    if (m_system_kind == SystemKind::PredatorPrey) {
        return {.u_min = 0.f, .u_max = 6.f, .v_min = 0.f, .v_max = 6.f, .z_min = -1.f, .z_max = 1.f};
    }
    return {.u_min = -4.f, .u_max = 4.f, .v_min = -4.f, .v_max = 4.f, .z_min = -1.f, .z_max = 1.f};
}

Mat4 SimulationDifferential2D::phase_mvp(RenderViewId view) const noexcept {
    return m_host ? m_host->camera().orthographic_mvp(view, 0.05f) : Mat4{1.f};
}

Vec2 SimulationDifferential2D::derivative_at(Vec2 state) const {
    if (!m_system) return {};
    const double in[] = {state.x, state.y};
    double out[] = {0.0, 0.0};
    m_system->evaluate(m_problem ? m_problem->time() : 0.0,
                       std::span<const double>{in, 2u},
                       std::span<double>{out, 2u});
    return {static_cast<float>(out[0]), static_cast<float>(out[1])};
}

} // namespace ndde

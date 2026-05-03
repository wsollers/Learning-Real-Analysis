#include "app/SurfaceSimScene.hpp"
#include "app/ParticleBehaviors.hpp"
#include "numeric/ops.hpp"
#include "sim/GradientWalker.hpp"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>

namespace ndde {

// ── swap_surface ──────────────────────────────────────────────────────────────

void SurfaceSimScene::swap_surface(SurfaceType type) {
    switch (type) {
        case SurfaceType::Gaussian:
            m_surface = std::make_unique<GaussianSurface>();
            break;
        case SurfaceType::Torus:
            m_surface = std::make_unique<ndde::math::Torus>(m_surface_state.torus_R, m_surface_state.torus_r);
            break;
        case SurfaceType::GaussianRipple: {
            auto s = std::make_unique<GaussianRipple>(m_surface_state.ripple_params);
            m_surface = std::move(s);
            break;
        }
        case SurfaceType::Extremum:
            m_surface = std::make_unique<ndde::math::ExtremumSurface>();
            m_pursuit.extremum_table.build(*m_surface);
            break;
    }
    m_surface_state.type    = type;
    m_particles.set_surface(m_surface.get());
    m_sim_time        = 0.f;
    m_display.mesh.mark_dirty();
    m_spawn.spawning_pursuer = false;

    std::vector<ParticleRole> roles;
    roles.reserve(m_curves.size());
    for (const auto& curve : m_curves)
        roles.push_back(curve.particle_role());
    m_particles.clear();
    m_spawn.leader_count = 0;
    m_spawn.chaser_count = 0;

    const u32 n  = static_cast<u32>(roles.size());
    const f32 u0 = m_surface->u_min(), u1 = m_surface->u_max();
    const f32 v0 = m_surface->v_min(), v1 = m_surface->v_max();
    const f32 um = m_surface->is_periodic_u() ? 0.f : (u1-u0)*0.08f;
    const f32 vm = m_surface->is_periodic_v() ? 0.f : (v1-v0)*0.08f;
    const f32 u_lo = u0+um, u_hi = u1-um;
    const f32 v_lo = v0+vm, v_hi = v1-vm;

    for (u32 i = 0; i < n; ++i) {
        const f32 su = (static_cast<f32>(i)+0.5f)/static_cast<f32>(std::max(n,1u));
        const f32 sv = std::fmod(su * 2.618033988f, 1.f);
        spawn_gradient_particle(roles[i], {u_lo + su*(u_hi-u_lo), v_lo + sv*(v_hi-v_lo)});
        prewarm_particle(m_curves.back(), 60);
    }
}

// ── advance_simulation ────────────────────────────────────────────────────────

void SurfaceSimScene::advance_simulation(f32 dt) {
    if (!m_paused) {
        if (auto* def = dynamic_cast<ndde::math::IDeformableSurface*>(m_surface.get()))
            def->advance(dt * m_sim_speed);
        m_sim_time += dt * m_sim_speed;

        rebuild_extremum_table_if_needed();

        m_particles.update(dt, m_sim_speed, m_sim_time);
    }
    update_snapshot_particles(m_particles);
}

// ── sync_pairwise_constraints ─────────────────────────────────────────────────

void SurfaceSimScene::sync_pairwise_constraints() {
    m_particles.clear_pair_constraints();
    if (m_pursuit.pair_collision)
        m_particles.add_pair_constraint<ndde::sim::MinDistConstraint>(m_pursuit.min_dist);
}

glm::vec2 SurfaceSimScene::reference_uv() const noexcept {
    if (m_curves.empty())
        return {(m_surface->u_min() + m_surface->u_max()) * 0.5f,
                (m_surface->v_min() + m_surface->v_max()) * 0.5f};
    return m_curves[0].head_uv();
}

glm::vec2 SurfaceSimScene::offset_spawn(glm::vec2 ref_uv, float radius, float angle) const noexcept {
    constexpr float margin = 0.5f;
    return {
        std::clamp(ref_uv.x + radius * ops::cos(angle),
                   m_surface->u_min() + margin, m_surface->u_max() - margin),
        std::clamp(ref_uv.y + radius * ops::sin(angle),
                   m_surface->v_min() + margin, m_surface->v_max() - margin)
    };
}

void SurfaceSimScene::prewarm_particle(AnimatedCurve& particle, int frames) {
    SimulationContext context = m_particles.context(m_sim_time);
    particle.set_behavior_context(&context);
    for (int i = 0; i < frames; ++i) {
        context.set_time(m_sim_time + static_cast<float>(i) / 60.f);
        particle.advance(1.f / 60.f, m_sim_speed);
    }
    particle.set_behavior_context(nullptr);
}

void SurfaceSimScene::spawn_gradient_particle(ParticleRole role, glm::vec2 uv) {
    ParticleBuilder builder = m_particles.factory().particle();
    builder
        .named(std::string(role_name(role)) + " - Gradient Walker")
        .role(role)
        .at(uv)
        .trail({TrailMode::Finite, AnimatedCurve::MAX_TRAIL, 0.015f})
        .with_equation(std::make_unique<ndde::sim::GradientWalker>());

    AnimatedCurve& particle = m_particles.spawn(std::move(builder));
    prewarm_particle(particle);

    if (role == ParticleRole::Leader) ++m_spawn.leader_count;
    else if (role == ParticleRole::Chaser) ++m_spawn.chaser_count;
}

void SurfaceSimScene::spawn_brownian_particle(glm::vec2 uv) {
    ParticleBuilder builder = m_particles.factory().particle();
    builder
        .named("Chaser - Brownian")
        .role(ParticleRole::Chaser)
        .at(uv)
        .trail({TrailMode::Finite, AnimatedCurve::MAX_TRAIL, 0.015f})
        .stochastic()
        .with_equation(std::make_unique<ndde::sim::BrownianMotion>(m_behavior_params.brownian));

    AnimatedCurve& particle = m_particles.spawn(std::move(builder));
    prewarm_particle(particle);
    ++m_spawn.chaser_count;
}

void SurfaceSimScene::spawn_delay_pursuit_particle(glm::vec2 uv) {
    if (m_curves.empty()) return;
    if (m_curves[0].history() == nullptr) {
        const std::size_t cap =
            static_cast<std::size_t>(std::ceil(m_behavior_params.delay_pursuit.tau * 120.f * 1.5f)) + 256;
        m_curves[0].enable_history(cap, 1.f / 120.f);
    }

    ParticleBuilder builder = m_particles.factory().particle();
    builder
        .named("Chaser - Delay Pursuit")
        .role(ParticleRole::Chaser)
        .at(uv)
        .trail({TrailMode::Finite, AnimatedCurve::MAX_TRAIL, 0.015f})
        .stochastic()
        .with_equation(std::make_unique<ndde::sim::DelayPursuitEquation>(
            m_curves[0].history(), m_surface.get(), m_behavior_params.delay_pursuit));
    m_particles.spawn(std::move(builder));
    ++m_spawn.chaser_count;
    ++m_spawn.delay_pursuit_count;
}

void SurfaceSimScene::spawn_showcase_service() {
    m_curves.clear();
    m_particles.clear_goals();
    m_spawn.leader_count = 0;
    m_spawn.chaser_count = 0;
    m_spawn.delay_pursuit_count = 0;
    m_spawn.spawning_pursuer = true;

    const std::size_t history_cap =
        static_cast<std::size_t>(std::ceil(2.0f * 120.f * 1.5f)) + 256;

    ParticleBuilder leader_builder = m_particles.factory().particle();
    leader_builder
        .named("Leader - Brownian - Bias Drift")
        .role(ParticleRole::Leader)
        .at({-4.5f, -4.0f})
        .history(history_cap, 1.f / 120.f)
        .trail({TrailMode::Finite, AnimatedCurve::MAX_TRAIL, 0.015f})
        .stochastic()
        .with_behavior<ConstantDriftBehavior>(0.75f, glm::vec2{0.55f, 0.28f})
        .with_behavior<BrownianBehavior>(BrownianBehavior::Params{
            .sigma = 0.16f,
            .drift_strength = 0.08f
        });
    AnimatedCurve& leader = m_particles.spawn(std::move(leader_builder));
    ++m_spawn.leader_count;

    for (int i = 0; i < 240; ++i) {
        m_sim_time += 1.f / 60.f;
        m_particles.update(1.f / 60.f, 1.f, m_sim_time);
    }

    ndde::SeekParticleBehavior::Params seek;
    seek.target = TargetSelector::nearest(ParticleRole::Leader);
    seek.speed = 0.95f;
    seek.delay_seconds = 1.0f;

    const glm::vec2 uv = offset_spawn(leader.head_uv(), 2.3f, 0.4f);
    ParticleBuilder seeker_builder = m_particles.factory().particle();
    seeker_builder
        .named("Chaser - Delayed Seek - Brownian")
        .role(ParticleRole::Chaser)
        .at(uv)
        .trail({TrailMode::Finite, AnimatedCurve::MAX_TRAIL, 0.015f})
        .stochastic()
        .with_behavior<SeekParticleBehavior>(seek)
        .with_behavior<BrownianBehavior>(0.25f, BrownianBehavior::Params{
            .sigma = 0.06f,
            .drift_strength = 0.f
        });
    m_particles.spawn(std::move(seeker_builder));
    ++m_spawn.chaser_count;
    ++m_spawn.delay_pursuit_count;

    m_particles.add_goal<CaptureGoal>(CaptureGoal::Params{
        .seeker_role = ParticleRole::Chaser,
        .target_role = ParticleRole::Leader,
        .radius = 0.22f
    });
    m_goal_status = GoalStatus::Running;
}

// ── Extremum helpers ──────────────────────────────────────────────────────────

void SurfaceSimScene::rebuild_extremum_table_if_needed() {
    if (m_surface_state.type != SurfaceType::Extremum) return;
    if (m_pursuit.extremum_rebuild_countdown == 0) {
        m_pursuit.extremum_table.build(*m_surface, m_sim_time);
        m_pursuit.extremum_rebuild_countdown = 30u;
    } else {
        --m_pursuit.extremum_rebuild_countdown;
    }
}

void SurfaceSimScene::spawn_leader_seeker() {
    const float u_mid = 0.5f*(m_surface->u_min() + m_surface->u_max());
    const float v_mid = 0.5f*(m_surface->v_min() + m_surface->v_max());

    std::unique_ptr<ndde::sim::IEquation> eq;
    if (m_pursuit.leader_mode == LeaderMode::Deterministic)
        eq = std::make_unique<ndde::sim::LeaderSeekerEquation>(
                &m_pursuit.extremum_table, m_pursuit.leader_seeker);
    else
        eq = std::make_unique<ndde::sim::BiasedBrownianLeader>(
                &m_pursuit.extremum_table, m_pursuit.biased_brownian_leader);

    // Enable history so pursuers can use it
    const std::size_t cap =
        static_cast<std::size_t>(std::ceil(m_pursuit.pursuit_tau * 120.f * 1.5f)) + 256;

    auto builder = m_particles.factory().particle();
    builder
        .named(m_pursuit.leader_mode == LeaderMode::Deterministic
            ? "Leader - Extremum Seeker"
            : "Leader - Biased Brownian")
        .role(ParticleRole::Leader)
        .at({u_mid, v_mid})
        .history(cap, 1.f / 120.f)
        .trail({TrailMode::Finite, AnimatedCurve::MAX_TRAIL, 0.015f})
        .stochastic()
        .with_equation(std::move(eq));
    m_particles.spawn(std::move(builder));
    ++m_spawn.leader_count;
    m_spawn.spawning_pursuer = true;
}

void SurfaceSimScene::spawn_pursuit_particle() {
    if (m_curves.empty()) return;

    // Ensure leader has history
    if (m_curves[0].history() == nullptr) {
        const std::size_t cap =
            static_cast<std::size_t>(std::ceil(m_pursuit.pursuit_tau * 120.f * 1.5f)) + 256;
        m_curves[0].enable_history(cap, 1.f / 120.f);
    }

    const glm::vec2 ref = m_curves[0].head_uv();
    const glm::vec2 uv  = offset_spawn(ref, 2.0f,
        static_cast<float>(m_spawn.delay_pursuit_count) * 1.1f + 0.3f);

    std::unique_ptr<ndde::sim::IEquation> eq;
    switch (m_pursuit.pursuit_mode) {
        case PursuitMode::Direct:
            eq = std::make_unique<ndde::sim::DirectPursuitEquation>(
                [this]{ return m_curves[0].head_uv(); },
                ndde::sim::DirectPursuitEquation::Params{
                    m_pursuit.leader_seeker.pursuit_speed, 0.f });
            break;
        case PursuitMode::Delayed:
            eq = std::make_unique<ndde::sim::DelayPursuitEquation>(
                m_curves[0].history(), m_surface.get(),
                ndde::sim::DelayPursuitEquation::Params{
                    m_pursuit.pursuit_tau, m_pursuit.leader_seeker.pursuit_speed, 0.f });
            break;
        case PursuitMode::Momentum:
            eq = std::make_unique<ndde::sim::MomentumBearingEquation>(
                m_curves[0].history(),
                ndde::sim::MomentumBearingEquation::Params{
                    m_pursuit.leader_seeker.pursuit_speed, m_pursuit.pursuit_window, 0.f });
            break;
    }

    auto builder = m_particles.factory().particle();
    builder
        .named("Chaser - Pursuit")
        .role(ParticleRole::Chaser)
        .at(uv)
        .trail({TrailMode::Finite, AnimatedCurve::MAX_TRAIL, 0.015f})
        .stochastic()
        .with_equation(std::move(eq));
    m_particles.spawn(std::move(builder));
    ++m_spawn.chaser_count;
    ++m_spawn.delay_pursuit_count;
}

void SurfaceSimScene::draw_leader_seeker_panel() {
    if (m_surface_state.type != SurfaceType::Extremum) return;
    ImGui::SeparatorText("Leader Seeker  [Ctrl+A]");

    // Leader mode
    {
        int mode = static_cast<int>(m_pursuit.leader_mode);
        bool changed = false;
        changed |= ImGui::RadioButton("Deterministic##lm", &mode, 0);
        ImGui::SameLine();
        changed |= ImGui::RadioButton("Biased Brownian##lm", &mode, 1);
        if (changed) m_pursuit.leader_mode = static_cast<LeaderMode>(mode);
    }

    if (m_pursuit.leader_mode == LeaderMode::Deterministic) {
        auto& p = m_pursuit.leader_seeker;
        ImGui::SliderFloat("Target grad mag##ls",  &p.target_grad_magnitude, 0.f, 2.f,  "%.2f");
        ImGui::SliderFloat("Epsilon##ls",           &p.epsilon,               0.01f, 0.5f, "%.3f");
        ImGui::SliderFloat("Leader speed##ls",      &p.pursuit_speed,         0.1f, 3.f,  "%.2f");
        ImGui::SliderFloat("Leader noise##ls",      &p.noise_sigma,           0.f,  1.f,  "%.3f");
        ImGui::SliderFloat("Arrival radius##ls",    &p.arrival_radius,        0.1f, 2.f,  "%.2f");
    } else {
        auto& p = m_pursuit.biased_brownian_leader;
        ImGui::SliderFloat("Sigma##bbl",             &p.sigma,           0.01f, 2.f,  "%.3f");
        ImGui::SliderFloat("Goal drift##bbl",        &p.drift_strength,  0.f,   2.f,  "%.2f");
        ImGui::SliderFloat("Gradient drift##bbl",    &p.gradient_drift,  -1.f,  1.f,  "%.3f");
        ImGui::SliderFloat("Epsilon##bbl",           &p.epsilon,         0.01f, 0.5f, "%.3f");
        ImGui::SliderFloat("Arrival radius##bbl",    &p.arrival_radius,  0.1f,  2.f,  "%.2f");
        if (p.sigma > 1e-5f)
            ImGui::TextDisabled("Peclet = %.2f  (drift/sigma^2)",
                p.drift_strength / (p.sigma * p.sigma));
    }

    if (ImGui::SmallButton(!m_spawn.spawning_pursuer ? "Spawn leader [Ctrl+A]" : "Spawn pursuer [Ctrl+A]")) {
        if (!m_spawn.spawning_pursuer) spawn_leader_seeker();
        else                     spawn_pursuit_particle();
    }

    // Pursuit mode
    ImGui::SeparatorText("Pursuit mode");
    {
        int mode = static_cast<int>(m_pursuit.pursuit_mode);
        bool changed = false;
        changed |= ImGui::RadioButton("Direct##pm",   &mode, 0);
        ImGui::SameLine();
        changed |= ImGui::RadioButton("Delayed##pm",  &mode, 1);
        ImGui::SameLine();
        changed |= ImGui::RadioButton("Momentum##pm", &mode, 2);
        if (changed) m_pursuit.pursuit_mode = static_cast<PursuitMode>(mode);
    }
    if (m_pursuit.pursuit_mode == PursuitMode::Delayed)
        ImGui::SliderFloat("Tau##pm",    &m_pursuit.pursuit_tau,    0.1f, 10.f, "%.2f s");
    if (m_pursuit.pursuit_mode == PursuitMode::Momentum)
        ImGui::SliderFloat("Window##pm", &m_pursuit.pursuit_window, 0.1f,  5.f, "%.2f s");

    // Extremum table readout
    ImGui::SeparatorText("Extremum table");
    if (m_pursuit.extremum_table.valid) {
        ImGui::TextDisabled("max  u=%.3f v=%.3f  z=%.3f",
            m_pursuit.extremum_table.max_uv.x, m_pursuit.extremum_table.max_uv.y, m_pursuit.extremum_table.max_z);
        ImGui::TextDisabled("min  u=%.3f v=%.3f  z=%.3f",
            m_pursuit.extremum_table.min_uv.x, m_pursuit.extremum_table.min_uv.y, m_pursuit.extremum_table.min_z);
        if (ImGui::SmallButton("Rebuild now##ext"))
            m_pursuit.extremum_table.build(*m_surface, m_sim_time);
    } else {
        ImGui::TextColored({1.f,0.6f,0.1f,1.f}, "Table invalid -- switch to Extremum surface");
    }
}

// ── export_session ────────────────────────────────────────────────────────────

void SurfaceSimScene::export_session(const std::string& path) const {
    std::ofstream f(path);
    if (!f.is_open()) return;

    f << std::fixed << std::setprecision(6);
    f << "particle_id,role,equation,record_type,step,a,b,c\n";

    for (u32 ci = 0; ci < static_cast<u32>(m_curves.size()); ++ci) {
        const auto& c = m_curves[ci];
        const std::string role = (c.role() == AnimatedCurve::Role::Leader) ? "leader" : "chaser";
        const std::string eq   = c.equation() ? c.equation()->name() : "unknown";

        for (u32 i = 0; i < c.trail_size(); ++i) {
            const Vec3 p = c.trail_pt(i);
            f << ci << ',' << role << ',' << eq << ",trail,"
              << i << ',' << p.x << ',' << p.y << ',' << p.z << '\n';
        }
        if (c.history()) {
            const auto recs = c.history()->to_vector();
            for (std::size_t i = 0; i < recs.size(); ++i) {
                f << ci << ',' << role << ',' << eq << ",history,"
                  << i << ',' << recs[i].t << ','
                  << recs[i].uv.x << ',' << recs[i].uv.y << '\n';
            }
        }
    }
}

} // namespace ndde

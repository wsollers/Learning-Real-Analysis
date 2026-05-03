#include "app/SurfaceSimController.hpp"

#include "app/GaussianRipple.hpp"
#include "app/GaussianSurface.hpp"
#include "math/ExtremumSurface.hpp"
#include "sim/MinDistConstraint.hpp"

#include <algorithm>
#include <cmath>

namespace ndde {

SurfaceSimController::SurfaceSimController(std::unique_ptr<ndde::math::ISurface>& surface,
                                           ParticleSystem& particles,
                                           std::vector<AnimatedCurve>& curves,
                                           SurfaceSelectionState& surface_state,
                                           SurfaceDisplayState& display,
                                           SurfacePursuitState& pursuit,
                                           SurfaceSpawnState& spawn,
                                           SurfaceSimSpawner& spawner,
                                           bool& paused,
                                           float& sim_time,
                                           float& sim_speed) noexcept
    : m_surface(surface)
    , m_particles(particles)
    , m_curves(curves)
    , m_surface_state(surface_state)
    , m_display(display)
    , m_pursuit(pursuit)
    , m_spawn(spawn)
    , m_spawner(spawner)
    , m_paused(paused)
    , m_sim_time(sim_time)
    , m_sim_speed(sim_speed)
{}

void SurfaceSimController::swap_surface(SurfaceType type) {
    switch (type) {
        case SurfaceType::Gaussian:
            m_surface = std::make_unique<GaussianSurface>();
            break;
        case SurfaceType::Torus:
            m_surface = std::make_unique<ndde::math::Torus>(m_surface_state.torus_R, m_surface_state.torus_r);
            break;
        case SurfaceType::GaussianRipple:
            m_surface = std::make_unique<GaussianRipple>(m_surface_state.ripple_params);
            break;
        case SurfaceType::Extremum:
            m_surface = std::make_unique<ndde::math::ExtremumSurface>();
            m_pursuit.extremum_table.build(*m_surface);
            break;
    }

    m_surface_state.type = type;
    m_particles.set_surface(m_surface.get());
    m_sim_time = 0.f;
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
        m_spawner.spawn_gradient_particle(roles[i], {u_lo + su*(u_hi-u_lo), v_lo + sv*(v_hi-v_lo)});
        m_spawner.prewarm_particle(m_curves.back(), 60);
    }
}

void SurfaceSimController::advance_simulation(f32 dt) {
    if (!m_paused) {
        if (auto* def = dynamic_cast<ndde::math::IDeformableSurface*>(m_surface.get()))
            def->advance(dt * m_sim_speed);
        m_sim_time += dt * m_sim_speed;
        rebuild_extremum_table_if_needed();
        m_particles.update(dt, m_sim_speed, m_sim_time);
    }
}

void SurfaceSimController::sync_pairwise_constraints() {
    m_particles.clear_pair_constraints();
    if (m_pursuit.pair_collision)
        m_particles.add_pair_constraint<ndde::sim::MinDistConstraint>(m_pursuit.min_dist);
}

void SurfaceSimController::rebuild_extremum_table_if_needed() {
    if (m_surface_state.type != SurfaceType::Extremum) return;
    if (m_pursuit.extremum_rebuild_countdown == 0) {
        m_pursuit.extremum_table.build(*m_surface, m_sim_time);
        m_pursuit.extremum_rebuild_countdown = 30u;
    } else {
        --m_pursuit.extremum_rebuild_countdown;
    }
}

} // namespace ndde

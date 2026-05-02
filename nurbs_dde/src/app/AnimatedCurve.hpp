#pragma once
// app/AnimatedCurve.hpp
// AnimatedCurve: one walker particle on a parametric surface.
//
// Moved from GaussianSurface.hpp (B1 refactor) so that ParticleRenderer and
// SpawnStrategy can include this type without pulling in GaussianSurface.
//
// Ownership model
// ───────────────
// Standard construction: surface, equation, and integrator pointers are all
// non-owning.  All three must outlive the AnimatedCurve.
//
// with_equation() factory: the particle OWNS its equation via m_owned_equation
// (unique_ptr).  m_equation is an alias to m_owned_equation.get().  This
// enables per-particle SDE equations (BrownianMotion, DelayPursuitEquation).
//
// Move semantics: AnimatedCurve is moveable but not copyable.  The unique_ptr
// members (m_owned_equation, m_history) move safely; m_equation (raw pointer
// alias) remains valid because unique_ptr move preserves the heap address.
//
// History recording (for delay-pursuit DDE)
// ─────────────────────────────────────────
// enable_history() allocates a ring buffer.  push_history() records the
// current parameter-space position after each advance() call.  query_history()
// interpolates to an arbitrary past time.  The buffer pointer is stable across
// vector reallocations (unique_ptr move does not change heap address).
//
// Live equation access
// ─────────────────────
// equation() returns a non-owning pointer to the active IEquation (owned or
// shared).  Callers may dynamic_cast to a concrete type to mutate parameters
// at runtime without respawning the particle:
//   if (auto* bm = dynamic_cast<ndde::sim::BrownianMotion*>(c.equation()))
//       bm->params().sigma = 0.2f;
// This is the mechanism used by the live Brownian tuning sliders in the UI.

#include "math/Scalars.hpp"
#include "math/Surfaces.hpp"
#include "sim/IEquation.hpp"
#include "sim/IIntegrator.hpp"
#include "sim/IConstraint.hpp"
#include "sim/HistoryBuffer.hpp"
#include "app/FrenetFrame.hpp"
#include "renderer/GpuTypes.hpp"
#include <glm/glm.hpp>
#include <vector>
#include <span>
#include <memory>

namespace ndde {

class AnimatedCurve {
public:
    enum class Role : u8 { Leader, Chaser };

    static constexpr u32 MAX_TRAIL   = 1200;
    static constexpr f32 WALK_SPEED  = 0.9f;
    static constexpr u32 MAX_SLOTS   = 4u;

    // Standard constructor: all three pointers are non-owning.
    // surface, equation, integrator must outlive this AnimatedCurve.
    explicit AnimatedCurve(f32 start_x, f32 start_y,
                           Role role,
                           u32 colour_slot,
                           const ndde::math::ISurface*   surface,
                           ndde::sim::IEquation*          equation,
                           const ndde::sim::IIntegrator*  integrator);

    // Factory: construct a particle that OWNS its equation.
    // The unique_ptr is moved into m_owned_equation; m_equation is set to
    // m_owned_equation.get().  All other pointers remain non-owning.
    static AnimatedCurve with_equation(
        f32 start_x, f32 start_y,
        Role role, u32 colour_slot,
        const ndde::math::ISurface*          surface,
        std::unique_ptr<ndde::sim::IEquation> owned_equation,
        const ndde::sim::IIntegrator*         integrator);

    // AnimatedCurve is moveable (unique_ptr members) but not copyable.
    AnimatedCurve(const AnimatedCurve&)            = delete;
    AnimatedCurve& operator=(const AnimatedCurve&) = delete;
    AnimatedCurve(AnimatedCurve&&)                 = default;
    AnimatedCurve& operator=(AnimatedCurve&&)      = default;

    void reset();
    void advance(f32 dt, f32 speed_scale = 1.f);

    [[nodiscard]] FrenetFrame frenet_at(u32 idx) const noexcept;

    [[nodiscard]] u32  trail_vertex_count() const noexcept;
    void tessellate_trail(std::span<Vertex> out) const;
    [[nodiscard]] Vec3 head_world() const noexcept;

    // head_uv: parameter-space position of the particle head.
    // Used by SpawnStrategy, DirectPursuitEquation, and export_session.
    // Note: this is the navigation coordinate -- arrival is detected by
    // neighbourhood radius, not exact equality.
    [[nodiscard]] glm::vec2 head_uv() const noexcept { return m_walk.uv; }

    [[nodiscard]] u32  trail_size()    const noexcept { return static_cast<u32>(m_trail.size()); }
    [[nodiscard]] bool has_trail()     const noexcept { return m_trail.size() >= 4; }
    [[nodiscard]] Vec3 trail_pt(u32 i) const noexcept { return m_trail[i]; }
    [[nodiscard]] Role role()          const noexcept { return m_role; }
    [[nodiscard]] u32  colour_slot()   const noexcept { return m_colour_slot; }

    // ── History recording (delay-pursuit DDE) ─────────────────────────────────
    void enable_history(std::size_t capacity = 4096, float dt_min = 1.f/120.f);
    void push_history(float t);
    [[nodiscard]] glm::vec2 query_history(float t_past) const;

    // Non-owning access to the history buffer (null if not enabled).
    // The pointer is stable across vector reallocations.
    [[nodiscard]] const ndde::sim::HistoryBuffer* history() const noexcept {
        return m_history.get();
    }

    // Non-owning access to the equation (owned or shared).
    // dynamic_cast to a concrete type to mutate params live, e.g.:
    //   dynamic_cast<ndde::sim::BrownianMotion*>(c.equation())
    [[nodiscard]] ndde::sim::IEquation*       equation()       noexcept { return m_equation; }
    [[nodiscard]] const ndde::sim::IEquation* equation() const noexcept { return m_equation; }

    // Add a constraint applied after every integration sub-step.
    // AnimatedCurve owns the constraint via unique_ptr.
    // Constraints are applied in insertion order.
    void add_constraint(std::unique_ptr<ndde::sim::IConstraint> c) {
        m_constraints.push_back(std::move(c));
    }

    // Mutable access to the particle state for pairwise constraint application.
    // Called only by SurfaceSimScene::apply_pairwise_constraints().
    // Do not use from equation or integrator code.
    [[nodiscard]] ndde::sim::ParticleState& walk_state() noexcept { return m_walk; }

    [[nodiscard]] static Vec4 trail_colour(Role role, u32 slot, f32 age_t) noexcept;

    [[nodiscard]] Vec4 head_colour() const noexcept {
        return trail_colour(m_role, m_colour_slot, 1.f);
    }

private:
    ndde::sim::ParticleState                m_walk;
    std::vector<Vec3>                        m_trail;
    const ndde::math::ISurface*              m_surface;        // non-owning, never null
    ndde::sim::IEquation*                    m_equation;       // non-owning OR alias to m_owned_equation
    std::unique_ptr<ndde::sim::IEquation>    m_owned_equation; // null when using shared equation
    const ndde::sim::IIntegrator*            m_integrator;     // non-owning, never null
    std::unique_ptr<ndde::sim::HistoryBuffer> m_history;       // null unless enable_history() called
    std::vector<std::unique_ptr<ndde::sim::IConstraint>> m_constraints; // applied after each sub-step
    Role                                     m_role;
    u32                                      m_colour_slot;
    f32                                      m_start_x, m_start_y;

    void step(f32 dt, f32 speed_scale);
};

} // namespace ndde

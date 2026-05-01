#pragma once
// app/GaussianRipple.hpp
// GaussianRipple: the Option-C Gaussian surface with a decaying radial wave.
// Inherits IDeformableSurface -- is_time_varying() returns true.
//
// Geometry:
//   p(u,v,t) = (u, v, f_base(u,v) + f_ripple(u,v,t))
//
//   f_base    = GaussianSurface::eval_static(u,v)   (6-Gaussian + sinusoid)
//
//   f_ripple(u,v,t) = A * exp(-d*t) * sin(2*pi*r/L - s*t) * exp(-r^2/(2*sigma^2))
//     r      = ||(u,v) - epicentre||   (radial distance from impact point)
//     A      = amplitude               (initial wave height, world units)
//     d      = damping rate            (1/second, exponential decay)
//     L      = wavelength              (spatial period, world units)
//     s      = wave speed              (world units/second)
//     sigma  = envelope width          (Gaussian spatial falloff, world units)
//
// Physical intuition:
//   A real 2D outward wave from a point source decays as r^{-1/2}.
//   We use a Gaussian envelope instead for cleaner finite-domain behaviour --
//   the wave stays localised and does not pile up at the boundary.
//   The wavefront moves outward at speed s; amplitude decays at rate d.
//   At t >> 1/d the surface relaxes back to the static Gaussian.
//
// Differential geometry:
//   Since p is a graph surface (u,v -> z), partial derivatives are:
//     du(u,v,t) = (1, 0, dh/du)
//     dv(u,v,t) = (0, 1, dh/dv)
//   computed by central finite differences on height(u,v,t).
//   The unit normal tilts away from vertical at wavefronts.
//   Gaussian curvature is highest where the height changes most rapidly --
//   the leading and trailing edges of each ring.
//
// Usage:
//   auto* ripple = new GaussianRipple();
//   ripple->set_epicentre(u0, v0);   // place impact at (u0,v0)
//   // each frame:
//   ripple->advance(dt);             // tick internal clock
//   surface.tessellate_wireframe(out, uN, vN, ripple->time());

#include "math/Surfaces.hpp"       // IDeformableSurface
#include "app/GaussianSurface.hpp" // eval_static, XMIN/XMAX/YMIN/YMAX
#include <cmath>

namespace ndde {

class GaussianRipple final : public ndde::math::IDeformableSurface {
public:
    // All wave parameters exposed for live UI tuning.
    struct Params {
        float amplitude  = 0.5f;    ///< initial wave height (world units)
        float damping    = 0.35f;   ///< exponential decay rate (1/second)
        float wavelength = 1.8f;    ///< spatial wavelength (world units)
        float speed      = 2.2f;    ///< propagation speed (world units/second)
        float sigma      = 2.8f;    ///< Gaussian envelope sigma (world units)
        float epicentre_u = 0.f;    ///< u-coordinate of impact point
        float epicentre_v = 0.f;    ///< v-coordinate of impact point
    };

    explicit GaussianRipple(Params p = {}) : m_p(p) {}

    // ── ISurface domain (identical to GaussianSurface) ────────────────────────
    [[nodiscard]] f32 u_min(float = 0.f) const override { return GaussianSurface::XMIN; }
    [[nodiscard]] f32 u_max(float = 0.f) const override { return GaussianSurface::XMAX; }
    [[nodiscard]] f32 v_min(float = 0.f) const override { return GaussianSurface::YMIN; }
    [[nodiscard]] f32 v_max(float = 0.f) const override { return GaussianSurface::YMAX; }

    // Non-periodic: particles reflect at the domain boundary.
    [[nodiscard]] bool is_periodic_u() const override { return false; }
    [[nodiscard]] bool is_periodic_v() const override { return false; }

    // ── Geometry ──────────────────────────────────────────────────────────────

    [[nodiscard]] Vec3 evaluate(float u, float v, float t = 0.f) const override {
        return Vec3{ u, v, height(u, v, t) };
    }

    // Central FD on height -- exact same structure as GaussianSurface::du/dv
    [[nodiscard]] Vec3 du(float u, float v, float t = 0.f) const override {
        constexpr float h = 1e-3f;
        return Vec3{ 1.f, 0.f, (height(u+h,v,t) - height(u-h,v,t)) / (2.f*h) };
    }
    [[nodiscard]] Vec3 dv(float u, float v, float t = 0.f) const override {
        constexpr float h = 1e-3f;
        return Vec3{ 0.f, 1.f, (height(u,v+h,t) - height(u,v-h,t)) / (2.f*h) };
    }

    // ── Deformation control ───────────────────────────────────────────────────

    // Place the wave epicentre and restart the clock.
    void set_epicentre(float u0, float v0) noexcept {
        m_p.epicentre_u = u0;
        m_p.epicentre_v = v0;
        m_time = 0.f;
    }

    // restart without moving epicentre
    void reset() override { m_time = 0.f; }

    Params&       params()       noexcept { return m_p; }
    const Params& params() const noexcept { return m_p; }

private:
    Params m_p;

    // f_base(u,v) + f_ripple(u,v,t)
    [[nodiscard]] float height(float u, float v, float t) const noexcept {
        const float z_base = GaussianSurface::eval_static(u, v);

        const float du_  = u - m_p.epicentre_u;
        const float dv_  = v - m_p.epicentre_v;
        const float r    = std::sqrt(du_*du_ + dv_*dv_);

        // Decaying radial wave with Gaussian spatial envelope
        const float wave =
            m_p.amplitude
            * std::exp(-m_p.damping * t)
            * std::sin(2.f * 3.14159265f * r / m_p.wavelength - m_p.speed * t)
            * std::exp(-0.5f * r * r / (m_p.sigma * m_p.sigma));

        return z_base + wave;
    }
};

} // namespace ndde

#pragma once
// app/AnalysisPanel.hpp

#include "engine/EngineAPI.hpp"
#include "app/HoverResult.hpp"

namespace ndde {

class AnalysisPanel {
public:
    void draw(const HoverResult& hover, EngineAPI& api);

    // ── Feature visibility ────────────────────────────────────────────────────
    bool show_epsilon_ball()    const { return m_show_epsilon_ball; }
    bool show_secant()          const { return m_show_secant; }
    bool show_tangent()         const { return m_show_tangent; }
    bool show_interval_lines()  const { return m_show_interval_lines; }
    bool show_lipschitz_cone()  const { return m_show_lipschitz; }

    // Frenet frame
    bool show_unit_tangent()    const { return m_show_T; }
    bool show_unit_normal()     const { return m_show_N; }
    bool show_unit_binormal()   const { return m_show_B; }

    // Osculating circle / sphere
    bool show_curvature_circle() const { return m_show_osc_circle; }

    // ── Quantities ────────────────────────────────────────────────────────────
    float get_epsilon_ball_radius() const { return m_epsilon; }
    float get_epsilon_interval()    const { return m_epsilon_interval; }
    float get_delta()               const { return m_delta; }
    float get_snap_px_radius()      const { return m_snap_px_radius; }

    // Arrow scale for T/N/B display (world units per unit vector)
    float get_frame_scale()         const { return m_frame_scale; }

    // ── Colours ───────────────────────────────────────────────────────────────
    const float* secant_colour()    const { return m_secant_colour;   }
    const float* tangent_colour()   const { return m_tangent_colour;  }
    const float* interval_colour()  const { return m_interval_colour; }
    const float* T_colour()         const { return m_T_colour; }
    const float* N_colour()         const { return m_N_colour; }
    const float* B_colour()         const { return m_B_colour; }
    const float* osc_colour()       const { return m_osc_colour; }

private:
    // ── Control state ─────────────────────────────────────────────────────────
    bool  m_show_epsilon_ball    = true;
    float m_epsilon              = 0.1f;
    float m_epsilon_interval     = 0.1f;
    float m_delta                = 0.1f;
    float m_snap_px_radius       = 20.f;

    bool  m_show_secant          = false;
    bool  m_show_tangent         = false;
    bool  m_show_interval_lines  = false;
    bool  m_show_lipschitz       = false;

    // Frenet frame toggles
    bool  m_show_T               = true;   ///< Unit tangent (red)
    bool  m_show_N               = true;   ///< Principal normal (green)
    bool  m_show_B               = true;   ///< Binormal (blue)
    float m_frame_scale          = 0.3f;   ///< Arrow length in world units

    // Osculating circle/sphere
    bool  m_show_osc_circle      = true;

    // Colours
    float m_secant_colour[3]     = { 1.f,  1.f,  0.f  };  // yellow
    float m_tangent_colour[3]    = { 0.3f, 1.f,  0.5f };  // mint green
    float m_interval_colour[3]   = { 0.6f, 0.4f, 1.0f };  // purple
    float m_T_colour[3]          = { 1.f,  0.25f,0.25f};  // red
    float m_N_colour[3]          = { 0.25f,1.f,  0.25f};  // green
    float m_B_colour[3]          = { 0.25f,0.5f, 1.f  };  // blue
    float m_osc_colour[3]        = { 1.f,  0.7f, 0.2f };  // orange

    void draw_control_panel(const HoverResult& hover);
    void draw_readout_panel(const HoverResult& hover, EngineAPI& api);
};

} // namespace ndde

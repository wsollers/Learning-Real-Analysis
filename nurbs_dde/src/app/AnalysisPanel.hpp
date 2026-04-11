#ifndef NURBS_DDE_ANALYSISPANEL_HPP
#define NURBS_DDE_ANALYSISPANEL_HPP

#include <imgui.h>
#include "app/HoverResult.hpp"

namespace ndde::app {

    class AnalysisPanel {
    public:
        void draw(const HoverResult& hover, ImFont* math_font);

        // Queried by Application each frame
        bool  show_epsilon_ball()   const { return m_show_epsilon_ball; }
        bool  show_secant()         const { return m_show_secant; }
        bool  show_tangent()        const { return m_show_tangent; }
        bool  show_lipschitz_cone() const { return m_show_lipschitz; }
        bool  show_interval_lines() const { return m_show_interval_lines; }

        float get_epsilon_ball_radius() const { return m_epsilon; }
        float get_epsilon_interval()    const { return m_epsilon_interval; }
        float get_delta()               const { return m_delta; }

        // Secant colour for renderer
        const float* secant_colour() const { return m_secant_colour; }

    private:
        // ── Epsilon ball ──────────────────────────────────────────────────────────
        bool  m_show_epsilon_ball = true;
        float m_epsilon           = 0.1f;   // snap radius

        // ── Epsilon / Delta (interval lines) ─────────────────────────────────────
        float m_epsilon_interval  = 0.1f;
        float m_delta             = 0.1f;

        // ── Feature toggles ───────────────────────────────────────────────────────
        bool  m_show_secant         = false;
        bool  m_show_tangent        = false;
        bool  m_show_interval_lines = false;
        bool  m_show_lipschitz      = false;

        // ── Secant colour ─────────────────────────────────────────────────────────
        float m_secant_colour[3] = {1.f, 1.f, 0.f};   // yellow default

        void draw_control_panel(const HoverResult& hover);
        void draw_readout_panel(const HoverResult& hover, ImFont* math_font);
    };

} // namespace ndde::app

#endif
#pragma once
// app/AnalysisPanel.hpp

#include <imgui.h>
#include "app/HoverResult.hpp"

namespace ndde {

class AnalysisPanel {
public:
    void draw(const HoverResult& hover, ImFont* math_font);

    bool  show_epsilon_ball()   const { return m_show_epsilon_ball; }
    bool  show_secant()         const { return m_show_secant; }
    bool  show_tangent()        const { return m_show_tangent; }
    bool  show_lipschitz_cone() const { return m_show_lipschitz; }
    bool  show_interval_lines() const { return m_show_interval_lines; }

    float get_epsilon_ball_radius() const { return m_epsilon; }
    float get_epsilon_interval()    const { return m_epsilon_interval; }
    float get_delta()               const { return m_delta; }

    // Snap threshold in screen pixels. The epsilon-ball radius controls the
    // *rendered* circle size in world space. The snap_px_radius controls how
    // close (in actual screen pixels) the cursor must be to a curve before
    // the snap fires. These are intentionally independent:
    //   - epsilon_ball = analysis concept (mathematical neighbourhood)
    //   - snap_px      = UI responsiveness (how precise you must be)
    float get_snap_px_radius() const { return m_snap_px_radius; }

    const float* secant_colour()   const { return m_secant_colour;   }
    const float* tangent_colour()  const { return m_tangent_colour;  }
    const float* interval_colour() const { return m_interval_colour; }

private:
    bool  m_show_epsilon_ball   = true;
    float m_epsilon             = 0.1f;
    float m_epsilon_interval    = 0.1f;
    float m_delta               = 0.1f;
    float m_snap_px_radius      = 20.f; // screen pixels — display-independent snap threshold
    bool  m_show_secant         = false;
    bool  m_show_tangent        = false;
    bool  m_show_interval_lines = false;
    bool  m_show_lipschitz      = false;
    float m_secant_colour[3]    = { 1.f,  1.f,  0.f  };
    float m_tangent_colour[3]   = { 0.3f, 1.f,  0.5f };
    float m_interval_colour[3]  = { 0.6f, 0.4f, 1.0f };

    void draw_control_panel(const HoverResult& hover);
    void draw_readout_panel(const HoverResult& hover, ImFont* math_font);
};

} // namespace ndde

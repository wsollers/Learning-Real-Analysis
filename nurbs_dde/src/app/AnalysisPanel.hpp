#pragma once
// app/AnalysisPanel.hpp
// ImGui panels for the epsilon/delta readout, secant/tangent toggles,
// and curve hover data. Driven by Scene::on_frame().

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

    const float* secant_colour() const { return m_secant_colour; }

private:
    bool  m_show_epsilon_ball   = true;
    float m_epsilon             = 0.1f;
    float m_epsilon_interval    = 0.1f;
    float m_delta               = 0.1f;
    bool  m_show_secant         = false;
    bool  m_show_tangent        = false;
    bool  m_show_interval_lines = false;
    bool  m_show_lipschitz      = false;
    float m_secant_colour[3]    = { 1.f, 1.f, 0.f };

    void draw_control_panel(const HoverResult& hover);
    void draw_readout_panel(const HoverResult& hover, ImFont* math_font);
};

} // namespace ndde

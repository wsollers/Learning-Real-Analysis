// app/AnalysisPanel.cpp
#include "app/AnalysisPanel.hpp"
#include <imgui.h>
#include <cmath>

namespace ndde {

void AnalysisPanel::draw(const HoverResult& hover, EngineAPI& api) {
    draw_control_panel(hover);
    draw_readout_panel(hover, api);
}

void AnalysisPanel::draw_control_panel(const HoverResult& hover) {
    ImGui::SetNextWindowBgAlpha(0.80f);
    ImGui::SetNextWindowSize(ImVec2(310.f, 0.f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2(24.f, 420.f), ImGuiCond_FirstUseEver);
    ImGui::Begin("Analysis");

    // ── Snap ─────────────────────────────────────────────────────────────────
    ImGui::SeparatorText("Snap");
    ImGui::TextDisabled("Snap threshold (screen px)");
    ImGui::SetNextItemWidth(-1.f);
    ImGui::SliderFloat("##snap_px", &m_snap_px_radius, 2.f, 60.f, "%.0f px");

    // ── Epsilon ball / sphere ─────────────────────────────────────────────────
    ImGui::SeparatorText("Epsilon Ball / Sphere");
    ImGui::Checkbox("Show##ball", &m_show_epsilon_ball);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(-1.f);
    ImGui::SliderFloat("e##radius", &m_epsilon, 0.001f, 1.0f, "%.4f");

    // ── Epsilon / Delta ───────────────────────────────────────────────────────
    ImGui::SeparatorText("Epsilon / Delta");
    ImGui::SetNextItemWidth(-1.f);
    ImGui::SliderFloat("e##interval", &m_epsilon_interval, 0.001f, 1.f, "%.4f");
    ImGui::SetNextItemWidth(-1.f);
    ImGui::SliderFloat("d##delta",    &m_delta,            0.001f, 1.f, "%.4f");

    // ── 2D features ───────────────────────────────────────────────────────────
    ImGui::SeparatorText("2D Features");

    ImGui::Checkbox("Secant line", &m_show_secant);
    if (m_show_secant) {
        ImGui::SameLine();
        ImGui::SetNextItemWidth(80.f);
        ImGui::ColorEdit3("##sec_col", m_secant_colour,
                          ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
    }

    ImGui::Checkbox("Tangent line", &m_show_tangent);
    if (m_show_tangent) {
        ImGui::SameLine();
        ImGui::SetNextItemWidth(80.f);
        ImGui::ColorEdit3("##tan_col", m_tangent_colour,
                          ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
    }

    ImGui::Checkbox("Interval lines", &m_show_interval_lines);
    if (m_show_interval_lines) {
        ImGui::SameLine();
        ImGui::SetNextItemWidth(80.f);
        ImGui::ColorEdit3("##int_col", m_interval_colour,
                          ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
    }

    ImGui::Checkbox("Lipschitz cone", &m_show_lipschitz);

    // ── Frenet-Serret frame (works in 2D and 3D) ──────────────────────────────
    ImGui::SeparatorText("Frenet\xe2\x80\x93Serret Frame");  // UTF-8 en-dash

    ImGui::Checkbox("T  unit tangent", &m_show_T);
    if (m_show_T) {
        ImGui::SameLine();
        ImGui::SetNextItemWidth(80.f);
        ImGui::ColorEdit3("##Tcol", m_T_colour,
                          ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
    }

    ImGui::Checkbox("N  principal normal", &m_show_N);
    if (m_show_N) {
        ImGui::SameLine();
        ImGui::SetNextItemWidth(80.f);
        ImGui::ColorEdit3("##Ncol", m_N_colour,
                          ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
    }

    ImGui::Checkbox("B  binormal", &m_show_B);
    if (m_show_B) {
        ImGui::SameLine();
        ImGui::SetNextItemWidth(80.f);
        ImGui::ColorEdit3("##Bcol", m_B_colour,
                          ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
    }

    ImGui::TextDisabled("Arrow scale (world units)");
    ImGui::SetNextItemWidth(-1.f);
    ImGui::SliderFloat("##fscale", &m_frame_scale, 0.01f, 2.f, "%.3f");

    // ── Osculating circle / sphere ────────────────────────────────────────────
    ImGui::SeparatorText("Osculating Circle / Sphere");
    ImGui::Checkbox("Show##osc", &m_show_osc_circle);
    if (m_show_osc_circle) {
        ImGui::SameLine();
        ImGui::SetNextItemWidth(80.f);
        ImGui::ColorEdit3("##osc_col", m_osc_colour,
                          ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
    }

    // ── Cursor ────────────────────────────────────────────────────────────────
    ImGui::SeparatorText("Cursor");
    if (hover.hit) {
        ImGui::TextColored(ImVec4(0.4f, 1.f, 0.4f, 1.f),
            "Snapped: (%.4f, %.4f, %.4f)",
            hover.world_x, hover.world_y, hover.world_z);
    } else {
        ImGui::TextDisabled("Hover over a curve to snap");
    }

    ImGui::End();
}

void AnalysisPanel::draw_readout_panel(const HoverResult& hover, EngineAPI& api) {
    ImGui::SetNextWindowBgAlpha(0.80f);
    ImGui::SetNextWindowSize(ImVec2(340.f, 0.f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2(24.f, 780.f), ImGuiCond_FirstUseEver);
    ImGui::Begin("Analysis Readout");

    // Helper: left-column label + right-column value
    auto lv = [&](const char* lbl, const char* fmt, auto... args) {
        api.push_math_font(false);
        ImGui::Text("%s", lbl);
        api.pop_math_font();
        ImGui::SameLine(160.f);
        ImGui::Text(fmt, args...);
    };

    auto lv_dis = [&](const char* lbl) {
        api.push_math_font(false);
        ImGui::Text("%s", lbl);
        api.pop_math_font();
        ImGui::SameLine(160.f);
        ImGui::TextDisabled("---");
    };

    // ── Snapped point ─────────────────────────────────────────────────────────
    ImGui::SeparatorText("Snapped Point");
    if (hover.hit) {
        lv("c  (x)",    "%.5f", hover.world_x);
        lv("f(c)  (y)", "%.5f", hover.world_y);
        if (std::abs(hover.world_z) > 1e-6f || hover.snap_t != 0.f)
            lv("z",     "%.5f", hover.world_z);
        lv("t",         "%.5f", hover.snap_t);
        lv("|p'(t)|  speed", "%.5f", hover.speed);
    } else {
        lv_dis("c  (x)");
        lv_dis("f(c)  (y)");
    }

    // ── Epsilon / Delta ───────────────────────────────────────────────────────
    ImGui::SeparatorText("Epsilon / Delta");
    lv("e (output)", "%.4f", m_epsilon_interval);
    lv("d (input)",  "%.4f", m_delta);

    // ── Secant ───────────────────────────────────────────────────────────────
    ImGui::SeparatorText("Secant");
    if (hover.hit && hover.has_secant) {
        lv("P0", "(%.4f, %.4f)", hover.secant_x0, hover.secant_y0);
        lv("P1", "(%.4f, %.4f)", hover.secant_x1, hover.secant_y1);
        lv("dy/dx", "%.6f", hover.slope);
    } else {
        ImGui::TextDisabled("---");
    }

    // ── Tangent line ─────────────────────────────────────────────────────────
    ImGui::SeparatorText("Tangent");
    if (hover.hit && hover.has_tangent) {
        lv("Slope", "%.6f", hover.tangent_slope);
    } else {
        ImGui::TextDisabled("---");
    }

    // ── Frenet-Serret ─────────────────────────────────────────────────────────
    ImGui::SeparatorText("Frenet\xe2\x80\x93Serret");
    if (hover.hit && hover.has_tangent) {
        lv("\xce\xba  curvature",   "%.6f", hover.kappa);
        if (hover.kappa > 1e-8f)
            lv("R = 1/\xce\xba", "%.5f", hover.osc_radius);
        else
            lv_dis("R = 1/\xce\xba");
        lv("\xcf\x84  torsion",    "%.6f", hover.tau);
        lv("|p'|  speed",          "%.5f", hover.speed);

        ImGui::Spacing();
        ImGui::TextColored(ImVec4(1.f,0.35f,0.35f,1.f),
            "T  (%.3f, %.3f, %.3f)", hover.T[0], hover.T[1], hover.T[2]);
        ImGui::TextColored(ImVec4(0.35f,1.f,0.35f,1.f),
            "N  (%.3f, %.3f, %.3f)", hover.N[0], hover.N[1], hover.N[2]);
        ImGui::TextColored(ImVec4(0.35f,0.6f,1.f,1.f),
            "B  (%.3f, %.3f, %.3f)", hover.B[0], hover.B[1], hover.B[2]);
    } else {
        ImGui::TextDisabled("---");
    }

    ImGui::End();
}

} // namespace ndde

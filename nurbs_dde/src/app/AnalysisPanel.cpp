// app/AnalysisPanel.cpp
#include "app/AnalysisPanel.hpp"
#include <imgui.h>

namespace ndde {

void AnalysisPanel::draw(const HoverResult& hover, ImFont* math_font) {
    draw_control_panel(hover);
    draw_readout_panel(hover, math_font);
}

void AnalysisPanel::draw_control_panel(const HoverResult& hover) {
    ImGui::SetNextWindowBgAlpha(0.80f);
    ImGui::SetNextWindowSize(ImVec2(290.f, 0.f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2(24.f, 420.f), ImGuiCond_FirstUseEver);
    ImGui::Begin("Analysis");

    // ── Snap ──────────────────────────────────────────────────────────────────
    // Snap threshold is in screen pixels — stays constant regardless of zoom
    // or display resolution. This is intentionally separate from the epsilon
    // ball radius, which is a mathematical world-space quantity.
    ImGui::SeparatorText("Snap");
    ImGui::TextDisabled("Snap threshold (screen px)");
    ImGui::SetNextItemWidth(-1.f);
    ImGui::SliderFloat("##snap_px", &m_snap_px_radius, 2.f, 60.f, "%.0f px");
    ImGui::TextDisabled("Must be within this many pixels of a curve");

    // ── Epsilon ball ──────────────────────────────────────────────────────────
    ImGui::SeparatorText("Epsilon Ball (world units)");
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

    // ── Feature toggles ───────────────────────────────────────────────────────
    ImGui::SeparatorText("Features");

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

    // ── Cursor ────────────────────────────────────────────────────────────────
    ImGui::SeparatorText("Cursor");
    if (hover.hit) {
        ImGui::TextColored(ImVec4(0.4f, 1.f, 0.4f, 1.f),
                           "Snapped: (%.4f, %.4f)", hover.world_x, hover.world_y);
    } else {
        ImGui::TextDisabled("Hover over a curve to snap");
    }

    ImGui::End();
}

void AnalysisPanel::draw_readout_panel(const HoverResult& hover, ImFont* math_font) {
    ImGui::SetNextWindowBgAlpha(0.80f);
    ImGui::SetNextWindowSize(ImVec2(320.f, 0.f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2(24.f, 720.f), ImGuiCond_FirstUseEver);
    ImGui::Begin("Analysis Readout");

    auto label_val = [&](const char* lbl, const char* fmt, float val) {
        if (math_font) ImGui::PushFont(math_font);
        ImGui::Text("%s", lbl);
        if (math_font) ImGui::PopFont();
        ImGui::SameLine(140.f);
        ImGui::Text(fmt, val);
    };

    ImGui::SeparatorText("Snapped Point");
    if (math_font) ImGui::PushFont(math_font);
    ImGui::Text("c");
    if (math_font) ImGui::PopFont();
    ImGui::SameLine(140.f);
    if (hover.hit) ImGui::Text("%.4f", hover.world_x);
    else           ImGui::TextDisabled("---");

    if (math_font) ImGui::PushFont(math_font);
    ImGui::Text("f(c)");
    if (math_font) ImGui::PopFont();
    ImGui::SameLine(140.f);
    if (hover.hit) ImGui::Text("%.4f", hover.world_y);
    else           ImGui::TextDisabled("---");

    ImGui::SeparatorText("Epsilon / Delta");
    label_val("e (output)", "%.4f", m_epsilon_interval);
    label_val("d (input)",  "%.4f", m_delta);

    ImGui::SeparatorText("Secant");
    if (hover.hit && hover.has_secant) {
        if (math_font) ImGui::PushFont(math_font);
        ImGui::Text("P0"); if (math_font) ImGui::PopFont();
        ImGui::SameLine(140.f);
        ImGui::Text("(%.4f, %.4f)", hover.secant_x0, hover.secant_y0);

        if (math_font) ImGui::PushFont(math_font);
        ImGui::Text("P1"); if (math_font) ImGui::PopFont();
        ImGui::SameLine(140.f);
        ImGui::Text("(%.4f, %.4f)", hover.secant_x1, hover.secant_y1);

        label_val("dy/dx", "%.6f", hover.slope);
    } else {
        ImGui::TextDisabled("---");
    }

    ImGui::SeparatorText("Tangent");
    if (hover.hit && hover.has_tangent)
        label_val("Slope", "%.6f", hover.tangent_slope);
    else
        ImGui::TextDisabled("---");

    ImGui::End();
}

} // namespace ndde

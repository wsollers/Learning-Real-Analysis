// app/AnalysisPanel.cpp
#include "AnalysisPanel.hpp"
#include <imgui.h>

namespace ndde::app {

void AnalysisPanel::draw(const HoverResult& hover, ImFont* math_font) {
    draw_control_panel(hover);
    draw_readout_panel(hover, math_font);
}

// ── Control panel ──────────────────────────────────────────────────────────────

void AnalysisPanel::draw_control_panel(const HoverResult& hover) {
    ImGui::SetNextWindowBgAlpha(0.80f);
    ImGui::SetNextWindowSize(ImVec2(280.f, 0.f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2(24.f, 420.f), ImGuiCond_FirstUseEver);
    ImGui::Begin("Analysis");

    // ── Epsilon ball ──────────────────────────────────────────────────────────
    ImGui::SeparatorText("Epsilon Ball");
    ImGui::Checkbox("Show##ball", &m_show_epsilon_ball);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(-1.f);
    ImGui::SliderFloat("\xce\xb5##radius", &m_epsilon, 0.01f, 0.5f, "%.3f");

    // ── Epsilon / Delta (interval lines) ─────────────────────────────────────
    ImGui::SeparatorText("Epsilon / Delta");
    ImGui::SetNextItemWidth(-1.f);
    ImGui::SliderFloat("\xce\xb5##interval", &m_epsilon_interval, 0.001f, 1.f, "%.4f");
    ImGui::SetNextItemWidth(-1.f);
    ImGui::SliderFloat("\xce\xb4##delta",    &m_delta,            0.001f, 1.f, "%.4f");

    // ── Feature toggles ───────────────────────────────────────────────────────
    ImGui::SeparatorText("Features");
    ImGui::Checkbox("Secant line",    &m_show_secant);
    if (m_show_secant) {
        ImGui::SameLine();
        ImGui::SetNextItemWidth(120.f);
        ImGui::ColorEdit3("##secant_col", m_secant_colour,
                          ImGuiColorEditFlags_NoInputs |
                          ImGuiColorEditFlags_NoLabel);
    }
    ImGui::Checkbox("Tangent line",   &m_show_tangent);
    ImGui::Checkbox("Interval lines", &m_show_interval_lines);
    ImGui::Checkbox("Lipschitz cone", &m_show_lipschitz);

    // ── Cursor status ─────────────────────────────────────────────────────────
    ImGui::SeparatorText("Cursor");
    if (hover.hit) {
        ImGui::TextColored(ImVec4(0.4f, 1.f, 0.4f, 1.f),
                           "Snapped: (%.4f, %.4f)",
                           hover.world_x, hover.world_y);
    } else {
        ImGui::TextDisabled("Hover over a curve to snap");
    }

    ImGui::End();
}

// ── Readout panel ─────────────────────────────────────────────────────────────

void AnalysisPanel::draw_readout_panel(const HoverResult& hover, ImFont* math_font) {
    ImGui::SetNextWindowBgAlpha(0.80f);
    ImGui::SetNextWindowSize(ImVec2(320.f, 0.f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2(24.f, 720.f), ImGuiCond_FirstUseEver);
    ImGui::Begin("Analysis Readout");

    // Helper: label in math font, value right-aligned
    auto row = [&](const char* label, const char* fmt, float val) {
        ImGui::PushFont(math_font);
        ImGui::Text("%s", label);
        ImGui::PopFont();
        ImGui::SameLine(140.f);
        ImGui::Text(fmt, val);
    };

    auto row2 = [&](const char* label, float x, float y) {
        ImGui::PushFont(math_font);
        ImGui::Text("%s", label);
        ImGui::PopFont();
        ImGui::SameLine(140.f);
        if (x == 0.f && y == 0.f && label[0] == '-')
            ImGui::TextDisabled("---");
        else
            ImGui::Text("(%.4f, %.4f)", x, y);
    };

    // ── Snapped point ─────────────────────────────────────────────────────────
    ImGui::SeparatorText("Snapped Point");
    if (hover.hit) {
        ImGui::PushFont(math_font); ImGui::Text("c"); ImGui::PopFont();
        ImGui::SameLine(140.f); ImGui::Text("%.4f", hover.world_x);

        ImGui::PushFont(math_font); ImGui::Text("f(c)"); ImGui::PopFont();
        ImGui::SameLine(140.f); ImGui::Text("%.4f", hover.world_y);
    } else {
        ImGui::PushFont(math_font); ImGui::Text("c"); ImGui::PopFont();
        ImGui::SameLine(140.f); ImGui::TextDisabled("---");

        ImGui::PushFont(math_font); ImGui::Text("f(c)"); ImGui::PopFont();
        ImGui::SameLine(140.f); ImGui::TextDisabled("---");
    }

    // ── Epsilon / Delta readout ───────────────────────────────────────────────
    ImGui::SeparatorText("Epsilon / Delta");
    row("\xce\xb5  (output)", "%.4f", m_epsilon_interval);
    row("\xce\xb4  (input)",  "%.4f", m_delta);

    // ── Secant ────────────────────────────────────────────────────────────────
    ImGui::SeparatorText("Secant");
    if (hover.hit && hover.has_secant) {
        ImGui::PushFont(math_font); ImGui::Text("P\xe2\x82\x80"); ImGui::PopFont();
        ImGui::SameLine(140.f);
        ImGui::Text("(%.4f, %.4f)", hover.secant_x0, hover.secant_y0);

        ImGui::PushFont(math_font); ImGui::Text("P\xe2\x82\x81"); ImGui::PopFont();
        ImGui::SameLine(140.f);
        ImGui::Text("(%.4f, %.4f)", hover.secant_x1, hover.secant_y1);

        row("\xce\x94y/\xce\x94x", "%.6f", hover.slope);
    } else {
        ImGui::PushFont(math_font); ImGui::Text("P\xe2\x82\x80"); ImGui::PopFont();
        ImGui::SameLine(140.f); ImGui::TextDisabled("---");

        ImGui::PushFont(math_font); ImGui::Text("P\xe2\x82\x81"); ImGui::PopFont();
        ImGui::SameLine(140.f); ImGui::TextDisabled("---");

        ImGui::PushFont(math_font); ImGui::Text("\xce\x94y/\xce\x94x"); ImGui::PopFont();
        ImGui::SameLine(140.f); ImGui::TextDisabled("---");
    }

    // ── Tangent ───────────────────────────────────────────────────────────────
    ImGui::SeparatorText("Tangent");
    if (hover.hit && hover.has_tangent) {
        row("Slope", "%.6f", hover.tangent_slope);
    } else {
        ImGui::PushFont(math_font); ImGui::Text("Slope"); ImGui::PopFont();
        ImGui::SameLine(140.f); ImGui::TextDisabled("---");
    }

    ImGui::End();
}

} // namespace ndde::app
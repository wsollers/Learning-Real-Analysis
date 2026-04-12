// app/Scene.cpp
#include "app/Scene.hpp"
#include <imgui.h>
#include <glm/gtc/matrix_transform.hpp>
#include <format>
#include <stdexcept>

namespace ndde {

void ConicEntry::rebuild() {
    needs_rebuild = false;
    if (type == 0) {
        conic = std::make_unique<math::Parabola>(par_a, par_b, par_c, par_tmin, par_tmax);
    } else {
        const auto axis = (hyp_axis == 0)
            ? math::HyperbolaAxis::Horizontal
            : math::HyperbolaAxis::Vertical;
        conic = std::make_unique<math::Hyperbola>(hyp_a, hyp_b, hyp_h, hyp_k, axis, hyp_range);
    }
}

Scene::Scene(EngineAPI api) : m_api(std::move(api)) {
    add_parabola();
    add_hyperbola();
}

void Scene::add_parabola() {
    ConicEntry e;
    e.name         = std::format("Parabola {}", m_conics.size());
    e.type         = 0;
    e.color        = { 1.0f, 0.8f, 0.2f, 1.0f };
    e.tessellation = m_api.config().simulation.tessellation;
    e.rebuild();
    m_conics.push_back(std::move(e));
}

void Scene::add_hyperbola() {
    ConicEntry e;
    e.name         = std::format("Hyperbola {}", m_conics.size());
    e.type         = 1;
    e.color        = { 0.4f, 0.8f, 1.0f, 1.0f };
    e.tessellation = m_api.config().simulation.tessellation;
    e.rebuild();
    m_conics.push_back(std::move(e));
}

void Scene::on_frame() {
    draw_main_panel();
    m_analysis_panel.draw(m_hover, m_api.math_font_body());

    submit_grid();
    submit_axes();
    submit_conics();
}

Mat4 Scene::ortho_mvp() const noexcept {
    const float e = m_axes_cfg.extent * 1.2f;
    return glm::ortho(-e, e, -e, e, -10.f, 10.f);
}

void Scene::submit_grid() {
    const u32 count = math::grid_vertex_count(m_axes_cfg);
    if (count == 0) return;
    auto slice = m_api.acquire(count);
    math::build_grid({ slice.vertices(), count }, m_axes_cfg);
    m_api.submit(slice, Topology::LineList, DrawMode::VertexColor, {1,1,1,1}, ortho_mvp());
}

void Scene::submit_axes() {
    const u32 count = math::axes_vertex_count(m_axes_cfg);
    auto slice = m_api.acquire(count);
    math::build_axes({ slice.vertices(), count }, m_axes_cfg);
    m_api.submit(slice, Topology::LineList, DrawMode::VertexColor, {1,1,1,1}, ortho_mvp());
}

void Scene::submit_conics() {
    const Mat4 mvp = ortho_mvp();
    for (auto& entry : m_conics) {
        if (!entry.enabled) continue;
        if (entry.needs_rebuild) entry.rebuild();
        if (!entry.conic) continue;

        if (entry.type == 1 && entry.hyp_two_branch) {
            auto* hyp   = static_cast<math::Hyperbola*>(entry.conic.get());
            const u32 n = hyp->two_branch_vertex_count(entry.tessellation);
            auto slice  = m_api.acquire(n);
            hyp->tessellate_two_branch({ slice.vertices(), n }, entry.tessellation, entry.color);
            m_api.submit(slice, Topology::LineStrip, DrawMode::VertexColor, entry.color, mvp);
        } else {
            const u32 n = entry.conic->vertex_count(entry.tessellation);
            auto slice  = m_api.acquire(n);
            entry.conic->tessellate({ slice.vertices(), n }, entry.tessellation, entry.color);
            m_api.submit(slice, Topology::LineStrip, DrawMode::VertexColor, entry.color, mvp);
        }
    }
}

void Scene::draw_main_panel() {
    ImGui::SetNextWindowPos(ImVec2(20.f, 20.f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(300.f, 0.f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowBgAlpha(0.88f);
    ImGui::Begin("Scene");

    ImGui::SeparatorText("Grid");
    ImGui::SliderFloat("Extent",     &m_axes_cfg.extent,     0.5f, 10.f, "%.1f");
    ImGui::SliderFloat("Major step", &m_axes_cfg.major_step, 0.1f, 2.f,  "%.2f");
    ImGui::SliderFloat("Minor step", &m_axes_cfg.minor_step, 0.05f, 1.f, "%.2f");
    ImGui::Checkbox("3D mode", &m_axes_cfg.is_3d);
    ImGui::Separator();

    ImGui::SeparatorText("Curves");
    for (int i = 0; i < static_cast<int>(m_conics.size()); ++i) {
        ImGui::PushID(i);
        auto& e = m_conics[i];
        ImGui::Checkbox("##en", &e.enabled);
        ImGui::SameLine();
        if (ImGui::CollapsingHeader(e.name.c_str()))
            draw_conic_panel(e, i);
        ImGui::PopID();
    }

    ImGui::Separator();
    if (ImGui::Button("+ Parabola",  ImVec2(-1.f, 0.f))) add_parabola();
    if (ImGui::Button("+ Hyperbola", ImVec2(-1.f, 0.f))) add_hyperbola();

    ImGui::End();
}

void Scene::draw_conic_panel(ConicEntry& e, int /*idx*/) {
    ImGui::Indent();

    float col[4] = { e.color.r, e.color.g, e.color.b, e.color.a };
    if (ImGui::ColorEdit4("Color##c", col))
        e.color = Vec4{ col[0], col[1], col[2], col[3] };

    int tess = static_cast<int>(e.tessellation);
    if (ImGui::SliderInt("Segments##c", &tess, 16, 1024))
        e.tessellation = static_cast<u32>(tess);

    ImGui::Separator();

    if (e.type == 0) {
        ImGui::Text("y = a*t^2 + b*t + c");
        bool d = false;
        d |= ImGui::SliderFloat("a##p",     &e.par_a,    -4.f, 4.f,  "%.3f");
        d |= ImGui::SliderFloat("b##p",     &e.par_b,    -4.f, 4.f,  "%.3f");
        d |= ImGui::SliderFloat("c##p",     &e.par_c,    -2.f, 2.f,  "%.3f");
        d |= ImGui::SliderFloat("t min##p", &e.par_tmin, -4.f, 0.f,  "%.2f");
        d |= ImGui::SliderFloat("t max##p", &e.par_tmax,  0.f, 4.f,  "%.2f");
        if (ImGui::Button("Reset##p")) {
            e.par_a = 1.f; e.par_b = 0.f; e.par_c = 0.f;
            e.par_tmin = -1.5f; e.par_tmax = 1.5f; d = true;
        }
        if (d) e.mark_dirty();
        if (e.conic)
            ImGui::TextDisabled("k(0) = %.5f", e.conic->curvature(0.f));
    } else {
        ImGui::Text("(x-h)^2/a^2 - (y-k)^2/b^2 = 1");
        bool d = false;
        d |= ImGui::SliderFloat("a##h",     &e.hyp_a,     0.1f, 4.f,  "%.2f");
        d |= ImGui::SliderFloat("b##h",     &e.hyp_b,     0.1f, 4.f,  "%.2f");
        d |= ImGui::SliderFloat("h##h",     &e.hyp_h,    -2.f,  2.f,  "%.2f");
        d |= ImGui::SliderFloat("k##h",     &e.hyp_k,    -2.f,  2.f,  "%.2f");
        d |= ImGui::SliderFloat("range##h", &e.hyp_range, 0.5f, 4.f,  "%.2f");
        const char* axis_names[] = { "Horizontal", "Vertical" };
        if (ImGui::Combo("Axis##h", &e.hyp_axis, axis_names, 2)) d = true;
        ImGui::Checkbox("Two branches##h", &e.hyp_two_branch);
        if (ImGui::Button("Reset##h")) {
            e.hyp_a = 1.f; e.hyp_b = 1.f; e.hyp_h = 0.f;
            e.hyp_k = 0.f; e.hyp_range = 2.f; d = true;
        }
        if (d) e.mark_dirty();
    }

    ImGui::Unindent();
}

} // namespace ndde

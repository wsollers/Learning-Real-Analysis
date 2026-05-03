#pragma once
// app/WavePredatorPreyScene.hpp
// Sim 4: trigonometric wave surface with predator/prey swarm recipes.

#include "engine/EngineAPI.hpp"
#include "app/HotkeyManager.hpp"
#include "app/PanelHost.hpp"
#include "app/ParticleInspectorPanel.hpp"
#include "app/ParticleRenderer.hpp"
#include "app/ProjectedParticleOverlay.hpp"
#include "app/ParticleSwarmFactory.hpp"
#include "app/SimulationSceneBase.hpp"
#include "app/SurfaceMeshCache.hpp"
#include "app/Viewport.hpp"
#include "math/GeometryTypes.hpp"
#include "math/Surfaces.hpp"
#include "numeric/ops.hpp"

#include <imgui.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cstring>
#include <memory>
#include <numbers>
#include <string_view>
#include <vector>

namespace ndde {

class WavePredatorPreySurface final : public ndde::math::ISurface {
public:
    explicit WavePredatorPreySurface(float extent = 4.f) : m_extent(extent) {}

    [[nodiscard]] Vec3 evaluate(float x, float y, float = 0.f) const override {
        return {x, y, height(x, y)};
    }

    [[nodiscard]] float height(float x, float y) const noexcept {
        return ops::sin(x) + ops::cos(y)
             + 0.5f * (ops::sin(2.f * x) + ops::cos(2.f * y));
    }

    [[nodiscard]] float u_min(float = 0.f) const override { return -m_extent; }
    [[nodiscard]] float u_max(float = 0.f) const override { return  m_extent; }
    [[nodiscard]] float v_min(float = 0.f) const override { return -m_extent; }
    [[nodiscard]] float v_max(float = 0.f) const override { return  m_extent; }
    [[nodiscard]] float extent() const noexcept { return m_extent; }

private:
    float m_extent = 4.f;
};

class WavePredatorPreyScene final : public SimulationSceneBase {
public:
    explicit WavePredatorPreyScene(EngineAPI api)
        : m_api(std::move(api))
        , m_surface(std::make_unique<WavePredatorPreySurface>(4.f))
        , m_particles(m_surface.get(), 4242u)
        , m_particle_renderer(m_api)
    {
        m_vp3d.base_extent = 5.f;
        m_vp3d.zoom = 1.f;
        m_vp3d.yaw = 0.58f;
        m_vp3d.pitch = 0.50f;

        m_hotkeys.register_action(Chord::ctrl(ImGuiKey_R), "Reset predator/prey",
            [this]{ spawn_predator_prey_showcase(); }, "Swarms");
        m_hotkeys.register_action(Chord::ctrl(ImGuiKey_B), "Brownian cloud",
            [this]{ spawn_brownian_cloud(); }, "Swarms");
        m_hotkeys.register_action(Chord::ctrl(ImGuiKey_L), "Contour band",
            [this]{ spawn_contour_band(); }, "Swarms");
        m_hotkeys.register_action(Chord::ctrl(ImGuiKey_P), "Pause / unpause",
            [this]{ m_paused = !m_paused; }, "Simulation");
        m_hotkeys.register_toggle(Chord::ctrl(ImGuiKey_F), "Hover Frenet frame",
            m_show_frenet, "Overlays");
        m_hotkeys.register_toggle(Chord::ctrl(ImGuiKey_O), "Hover osculating circle",
            m_show_osc, "Overlays");
        m_hotkeys.register_toggle(Chord::ctrl(ImGuiKey_H), "Hotkey panel",
            m_show_hotkeys, "Panels");

        register_panels();
        spawn_predator_prey_showcase();
    }

    void on_frame(f32 dt) override {
        advance_particles(m_particles, dt);
        draw_dockspace_root("##wave_predator_dock", "WavePredatorDockSpace");

        draw_canvas_3d();
        m_panels.draw_all();
        submit_contour_second_window();
        draw_hotkey_panel(m_hotkeys);
    }

    [[nodiscard]] std::string_view name() const override { return "Wave Predator-Prey"; }

    void on_key_event(int key, int action, int mods) override {
        if (action != GLFW_PRESS) return;
        const bool ctrl = (mods & GLFW_MOD_CONTROL) != 0;
        const bool shift = (mods & GLFW_MOD_SHIFT) != 0;
        if (!ctrl || shift) return;
        switch (key) {
            case GLFW_KEY_R: spawn_predator_prey_showcase(); break;
            case GLFW_KEY_B: spawn_brownian_cloud(); break;
            case GLFW_KEY_L: spawn_contour_band(); break;
            case GLFW_KEY_P: m_paused = !m_paused; break;
            case GLFW_KEY_F: m_show_frenet = !m_show_frenet; break;
            case GLFW_KEY_O: m_show_osc = !m_show_osc; break;
            case GLFW_KEY_H: m_show_hotkeys = !m_show_hotkeys; break;
            default: break;
        }
    }

private:
    EngineAPI m_api;
    std::unique_ptr<WavePredatorPreySurface> m_surface;
    ParticleSystem m_particles;
    ParticleRenderer m_particle_renderer;
    HotkeyManager m_hotkeys;
    PanelHost m_panels;
    Viewport m_vp3d;
    SurfaceMeshCache m_mesh;

    u32 m_grid_lines = 80;
    float m_color_scale = 1.05f;
    bool m_show_frenet = true;
    bool m_show_osc = true;

    void register_panels() {
        m_panels.clear();
        m_panels.add({
            .title = "Wave - Swarms",
            .default_pos = {20.f, 55.f},
            .default_size = {330.f, 455.f},
            .bg_alpha = 0.90f,
            .draw_body = [this] { draw_swarms_body(); }
        });
        m_panels.add({
            .title = "Wave - Camera",
            .default_pos = {20.f, 530.f},
            .default_size = {330.f, 150.f},
            .bg_alpha = 0.90f,
            .draw_body = [this] { draw_camera_body(); }
        });
    }

    void reset_particles() {
        m_particles.clear();
        m_particles.clear_goals();
        m_goal_status = GoalStatus::Running;
        reset_simulation_clock();
    }

    void spawn_predator_prey_showcase() {
        reset_particles();
        ParticleSwarmFactory swarms(m_particles);
        [[maybe_unused]] const SwarmBuildResult swarm = swarms.leader_pursuit(ParticleSwarmFactory::LeaderPursuitParams{
            .leader_count = 3u,
            .chaser_count = 9u,
            .center = {0.f, 0.f},
            .leader_radius = 0.85f,
            .chaser_radius = 2.85f,
            .leader_speed = 0.48f,
            .chaser_speed = 0.92f,
            .delay_seconds = 0.42f,
            .capture_radius = 0.18f,
            .leader_noise = {.sigma = 0.09f, .drift_strength = -0.04f},
            .chaser_noise = {.sigma = 0.05f, .drift_strength = 0.f},
            .leader_trail = {TrailMode::Persistent, 2200u, 0.010f},
            .chaser_trail = {TrailMode::Finite, 1400u, 0.012f},
            .add_capture_goal = true,
            .leader_label = "Prey - Delayed Avoid - Brownian",
            .chaser_label = "Predator - Delayed Seek - Brownian"
        });

        for (int i = 0; i < 100; ++i) {
            m_sim_time += 1.f / 60.f;
            m_particles.update(1.f / 60.f, 1.f, m_sim_time);
        }
    }

    void spawn_brownian_cloud() {
        ParticleSwarmFactory swarms(m_particles);
        [[maybe_unused]] const SwarmBuildResult swarm = swarms.brownian_cloud(ParticleSwarmFactory::BrownianCloudParams{
            .count = 18u,
            .center = {0.f, 0.f},
            .radius = 2.2f,
            .role = ParticleRole::Avoider,
            .brownian = {.sigma = 0.13f, .drift_strength = 0.025f},
            .trail = {TrailMode::Finite, 900u, 0.014f},
            .label = "Avoider - Brownian Cloud"
        });
    }

    void spawn_contour_band() {
        ParticleSwarmFactory swarms(m_particles);
        ndde::sim::LevelCurveWalker::Params walker;
        walker.epsilon = 0.16f;
        walker.walk_speed = 0.54f;
        walker.turn_rate = 2.7f;
        walker.tangent_floor = 0.45f;
        [[maybe_unused]] const SwarmBuildResult swarm = swarms.contour_band(ParticleSwarmFactory::ContourBandParams{
            .count = 12u,
            .center = {0.f, 0.f},
            .radius = 1.8f,
            .shared_level = true,
            .role = ParticleRole::Neutral,
            .walker = walker,
            .noise = {.sigma = 0.018f, .drift_strength = 0.f},
            .trail = {TrailMode::Persistent, 2200u, 0.010f},
            .label = "Neutral - Contour Band - Brownian"
        });
    }

    void rebuild_geometry_if_needed() {
        m_mesh.rebuild_if_needed(*m_surface, SurfaceMeshOptions{
            .grid_lines = m_grid_lines,
            .time = 0.f,
            .color_scale = m_color_scale,
            .wire_color = {0.92f, 0.96f, 1.f, 0.30f},
            .fill_color_mode = SurfaceFillColorMode::HeightCell,
            .build_contour = true
        });
    }

    [[nodiscard]] Vec3 project_point(Vec3 p) const noexcept {
        const float cy = ops::cos(m_vp3d.yaw);
        const float sy = ops::sin(m_vp3d.yaw);
        const float cp = ops::cos(m_vp3d.pitch);
        const float sp = ops::sin(m_vp3d.pitch);
        const float xr = cy * p.x - sy * p.y;
        const float yr = sy * p.x + cy * p.y;
        const float screen_y = cp * yr - sp * p.z;
        const float inv_zoom = 1.f / ops::clamp(m_vp3d.zoom, 0.05f, 20.f);
        return {xr * inv_zoom, screen_y * inv_zoom, (sp * yr + cp * p.z) * 0.01f};
    }

    [[nodiscard]] ImVec2 projected_to_canvas(Vec3 p, const ImVec2& cpos, const ImVec2& csz) const noexcept {
        const float ext = m_surface->extent();
        const float aspect = csz.x / std::max(csz.y, 1.f);
        const float half_x = aspect >= 1.f ? ext * aspect : ext;
        const float half_y = aspect >= 1.f ? ext : ext / aspect;
        const float nx = (p.x + half_x) / (2.f * half_x);
        const float ny = 1.f - ((p.y + half_y) / (2.f * half_y));
        return {cpos.x + nx * csz.x, cpos.y + ny * csz.y};
    }

    [[nodiscard]] static ImU32 to_imgui_color(Vec4 c) noexcept {
        const auto u8 = [](float v) { return static_cast<int>(ops::clamp(v, 0.f, 1.f) * 255.f + 0.5f); };
        return IM_COL32(u8(c.r), u8(c.g), u8(c.b), u8(c.a));
    }

    void draw_projected_surface(const ImVec2& cpos, const ImVec2& csz) {
        rebuild_geometry_if_needed();
        ImDrawList* dl = ImGui::GetWindowDrawList();
        dl->PushClipRect(cpos, ImVec2(cpos.x + csz.x, cpos.y + csz.y), true);
        for (u32 i = 0; i + 2 < m_mesh.fill_count(); i += 3) {
            const Vertex& a = m_mesh.fill_vertices()[i];
            const Vertex& b = m_mesh.fill_vertices()[i + 1];
            const Vertex& c = m_mesh.fill_vertices()[i + 2];
            dl->AddTriangleFilled(
                projected_to_canvas(project_point(a.pos), cpos, csz),
                projected_to_canvas(project_point(b.pos), cpos, csz),
                projected_to_canvas(project_point(c.pos), cpos, csz),
                to_imgui_color(a.color));
        }
        for (u32 i = 0; i + 1 < m_mesh.wire_count(); i += 2) {
            dl->AddLine(projected_to_canvas(project_point(m_mesh.wire_vertices()[i].pos), cpos, csz),
                        projected_to_canvas(project_point(m_mesh.wire_vertices()[i + 1].pos), cpos, csz),
                        IM_COL32(235, 245, 255, 86), 1.f);
        }
        dl->PopClipRect();
    }

    void draw_projected_particles(const ImVec2& cpos, const ImVec2& csz) {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        const bool hovered = ImGui::IsItemHovered();
        [[maybe_unused]] const ProjectedParticleOverlayResult overlay =
            draw_projected_particle_overlay(dl, m_particles.particles(), cpos, csz,
                [this, cpos, csz](Vec3 p) {
                    return projected_to_canvas(project_point(p), cpos, csz);
                },
                ProjectedParticleOverlayOptions{
                    .hover_enabled = hovered,
                    .show_frenet = m_show_frenet,
                    .show_osculating_circle = m_show_osc,
                    .frame_scale = 0.38f
                });
    }

    void draw_canvas_3d() {
        ImGui::SetNextWindowPos(ImVec2(360.f, 20.f), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(900.f, 720.f), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowBgAlpha(0.f);
        constexpr ImGuiWindowFlags flags =
            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoBackground;
        ImGui::Begin("Wave Predator-Prey 3D", nullptr, flags);
        const ImVec2 cpos = ImGui::GetCursorScreenPos();
        const ImVec2 csz = ImGui::GetContentRegionAvail();
        ImGui::InvisibleButton("wave_predator_canvas", csz,
            ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight | ImGuiButtonFlags_MouseButtonMiddle);
        if (ImGui::IsItemHovered()) {
            const ImGuiIO& io = ImGui::GetIO();
            if (ops::abs(io.MouseWheel) > 0.f)
                m_vp3d.zoom = ops::clamp(m_vp3d.zoom * (1.f + 0.12f * io.MouseWheel), 0.05f, 20.f);
            if (ImGui::IsMouseDragging(ImGuiMouseButton_Right) || ImGui::IsMouseDragging(ImGuiMouseButton_Middle))
                m_vp3d.orbit(io.MouseDelta.x, io.MouseDelta.y);
        }
        draw_projected_surface(cpos, csz);
        draw_projected_particles(cpos, csz);
        ImDrawList* dl = ImGui::GetWindowDrawList();
        dl->AddText(ImVec2(cpos.x + 8.f, cpos.y + 6.f), IM_COL32(220, 220, 220, 190),
            "Right-drag: orbit   Scroll: zoom   Ctrl+R: reset   Ctrl+B: cloud   Ctrl+L: contour band");
        if (m_paused)
            dl->AddText(ImVec2(cpos.x + 8.f, cpos.y + 26.f), IM_COL32(255, 210, 60, 240), "PAUSED  [Ctrl+P]");
        ImGui::End();
    }

    void submit_contour_second_window() {
        const Vec2 sz = m_api.viewport_size2();
        if (sz.x <= 0.f || sz.y <= 0.f) return;
        rebuild_geometry_if_needed();
        const float ext = m_surface->extent();
        const float aspect = sz.x / std::max(sz.y, 1.f);
        const Mat4 mvp = aspect >= 1.f
            ? glm::ortho(-ext * aspect, ext * aspect, -ext, ext, -1.f, 1.f)
            : glm::ortho(-ext, ext, -ext / aspect, ext / aspect, -1.f, 1.f);

        auto fill = m_api.acquire(m_mesh.contour_count());
        for (u32 i = 0; i < m_mesh.contour_count(); ++i) {
            fill.vertices()[i] = m_mesh.contour_vertices()[i];
            fill.vertices()[i].pos.z = 0.f;
        }
        m_api.submit_to(RenderTarget::Contour2D, fill, Topology::TriangleList, DrawMode::VertexColor, {1,1,1,1}, mvp);

        for (const Particle& particle : m_particles.particles()) {
            const u32 n = particle.trail_vertex_count();
            if (n < 2) continue;
            auto trail = m_api.acquire(n);
            particle.tessellate_trail({trail.vertices(), n});
            for (u32 i = 0; i < n; ++i) {
                trail.vertices()[i].pos.z = 0.f;
                trail.vertices()[i].color.a = std::max(trail.vertices()[i].color.a, 0.72f);
            }
            m_api.submit_to(RenderTarget::Contour2D, trail, Topology::LineStrip, DrawMode::VertexColor, {1,1,1,1}, mvp);
        }
    }

    void draw_swarms_body() {
        ImGui::TextDisabled("z = sin x + cos y + 0.5(sin 2x + cos 2y)");
        ImGui::Checkbox("Paused  [Ctrl+P]", &m_paused);
        ImGui::SliderFloat("Sim speed##wave", &m_sim_speed, 0.1f, 5.f, "%.2f");
        int grid = static_cast<int>(m_grid_lines);
        if (ImGui::SliderInt("Grid##wave", &grid, 12, 150)) {
            m_grid_lines = static_cast<u32>(std::max(grid, 12));
            m_mesh.mark_dirty();
        }
        ImGui::SliderFloat("Color scale##wave", &m_color_scale, 0.2f, 4.f, "%.2f");
        ImGui::Checkbox("Hover Frenet  [Ctrl+F]", &m_show_frenet);
        ImGui::Checkbox("Osculating circle  [Ctrl+O]", &m_show_osc);
        if (ImGui::Button("Reset predator/prey  [Ctrl+R]", ImVec2(-1.f, 0.f))) spawn_predator_prey_showcase();
        if (ImGui::Button("Add Brownian cloud  [Ctrl+B]", ImVec2(-1.f, 0.f))) spawn_brownian_cloud();
        if (ImGui::Button("Add contour band  [Ctrl+L]", ImVec2(-1.f, 0.f))) spawn_contour_band();
        if (ImGui::Button("Clear all", ImVec2(-1.f, 0.f))) reset_particles();
        if (m_goal_status == GoalStatus::Succeeded)
            ImGui::TextColored({0.4f, 1.f, 0.4f, 1.f}, "Predator capture reached - paused");
        ParticleInspectorPanel::draw(m_particles.particles(), ParticleInspectorOptions{
            .label = "Active particles",
            .show_level_curve_controls = true,
            .show_brownian_controls = true,
            .show_trail_controls = true
        });
    }

    void draw_camera_body() {
        ImGui::SliderFloat("Yaw##wave", &m_vp3d.yaw, -std::numbers::pi_v<float>, std::numbers::pi_v<float>, "%.2f");
        ImGui::SliderFloat("Pitch##wave", &m_vp3d.pitch, -1.5f, 1.5f, "%.2f");
        ImGui::SliderFloat("Zoom##wave", &m_vp3d.zoom, 0.1f, 8.f, "%.2f");
        if (ImGui::Button("Reset##wave")) m_vp3d.reset();
    }
};

} // namespace ndde

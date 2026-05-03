#pragma once
// app/MultiWellScene.hpp
// Sim 3: multi-well Gaussian/trigonometric landscape with centroid pursuit.

#include "engine/EngineAPI.hpp"
#include "app/HotkeyManager.hpp"
#include "app/PanelHost.hpp"
#include "app/SimulationSceneBase.hpp"
#include "app/ParticleBehaviors.hpp"
#include "app/ParticleInspectorPanel.hpp"
#include "app/ParticleRenderer.hpp"
#include "app/ParticleSystem.hpp"
#include "app/SurfaceMeshCache.hpp"
#include "app/Viewport.hpp"
#include "math/Surfaces.hpp"
#include "math/GeometryTypes.hpp"
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

class MultiWellWaveSurface final : public ndde::math::ISurface {
public:
    explicit MultiWellWaveSurface(float extent = 4.f) : m_extent(extent) {}

    [[nodiscard]] Vec3 evaluate(float x, float y, float = 0.f) const override {
        return {x, y, height(x, y)};
    }

    [[nodiscard]] float height(float x, float y) const noexcept {
        const float g0 = 1.15f * ops::exp(-((x - 1.4f) * (x - 1.4f) + (y - 0.6f) * (y - 0.6f)));
        const float g1 = -0.85f * ops::exp(-(((x + 1.2f) * (x + 1.2f)) / 0.8f
                                            + ((y + 1.0f) * (y + 1.0f)) / 1.4f));
        const float g2 = 0.65f * ops::exp(-(((x - 0.2f) * (x - 0.2f)) / 1.8f
                                           + ((y - 1.6f) * (y - 1.6f)) / 0.6f));
        const float w0 = 0.18f * ops::sin(2.f * x) * ops::cos(2.f * y);
        const float w1 = 0.08f * ops::sin(4.f * x + y) * ops::cos(3.f * y - x);
        return g0 + g1 + g2 + w0 + w1;
    }

    [[nodiscard]] float u_min(float = 0.f) const override { return -m_extent; }
    [[nodiscard]] float u_max(float = 0.f) const override { return  m_extent; }
    [[nodiscard]] float v_min(float = 0.f) const override { return -m_extent; }
    [[nodiscard]] float v_max(float = 0.f) const override { return  m_extent; }
    [[nodiscard]] float extent() const noexcept { return m_extent; }

private:
    float m_extent = 4.f;
};

class MultiWellScene final : public SimulationSceneBase {
public:
    explicit MultiWellScene(EngineAPI api)
        : m_api(std::move(api))
        , m_surface(std::make_unique<MultiWellWaveSurface>(4.f))
        , m_particles(m_surface.get())
        , m_particle_renderer(m_api)
    {
        m_vp3d.base_extent = 5.f;
        m_vp3d.zoom = 1.f;
        m_vp3d.yaw = 0.48f;
        m_vp3d.pitch = 0.55f;

        m_hotkeys.register_action(Chord::ctrl(ImGuiKey_A), "Spawn avoider",
            [this]{ spawn_avoider(); }, "Particles");
        m_hotkeys.register_action(Chord::ctrl(ImGuiKey_C), "Spawn centroid seeker",
            [this]{ spawn_centroid_seeker(); }, "Particles");
        m_hotkeys.register_action(Chord::ctrl(ImGuiKey_R), "Reset showcase",
            [this]{ spawn_showcase_service(); }, "Simulation");
        m_hotkeys.register_action(Chord::ctrl(ImGuiKey_P), "Pause / unpause",
            [this]{ m_paused = !m_paused; }, "Simulation");
        m_hotkeys.register_toggle(Chord::ctrl(ImGuiKey_H), "Hotkey panel",
            m_show_hotkeys, "Panels");

        register_panels();
        spawn_showcase_service();
    }

    void on_frame(f32 dt) override {
        advance_particles(m_particles, dt);
        draw_dockspace_root("##multiwell_dock", "MultiWellDockSpace");

        draw_canvas_3d();
        m_panels.draw_all();
        submit_contour_second_window();
        draw_hotkey_panel(m_hotkeys);
    }

    [[nodiscard]] std::string_view name() const override { return "Multi-Well Centroid"; }

    void on_key_event(int key, int action, int mods) override {
        if (action != GLFW_PRESS) return;
        const bool ctrl = (mods & GLFW_MOD_CONTROL) != 0;
        const bool shift = (mods & GLFW_MOD_SHIFT) != 0;
        if (!ctrl || shift) return;
        switch (key) {
            case GLFW_KEY_A: spawn_avoider(); break;
            case GLFW_KEY_C: spawn_centroid_seeker(); break;
            case GLFW_KEY_R: spawn_showcase_service(); break;
            case GLFW_KEY_P: m_paused = !m_paused; break;
            case GLFW_KEY_H: m_show_hotkeys = !m_show_hotkeys; break;
            default: break;
        }
    }

private:
    EngineAPI m_api;
    std::unique_ptr<MultiWellWaveSurface> m_surface;
    ParticleSystem m_particles;
    ParticleRenderer m_particle_renderer;
    HotkeyManager m_hotkeys;
    PanelHost m_panels;
    Viewport m_vp3d;

    u32 m_grid_lines = 70;
    float m_color_scale = 1.25f;
    u32 m_spawn_count = 0;

    SurfaceMeshCache m_mesh;

    void register_panels() {
        m_panels.clear();
        m_panels.add({
            .title = "Multi-Well - Particles",
            .default_pos = {20.f, 55.f},
            .default_size = {310.f, 430.f},
            .bg_alpha = 0.90f,
            .draw_body = [this] { draw_particles_body(); }
        });
        m_panels.add({
            .title = "Multi-Well - Camera",
            .default_pos = {20.f, 505.f},
            .default_size = {310.f, 150.f},
            .bg_alpha = 0.90f,
            .draw_body = [this] { draw_camera_body(); }
        });
    }

    void spawn_showcase_service() {
        m_particles.clear();
        m_particles.clear_goals();
        m_spawn_count = 0;
        reset_simulation_clock();

        spawn_avoider_at({-2.15f, -1.15f}, {0.18f, 0.05f});
        spawn_avoider_at({ 1.55f,  1.35f}, {-0.12f, 0.10f});

        for (int i = 0; i < 90; ++i) {
            m_sim_time += 1.f / 60.f;
            m_particles.update(1.f / 60.f, 1.f, m_sim_time);
        }

        spawn_centroid_seeker_at({-0.20f, 2.55f});
        m_particles.add_goal<CaptureGoal>(CaptureGoal::Params{
            .seeker_role = ParticleRole::Chaser,
            .target_role = ParticleRole::Avoider,
            .radius = 0.20f
        });
        m_goal_status = GoalStatus::Running;
    }

    void spawn_avoider() {
        const float a = static_cast<float>(m_spawn_count) * 1.73f;
        const float r = 1.3f + 0.25f * static_cast<float>(m_spawn_count % 3u);
        spawn_avoider_at({r * ops::cos(a), r * ops::sin(a)}, {0.10f * ops::cos(a + 1.f), 0.10f * ops::sin(a + 1.f)});
    }

    void spawn_avoider_at(glm::vec2 uv, glm::vec2 drift) {
        AvoidParticleBehavior::Params avoid;
        avoid.target = TargetSelector::nearest(ParticleRole::Chaser);
        avoid.speed = 0.55f;
        avoid.delay_seconds = 0.35f;

        ParticleBuilder builder = m_particles.factory().particle();
        builder
            .named("Avoider - Delayed Avoid - Brownian")
            .role(ParticleRole::Avoider)
            .at(uv)
            .trail({TrailMode::Finite, AnimatedCurve::MAX_TRAIL, 0.012f})
            .stochastic()
            .with_behavior<AvoidParticleBehavior>(avoid)
            .with_behavior<ConstantDriftBehavior>(0.35f, drift)
            .with_behavior<BrownianBehavior>(0.35f, BrownianBehavior::Params{
                .sigma = 0.10f,
                .drift_strength = 0.04f
            });
        m_particles.spawn(std::move(builder));
        ++m_spawn_count;
    }

    void spawn_centroid_seeker() {
        spawn_centroid_seeker_at({0.f, 0.f});
    }

    void spawn_centroid_seeker_at(glm::vec2 uv) {
        ParticleBuilder builder = m_particles.factory().particle();
        builder
            .named("Chaser - Centroid Seek - Brownian")
            .role(ParticleRole::Chaser)
            .at(uv)
            .trail({TrailMode::Finite, AnimatedCurve::MAX_TRAIL, 0.012f})
            .stochastic()
            .with_behavior<CentroidSeekBehavior>(CentroidSeekBehavior::Params{
                .role = ParticleRole::Avoider,
                .speed = 0.78f
            })
            .with_behavior<BrownianBehavior>(0.18f, BrownianBehavior::Params{
                .sigma = 0.055f,
                .drift_strength = 0.f
            });
        m_particles.spawn(std::move(builder));
        ++m_spawn_count;
    }

    void rebuild_geometry_if_needed() {
        m_mesh.rebuild_if_needed(*m_surface, SurfaceMeshOptions{
            .grid_lines = m_grid_lines,
            .time = 0.f,
            .color_scale = m_color_scale,
            .wire_color = {0.92f, 0.96f, 1.f, 0.32f},
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
                        IM_COL32(235, 245, 255, 88), 1.f);
        }
        for (const Particle& particle : m_particles.particles()) {
            if (!particle.has_trail()) continue;
            const ImVec2 p = projected_to_canvas(project_point(particle.head_world()), cpos, csz);
            dl->AddCircleFilled(p, 4.f, to_imgui_color(particle.head_colour()), 16);
        }
        dl->PopClipRect();
    }

    [[nodiscard]] Mat4 canvas_mvp(const ImVec2& cpos, const ImVec2& csz) const noexcept {
        const Vec2 sw = m_api.viewport_size();
        const float swx = sw.x > 0.f ? sw.x : 1.f;
        const float swy = sw.y > 0.f ? sw.y : 1.f;
        const float cw = csz.x > 0.f ? csz.x : 1.f;
        const float ch = csz.y > 0.f ? csz.y : 1.f;
        const ImVec2 vp0 = ImGui::GetMainViewport()->Pos;
        const float sx = cw / swx;
        const float sy = ch / swy;
        const float bx = 2.f * (cpos.x - vp0.x) / swx + sx - 1.f;
        const float by = -(2.f * (cpos.y - vp0.y) / swy + sy - 1.f);
        Mat4 remap(0.f);
        remap[0][0] = sx; remap[1][1] = sy; remap[2][2] = 1.f;
        remap[3][0] = bx; remap[3][1] = by; remap[3][3] = 1.f;
        const float ext = m_surface->extent();
        const float aspect = cw / ch;
        const Mat4 ortho = aspect >= 1.f
            ? glm::ortho(-ext * aspect, ext * aspect, -ext, ext, -10.f, 10.f)
            : glm::ortho(-ext, ext, -ext / aspect, ext / aspect, -10.f, 10.f);
        return remap * ortho;
    }

    void draw_canvas_3d() {
        ImGui::SetNextWindowPos(ImVec2(330.f, 20.f), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(850.f, 700.f), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowBgAlpha(0.f);
        constexpr ImGuiWindowFlags flags =
            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoBackground;
        ImGui::Begin("Multi-Well 3D", nullptr, flags);
        const ImVec2 cpos = ImGui::GetCursorScreenPos();
        const ImVec2 csz = ImGui::GetContentRegionAvail();
        ImGui::InvisibleButton("multiwell_canvas", csz,
            ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight | ImGuiButtonFlags_MouseButtonMiddle);
        if (ImGui::IsItemHovered()) {
            const ImGuiIO& io = ImGui::GetIO();
            if (ops::abs(io.MouseWheel) > 0.f)
                m_vp3d.zoom = ops::clamp(m_vp3d.zoom * (1.f + 0.12f * io.MouseWheel), 0.05f, 20.f);
            if (ImGui::IsMouseDragging(ImGuiMouseButton_Right) || ImGui::IsMouseDragging(ImGuiMouseButton_Middle))
                m_vp3d.orbit(io.MouseDelta.x, io.MouseDelta.y);
        }
        const Mat4 mvp = canvas_mvp(cpos, csz);
        draw_projected_surface(cpos, csz);
        m_particle_renderer.show_frenet = false;
        m_particle_renderer.submit_all(m_particles.particles(), *m_surface, m_sim_time, mvp, -1, -1, false);
        ImDrawList* dl = ImGui::GetWindowDrawList();
        dl->AddText(ImVec2(cpos.x + 8.f, cpos.y + 6.f), IM_COL32(220, 220, 220, 190),
            "Right-drag: orbit   Scroll: zoom   Ctrl+A: avoider   Ctrl+C: centroid seeker");
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
                trail.vertices()[i].color.a = std::max(trail.vertices()[i].color.a, 0.70f);
            }
            m_api.submit_to(RenderTarget::Contour2D, trail, Topology::LineStrip, DrawMode::VertexColor, {1,1,1,1}, mvp);
        }
    }

    void draw_particles_body() {
        ImGui::Checkbox("Paused  [Ctrl+P]", &m_paused);
        ImGui::SliderFloat("Sim speed##mw", &m_sim_speed, 0.1f, 5.f, "%.2f");
        int grid = static_cast<int>(m_grid_lines);
        if (ImGui::SliderInt("Grid##mw", &grid, 12, 140)) {
            m_grid_lines = static_cast<u32>(std::max(grid, 12));
            m_mesh.mark_dirty();
        }
        ImGui::SliderFloat("Color scale##mw", &m_color_scale, 0.2f, 4.f, "%.2f");
        if (ImGui::Button("Spawn avoider  [Ctrl+A]", ImVec2(-1.f, 0.f))) spawn_avoider();
        if (ImGui::Button("Spawn centroid seeker  [Ctrl+C]", ImVec2(-1.f, 0.f))) spawn_centroid_seeker();
        if (ImGui::Button("Reset showcase  [Ctrl+R]", ImVec2(-1.f, 0.f))) spawn_showcase_service();
        if (ImGui::Button("Clear all", ImVec2(-1.f, 0.f))) {
            m_particles.clear();
            m_particles.clear_goals();
            m_goal_status = GoalStatus::Running;
        }
        if (m_goal_status == GoalStatus::Succeeded)
            ImGui::TextColored({0.4f, 1.f, 0.4f, 1.f}, "Capture reached - paused");
        ParticleInspectorPanel::draw(m_particles.particles(), ParticleInspectorOptions{
            .label = "Active particles",
            .show_level_curve_controls = false,
            .show_brownian_controls = true,
            .show_trail_controls = true
        });
    }

    void draw_camera_body() {
        ImGui::SliderFloat("Yaw##mw", &m_vp3d.yaw, -std::numbers::pi_v<float>, std::numbers::pi_v<float>, "%.2f");
        ImGui::SliderFloat("Pitch##mw", &m_vp3d.pitch, -1.5f, 1.5f, "%.2f");
        ImGui::SliderFloat("Zoom##mw", &m_vp3d.zoom, 0.1f, 8.f, "%.2f");
        if (ImGui::Button("Reset##mw")) m_vp3d.reset();
    }

};

} // namespace ndde

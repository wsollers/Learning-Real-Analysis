#pragma once
// app/AnalysisScene.hpp
// AnalysisScene: differential geometry analysis on the SineRationalSurface.
//
// Surface
// ───────
//   f(x,y) = [3/(1+(x+y+1)²)] sin(2x) cos(2y) + 0.1 sin(5x) sin(5y)
//   Domain: [-4, 4]²
//
// This scene demonstrates level-curve tracing. Particles (LevelCurveWalker)
// spawn at a random position and lock onto the level curve f(x,y) = z₀ where
// z₀ is the height at the spawn point. An ε-band confinement term keeps them
// from drifting off the curve.
//
// Panel layout (each is a separate, independently moveable ImGui window)
// ─────────────────────────────────────────────────────────────────────
//   "Analysis – Surface"    surface geometry readout (K, H, grad, z at cursor)
//   "Analysis – Walkers"    spawn controls, per-walker params (z₀, ε, speed)
//   "Analysis – Camera"     yaw/pitch/zoom, reset
//   Engine-global panel owns simulation switching and frame stats.
//
// The 3D canvas ("Analysis 3D") fills the dockspace exactly as in
// SurfaceSimScene. The second window shows a 2D heat/contour view of the same
// height field so the level-curve walkers have a planar reference.

#include "engine/EngineAPI.hpp"
#include "app/SceneFactories.hpp"
#include "app/SimulationSceneBase.hpp"
#include "app/AnimatedCurve.hpp"
#include "app/FrenetFrame.hpp"
#include "app/ParticleRenderer.hpp"
#include "app/ParticleFactory.hpp"
#include "app/HotkeyManager.hpp"
#include "app/PanelHost.hpp"
#include "app/ParticleInspectorPanel.hpp"
#include "app/SurfaceMeshCache.hpp"
#include "app/Viewport.hpp"
#include "app/ParticleSystem.hpp"
#include "math/SineRationalSurface.hpp"
#include "sim/LevelCurveWalker.hpp"
#include "sim/BrownianMotion.hpp"
#include "sim/IConstraint.hpp"
#include "math/GeometryTypes.hpp"
#include "numeric/ops.hpp"

#include <imgui.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <memory>
#include <vector>
#include <string>
#include <random>
#include <numbers>
#include <cmath>
#include <cstring>
#include <algorithm>

namespace ndde {

class AnalysisScene final : public SimulationSceneBase {
public:
    explicit AnalysisScene(EngineAPI api)
        : m_api(std::move(api))
        , m_surface(std::make_unique<ndde::math::SineRationalSurface>(4.f))
        , m_particles(m_surface.get())
        , m_particle_renderer(m_api)
    {
        m_vp3d.base_extent = 5.f;
        m_vp3d.zoom        = 1.f;
        m_vp3d.yaw         = 0.6f;
        m_vp3d.pitch       = 0.45f;

        // Register hotkeys
        m_hotkeys.register_action(Chord::ctrl(ImGuiKey_W), "Spawn walker",
            [this]{ spawn_walker(); }, "Walkers");
        m_hotkeys.register_action(Chord::ctrl(ImGuiKey_P), "Pause / unpause",
            [this]{ m_paused = !m_paused; }, "Simulation");
        m_hotkeys.register_toggle(Chord::ctrl(ImGuiKey_H), "Hotkey panel",
            m_show_hotkeys, "Panels");

        register_panels();
        spawn_showcase_service();
    }

    // ── IScene ────────────────────────────────────────────────────────────────

    void on_frame(f32 dt) override {
        advance_particles(m_particles, dt);
        draw_dockspace_root("##analysis_dock", "AnalysisDockSpace");

        draw_canvas_3d();
        m_panels.draw_all();
        submit_contour_second_window();
        draw_hotkey_panel(m_hotkeys);
    }

    [[nodiscard]] std::string_view name() const override { return "Analysis – Sine-Rational"; }
    void on_key_event(int key, int action, int mods) override {
        if (action != GLFW_PRESS) return;
        const bool ctrl = (mods & GLFW_MOD_CONTROL) != 0;
        const bool shift = (mods & GLFW_MOD_SHIFT) != 0;
        if (!ctrl || shift) return;
        switch (key) {
            case GLFW_KEY_W: spawn_walker(); break;
            case GLFW_KEY_P: m_paused = !m_paused; break;
            case GLFW_KEY_H: m_show_hotkeys = !m_show_hotkeys; break;
            default: break;
        }
    }

private:
    // ── Core state ────────────────────────────────────────────────────────────
    EngineAPI                                       m_api;
    std::unique_ptr<ndde::math::SineRationalSurface> m_surface;
    ParticleSystem                                  m_particles;
    ParticleRenderer                                m_particle_renderer;
    HotkeyManager                                   m_hotkeys;
    PanelHost                                       m_panels;
    Viewport                                        m_vp3d;

    // Geometry cache
    SurfaceMeshCache  m_mesh;
    std::vector<Vertex> m_projected_fill_cache;
    std::vector<Vertex> m_projected_wire_cache;
    u32               m_grid_lines  = 60;
    float             m_curv_scale  = 3.f;

    // Spawn params
    float m_epsilon    = 0.15f;
    float m_walk_speed = 0.7f;
    float m_noise_sigma = 0.0f;
    u32   m_spawn_count = 0;

    // ── Spawn ─────────────────────────────────────────────────────────────────

    void register_panels() {
        m_panels.clear();
        m_panels.add({
            .title = "Analysis - Surface",
            .default_pos = {20.f, 20.f},
            .default_size = {290.f, 340.f},
            .bg_alpha = 0.88f,
            .draw_body = [this] { draw_surface_body(); }
        });
        m_panels.add({
            .title = "Analysis - Walkers",
            .default_pos = {20.f, 370.f},
            .default_size = {290.f, 380.f},
            .bg_alpha = 0.88f,
            .draw_body = [this] { draw_walkers_body(); }
        });
        m_panels.add({
            .title = "Analysis - Camera",
            .default_pos = {20.f, 760.f},
            .default_size = {290.f, 160.f},
            .bg_alpha = 0.88f,
            .draw_body = [this] { draw_camera_body(); }
        });
    }

    [[nodiscard]] static ndde::sim::LevelCurveWalker* level_walker(AnimatedCurve& c) noexcept {
        if (auto* eq = dynamic_cast<ndde::sim::LevelCurveWalker*>(c.equation()))
            return eq;
        if (auto* stack = dynamic_cast<BehaviorStack*>(c.equation()))
            return stack->find_equation<ndde::sim::LevelCurveWalker>();
        return nullptr;
    }

    [[nodiscard]] static const ndde::sim::LevelCurveWalker* level_walker(const AnimatedCurve& c) noexcept {
        if (const auto* eq = dynamic_cast<const ndde::sim::LevelCurveWalker*>(c.equation()))
            return eq;
        if (const auto* stack = dynamic_cast<const BehaviorStack*>(c.equation()))
            return stack->find_equation<ndde::sim::LevelCurveWalker>();
        return nullptr;
    }

    void spawn_showcase_service() {
        m_particles.clear();
        m_particles.clear_goals();
        m_spawn_count = 0;
        reset_simulation_clock();

        ndde::sim::LevelCurveWalker::Params p;
        p.z0 = m_surface->height(-1.25f, 0.65f);
        p.epsilon = 0.18f;
        p.walk_speed = 0.72f;
        p.tangent_floor = 0.42f;

        ParticleBuilder leader_builder = m_particles.factory().particle();
        leader_builder
            .named("Leader - Level Curve - Brownian")
            .role(ParticleRole::Leader)
            .at({-1.25f, 0.65f})
            .history(640, 1.f / 120.f)
            .trail({TrailMode::Finite, AnimatedCurve::MAX_TRAIL, 0.012f})
            .stochastic()
            .with_equation(std::make_unique<ndde::sim::LevelCurveWalker>(p))
            .with_behavior<BrownianBehavior>(0.20f, BrownianBehavior::Params{
                .sigma = 0.045f,
                .drift_strength = 0.f
            });
        AnimatedCurve& leader = m_particles.spawn(std::move(leader_builder));
        ++m_spawn_count;

        for (int i = 0; i < 180; ++i) {
            m_sim_time += 1.f / 60.f;
            m_particles.update(1.f / 60.f, 1.f, m_sim_time);
        }

        SeekParticleBehavior::Params seek;
        seek.target = TargetSelector::nearest(ParticleRole::Leader);
        seek.speed = 0.86f;
        seek.delay_seconds = 0.85f;

        const glm::vec2 leader_uv = leader.head_uv();
        ParticleBuilder seeker_builder = m_particles.factory().particle();
        seeker_builder
            .named("Chaser - Delayed Seek - Brownian")
            .role(ParticleRole::Chaser)
            .at({leader_uv.x + 1.0f, leader_uv.y - 0.75f})
            .trail({TrailMode::Finite, AnimatedCurve::MAX_TRAIL, 0.012f})
            .stochastic()
            .with_behavior<SeekParticleBehavior>(seek)
            .with_behavior<BrownianBehavior>(0.18f, BrownianBehavior::Params{
                .sigma = 0.035f,
                .drift_strength = 0.f
            });
        m_particles.spawn(std::move(seeker_builder));
        ++m_spawn_count;

        m_particles.add_goal<CaptureGoal>(CaptureGoal::Params{
            .seeker_role = ParticleRole::Chaser,
            .target_role = ParticleRole::Leader,
            .radius = 0.18f
        });
        m_goal_status = GoalStatus::Running;
    }

    void spawn_walker() {
        // Pick a random position in the domain
        std::uniform_real_distribution<float> du(
            m_surface->u_min() + 0.5f, m_surface->u_max() - 0.5f);
        std::uniform_real_distribution<float> dv(
            m_surface->v_min() + 0.5f, m_surface->v_max() - 0.5f);

        const float u0 = du(m_particles.rng());
        const float v0 = dv(m_particles.rng());
        const float z0 = m_surface->height(u0, v0);

        ndde::sim::LevelCurveWalker::Params p;
        p.z0         = z0;
        p.epsilon    = m_epsilon;
        p.walk_speed = m_walk_speed;

        ParticleBuilder builder = m_particles.factory().particle();
        builder.named("Walker")
            .role(ParticleRole::Leader)
            .at({u0, v0})
            .trail({TrailMode::Finite, AnimatedCurve::MAX_TRAIL, 0.015f})
            .with_equation(std::make_unique<ndde::sim::LevelCurveWalker>(p));

        if (m_noise_sigma > 1e-6f) {
            builder.stochastic()
                .with_behavior<BrownianBehavior>(BrownianBehavior::Params{
                    .sigma = m_noise_sigma,
                    .drift_strength = 0.f
                });
        }

        AnimatedCurve& c = m_particles.spawn(std::move(builder));

        SimulationContext context = m_particles.context(m_sim_time);
        context.set_time(m_sim_time);
        c.set_behavior_context(&context);

        // Pre-warm 120 frames so trail is visible immediately
        for (int i = 0; i < 120; ++i) {
            context.set_time(m_sim_time + static_cast<float>(i) / 60.f);
            c.advance(1.f/60.f, m_sim_speed);
        }
        ++m_spawn_count;
    }

    // ── MVP ───────────────────────────────────────────────────────────────────

    [[nodiscard]] Mat4 canvas_mvp(const ImVec2& cpos, const ImVec2& csz) const noexcept {
        const Vec2  sw   = m_api.viewport_size();
        const float swx  = sw.x > 0.f ? sw.x : 1.f;
        const float swy  = sw.y > 0.f ? sw.y : 1.f;
        const float cw   = csz.x > 0.f ? csz.x : 1.f;
        const float ch   = csz.y > 0.f ? csz.y : 1.f;

        const float sx = cw / swx;
        const float sy = ch / swy;
        const ImVec2 vp0 = ImGui::GetMainViewport()->Pos;
        const float cx = cpos.x - vp0.x;
        const float cy = cpos.y - vp0.y;
        const float bx =  2.f*cx/swx + sx - 1.f;
        const float by = -(2.f*cy/swy + sy - 1.f);

        Mat4 remap(0.f);
        remap[0][0] = sx;  remap[1][1] = sy;  remap[2][2] = 1.f;
        remap[3][0] = bx;  remap[3][1] = by;  remap[3][3] = 1.f;

        const float ext = m_surface->extent();
        const float aspect = cw / ch;
        const Mat4 ortho = aspect >= 1.f
            ? glm::ortho(-ext * aspect, ext * aspect, -ext, ext, -10.f, 10.f)
            : glm::ortho(-ext, ext, -ext / aspect, ext / aspect, -10.f, 10.f);
        return remap * ortho;
    }

    // ── Surface geometry ──────────────────────────────────────────────────────

    void rebuild_geometry_if_needed() {
        m_mesh.rebuild_if_needed(*m_surface, SurfaceMeshOptions{
            .grid_lines = m_grid_lines,
            .time = 0.f,
            .color_scale = m_curv_scale,
            .wire_color = {0.3f, 0.6f, 0.9f, 0.55f},
            .fill_color_mode = SurfaceFillColorMode::HeightCell,
            .build_contour = true
        });
    }

    void submit_contour_second_window() {
        const Vec2 sz = m_api.viewport_size2();
        if (sz.x <= 0.f || sz.y <= 0.f) return;
        rebuild_geometry_if_needed();
        if (m_mesh.contour_count() == 0) return;

        const float ext = m_surface->extent();
        const float aspect = sz.x / std::max(sz.y, 1.f);
        const Mat4 mvp = aspect >= 1.f
            ? glm::ortho(-ext * aspect, ext * aspect, -ext, ext, -1.f, 1.f)
            : glm::ortho(-ext, ext, -ext / aspect, ext / aspect, -1.f, 1.f);

        auto sl = m_api.acquire(m_mesh.contour_count());
        std::memcpy(sl.vertices(), m_mesh.contour_vertices().data(), m_mesh.contour_count()*sizeof(Vertex));
        m_api.submit_to(RenderTarget::Contour2D, sl, Topology::TriangleList,
                        DrawMode::VertexColor, {1,1,1,1}, mvp);

        if (m_mesh.wire_count() > 0) {
            auto wl = m_api.acquire(m_mesh.wire_count());
            std::memcpy(wl.vertices(), m_mesh.wire_vertices().data(), m_mesh.wire_count()*sizeof(Vertex));
            for (u32 i = 0; i < m_mesh.wire_count(); ++i) {
                wl.vertices()[i].pos.z = 0.f;
                wl.vertices()[i].color = {0.95f, 0.95f, 1.f, 0.32f};
            }
            m_api.submit_to(RenderTarget::Contour2D, wl, Topology::LineList,
                            DrawMode::VertexColor, {1,1,1,1}, mvp);
        }

        for (const auto& curve : m_particles.particles()) {
            const u32 n = curve.trail_vertex_count();
            if (n < 2) continue;

            auto tr = m_api.acquire(n);
            curve.tessellate_trail({tr.vertices(), n});
            for (u32 i = 0; i < n; ++i) {
                tr.vertices()[i].pos.z = 0.f;
                tr.vertices()[i].color.a = std::max(tr.vertices()[i].color.a, 0.65f);
            }
            m_api.submit_to(RenderTarget::Contour2D, tr, Topology::LineStrip,
                            DrawMode::VertexColor, {1,1,1,1}, mvp);

            const glm::vec2 uv = curve.head_uv();
            constexpr float r = 0.055f;
            auto dot = m_api.acquire(4);
            const Vec4 col = curve.head_colour();
            dot.vertices()[0] = {{uv.x - r, uv.y, 0.f}, col};
            dot.vertices()[1] = {{uv.x + r, uv.y, 0.f}, col};
            dot.vertices()[2] = {{uv.x, uv.y - r, 0.f}, col};
            dot.vertices()[3] = {{uv.x, uv.y + r, 0.f}, col};
            m_api.submit_to(RenderTarget::Contour2D, dot, Topology::LineList,
                            DrawMode::VertexColor, {1,1,1,1}, mvp);
        }
    }

    [[nodiscard]] Vec3 project_analysis_point(Vec3 p) const noexcept {
        const float cy = ops::cos(m_vp3d.yaw);
        const float sy = ops::sin(m_vp3d.yaw);
        const float cp = ops::cos(m_vp3d.pitch);
        const float sp = ops::sin(m_vp3d.pitch);

        const float xr = cy * p.x - sy * p.y;
        const float yr = sy * p.x + cy * p.y;
        const float zr = p.z;

        const float screen_y = cp * yr - sp * zr;
        const float depth    = sp * yr + cp * zr;
        const float inv_zoom = 1.f / std::max(m_vp3d.zoom, 0.05f);
        return {xr * inv_zoom, screen_y * inv_zoom, depth * 0.01f};
    }

    [[nodiscard]] static ImU32 to_imgui_color(Vec4 c) noexcept {
        const auto u8c = [](float v) -> int {
            return static_cast<int>(std::clamp(v, 0.f, 1.f) * 255.f + 0.5f);
        };
        return IM_COL32(u8c(c.r), u8c(c.g), u8c(c.b), u8c(c.a));
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

    void draw_projected_imgui_surface(const ImVec2& cpos, const ImVec2& csz) {
        rebuild_geometry_if_needed();
        ImDrawList* dl = ImGui::GetWindowDrawList();
        dl->PushClipRect(cpos, ImVec2(cpos.x + csz.x, cpos.y + csz.y), true);

        for (u32 i = 0; i + 2 < m_mesh.fill_count(); i += 3) {
            const Vertex& a = m_mesh.fill_vertices()[i + 0];
            const Vertex& b = m_mesh.fill_vertices()[i + 1];
            const Vertex& c = m_mesh.fill_vertices()[i + 2];
            const ImVec2 pa = projected_to_canvas(project_analysis_point(a.pos), cpos, csz);
            const ImVec2 pb = projected_to_canvas(project_analysis_point(b.pos), cpos, csz);
            const ImVec2 pc = projected_to_canvas(project_analysis_point(c.pos), cpos, csz);
            dl->AddTriangleFilled(pa, pb, pc, to_imgui_color(a.color));
        }

        const ImU32 wire_col = IM_COL32(215, 235, 255, 105);
        for (u32 i = 0; i + 1 < m_mesh.wire_count(); i += 2) {
            const ImVec2 a = projected_to_canvas(project_analysis_point(m_mesh.wire_vertices()[i + 0].pos), cpos, csz);
            const ImVec2 b = projected_to_canvas(project_analysis_point(m_mesh.wire_vertices()[i + 1].pos), cpos, csz);
            dl->AddLine(a, b, wire_col, 1.0f);
        }

        for (const auto& curve : m_particles.particles()) {
            if (!curve.has_trail()) continue;
            const Vec3 hp = curve.head_world();
            const ImVec2 p = projected_to_canvas(project_analysis_point(hp), cpos, csz);
            dl->AddCircleFilled(p, 4.f, IM_COL32(255, 230, 80, 235), 16);
        }

        dl->PopClipRect();
    }

    void submit_projected_primary(const Mat4& mvp) {
        rebuild_geometry_if_needed();
        if (m_mesh.fill_count() > 0) {
            m_projected_fill_cache.resize(m_mesh.fill_count());
            for (u32 i = 0; i < m_mesh.fill_count(); ++i) {
                m_projected_fill_cache[i] = m_mesh.fill_vertices()[i];
                m_projected_fill_cache[i].pos = project_analysis_point(m_mesh.fill_vertices()[i].pos);
            }
            auto sl = m_api.acquire(m_mesh.fill_count());
            std::memcpy(sl.vertices(), m_projected_fill_cache.data(), m_mesh.fill_count()*sizeof(Vertex));
            m_api.submit_to(RenderTarget::Primary3D, sl, Topology::TriangleList,
                            DrawMode::VertexColor, {1,1,1,1}, mvp);
        }

        if (m_mesh.wire_count() > 0) {
            m_projected_wire_cache.resize(m_mesh.wire_count());
            for (u32 i = 0; i < m_mesh.wire_count(); ++i) {
                m_projected_wire_cache[i] = m_mesh.wire_vertices()[i];
                m_projected_wire_cache[i].pos = project_analysis_point(m_mesh.wire_vertices()[i].pos);
                m_projected_wire_cache[i].color = {0.90f, 0.95f, 1.f, 0.50f};
            }
            auto sl = m_api.acquire(m_mesh.wire_count());
            std::memcpy(sl.vertices(), m_projected_wire_cache.data(), m_mesh.wire_count()*sizeof(Vertex));
            m_api.submit_to(RenderTarget::Primary3D, sl, Topology::LineList,
                            DrawMode::VertexColor, {1,1,1,1}, mvp);
        }
    }

    // ── 3D canvas window ──────────────────────────────────────────────────────

    void draw_canvas_3d() {
        ImGui::SetNextWindowPos(ImVec2(330.f, 20.f),  ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(750.f, 660.f), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowBgAlpha(0.f);
        const ImGuiWindowFlags flags =
            ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoScrollWithMouse |
            ImGuiWindowFlags_NoBackground;
        ImGui::Begin("Analysis 3D", nullptr, flags);

        const ImVec2 cpos = ImGui::GetCursorScreenPos();
        const ImVec2 csz  = ImGui::GetContentRegionAvail();
        m_vp3d.fb_w = csz.x; m_vp3d.fb_h = csz.y;
        m_vp3d.dp_w = csz.x; m_vp3d.dp_h = csz.y;

        ImGui::InvisibleButton("3d_canvas", csz,
            ImGuiButtonFlags_MouseButtonLeft |
            ImGuiButtonFlags_MouseButtonRight |
            ImGuiButtonFlags_MouseButtonMiddle);

        if (ImGui::IsItemHovered()) {
            const ImGuiIO& io = ImGui::GetIO();
            if (std::abs(io.MouseWheel) > 0.f)
                m_vp3d.zoom = std::clamp(m_vp3d.zoom*(1.f+0.12f*io.MouseWheel), 0.05f, 20.f);
            if (ImGui::IsMouseDragging(ImGuiMouseButton_Right) ||
                ImGui::IsMouseDragging(ImGuiMouseButton_Middle))
                m_vp3d.orbit(io.MouseDelta.x, io.MouseDelta.y);
        }

        const Mat4 mvp = canvas_mvp(cpos, csz);
        draw_projected_imgui_surface(cpos, csz);

        // Particles
        m_particle_renderer.show_frenet = false;
        m_particle_renderer.submit_all(m_particles.particles(), *m_surface, m_sim_time,
                                       mvp, -1, -1, false);

        // Level-band indicator lines per walker
        for (const auto& c : m_particles.particles()) {
            if (!c.has_trail()) continue;
            if (const auto* eq = level_walker(c)) {
                const float z0  = eq->params().z0;
                const float eps = eq->params().epsilon;
                // Draw small dot at head coloured by how close we are to z0
                const Vec3 hp = c.head_world();
                const Vec3 pp = project_analysis_point(hp);
                const float dz = std::abs(hp.z - z0);
                const float t  = std::clamp(dz / (eps + 1e-6f), 0.f, 1.f);
                const Vec4 col = {1.f - t, 0.3f + 0.5f*(1.f-t), t*0.8f, 1.f};
                auto sl = m_api.acquire(1);
                sl.vertices()[0] = {pp, col};
                m_api.submit_to(RenderTarget::Primary3D, sl, Topology::LineStrip, DrawMode::UniformColor, col, mvp);
            }
        }

        ImDrawList* dl = ImGui::GetWindowDrawList();
        dl->AddText(ImVec2(cpos.x+8, cpos.y+6),
            IM_COL32(200,200,200,180),
            "Right-drag: orbit   Scroll: zoom   Ctrl+W: add walker");
        dl->AddText(ImVec2(cpos.x+8, cpos.y+24),
            IM_COL32(160,210,255,180),
            "3D projected surface preview");
        if (m_paused)
            dl->AddText(ImVec2(cpos.x+8, cpos.y+42),
                IM_COL32(255,210,60,240), "PAUSED  [Ctrl+P]");

        ImGui::End();
    }

    // ── Panels ────────────────────────────────────────────────────────────────

    void draw_surface_body() {
        ImGui::SeparatorText("Function");
        ImGui::TextDisabled("f(x,y) = [3/(1+(x+y+1)\xc2\xb2)] sin(2x)cos(2y)");
        ImGui::TextDisabled("       + 0.1 sin(5x) sin(5y)");
        ImGui::Spacing();

        ImGui::SeparatorText("Geometry");
        {
            int gl = static_cast<int>(m_grid_lines);
            if (ImGui::SliderInt("Grid lines##as", &gl, 8, 120)) {
                m_grid_lines = static_cast<u32>(gl);
                m_mesh.mark_dirty();
            }
            if (ImGui::SliderFloat("Color scale##as", &m_curv_scale, 0.1f, 10.f, "%.2f"))
                m_mesh.mark_dirty();
            ImGui::TextDisabled("~%u k verts", (4u*m_grid_lines*(m_grid_lines+1u))/1000u);
        }

        ImGui::SeparatorText("At cursor (u,v)");
        // Show surface values at arbitrary position entered by user
        {
            static float probe_u = 0.f, probe_v = 0.f;
            ImGui::SetNextItemWidth(90.f);
            ImGui::InputFloat("u##probe", &probe_u, 0.1f, 0.5f, "%.2f");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(90.f);
            ImGui::InputFloat("v##probe", &probe_v, 0.1f, 0.5f, "%.2f");
            probe_u = std::clamp(probe_u, m_surface->u_min(), m_surface->u_max());
            probe_v = std::clamp(probe_v, m_surface->v_min(), m_surface->v_max());

            const float z  = m_surface->height(probe_u, probe_v);
            const float gx = m_surface->du(probe_u, probe_v).z;
            const float gy = m_surface->dv(probe_u, probe_v).z;
            const float gm = ops::sqrt(gx*gx + gy*gy);
            const float K  = m_surface->gaussian_curvature(probe_u, probe_v);
            const float H  = m_surface->mean_curvature(probe_u, probe_v);

            ImGui::TextDisabled("z     = %.4f", z);
            ImGui::TextDisabled("|grad| = %.4f", gm);
            ImGui::TextDisabled("grad  = (%.3f, %.3f)", gx, gy);
            ImGui::TextDisabled("K     = %.5f", K);
            ImGui::TextDisabled("H     = %.5f", H);
            const float perpx = -gy / (gm + 1e-9f);
            const float perpy =  gx / (gm + 1e-9f);
            ImGui::TextDisabled("level-tangent = (%.3f, %.3f)", perpx, perpy);
        }
    }

    void draw_walkers_body() {
        ImGui::SeparatorText("Spawn parameters");
        ImGui::SliderFloat("Epsilon (band)##w",  &m_epsilon,    0.02f, 1.f,  "%.3f");
        ImGui::SliderFloat("Walk speed##w",       &m_walk_speed, 0.1f,  2.f,  "%.2f");
        ImGui::SliderFloat("Noise sigma##w",      &m_noise_sigma,0.f,   0.5f, "%.3f");
        ImGui::SliderFloat("Sim speed##w",        &m_sim_speed,  0.1f,  5.f,  "%.2f");
        ImGui::Spacing();

        if (ImGui::Button("Spawn walker  [Ctrl+W]", ImVec2(-1.f, 0.f)))
            spawn_walker();
        if (ImGui::Button("Reset showcase", ImVec2(-1.f, 0.f)))
            spawn_showcase_service();
        if (ImGui::Button("Clear all", ImVec2(-1.f, 0.f))) {
            m_particles.clear();
            m_particles.clear_goals();
            m_spawn_count = 0;
            m_goal_status = GoalStatus::Running;
        }

        if (m_goal_status == GoalStatus::Succeeded)
            ImGui::TextColored({0.4f, 1.f, 0.4f, 1.f}, "Capture reached - paused");
        ParticleInspectorPanel::draw(m_particles.particles(), ParticleInspectorOptions{
            .label = "Active walkers",
            .show_level_curve_controls = true,
            .show_brownian_controls = true,
            .show_trail_controls = true
        });
    }

    void draw_camera_body() {
        ImGui::SliderFloat("Yaw##ac",   &m_vp3d.yaw,
                           -std::numbers::pi_v<float>, std::numbers::pi_v<float>, "%.2f");
        ImGui::SliderFloat("Pitch##ac", &m_vp3d.pitch, -1.5f, 1.5f, "%.2f");
        ImGui::SliderFloat("Zoom##ac",  &m_vp3d.zoom,   0.1f,  8.f, "%.2f");
        if (ImGui::Button("Reset##ac")) m_vp3d.reset();

        ImGui::SeparatorText("Pause");
        ImGui::Checkbox("Paused  [Ctrl+P]", &m_paused);
    }

};

} // namespace ndde

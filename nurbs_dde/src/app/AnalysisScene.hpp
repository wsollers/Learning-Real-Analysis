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
//   "Analysis – Debug"      PerformancePanel + CoordDebugPanel toggles
//
// The 3D canvas ("Analysis 3D") fills the dockspace exactly as in
// SurfaceSimScene, but only the surface and particle trails are drawn —
// no contour second window.

#include "engine/IScene.hpp"
#include "engine/EngineAPI.hpp"
#include "app/SurfaceSimScene.hpp"
#include "app/AnimatedCurve.hpp"
#include "app/FrenetFrame.hpp"
#include "app/ParticleRenderer.hpp"
#include "app/HotkeyManager.hpp"
#include "app/PerformancePanel.hpp"
#include "app/Viewport.hpp"
#include "math/SineRationalSurface.hpp"
#include "sim/LevelCurveWalker.hpp"
#include "sim/EulerIntegrator.hpp"
#include "sim/MilsteinIntegrator.hpp"
#include "sim/BrownianMotion.hpp"
#include "sim/IConstraint.hpp"
#include "renderer/GpuTypes.hpp"

#include <imgui.h>
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

class AnalysisScene final : public IScene {
public:
    explicit AnalysisScene(EngineAPI api)
        : m_api(std::move(api))
        , m_surface(std::make_unique<ndde::math::SineRationalSurface>(4.f))
        , m_particle_renderer(m_api)
        , m_rng(std::random_device{}())
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

        // Pre-warm with one walker
        spawn_walker();
    }

    // ── IScene ────────────────────────────────────────────────────────────────

    void on_frame(f32 dt) override {
        m_hotkeys.dispatch();

        if (!m_paused) {
            m_sim_time += dt;
            for (auto& c : m_curves)
                c.advance(dt, m_sim_speed);
        }

        // Dockspace root
        const ImGuiViewport* vp = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(vp->WorkPos);
        ImGui::SetNextWindowSize(vp->WorkSize);
        ImGui::SetNextWindowViewport(vp->ID);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding,  0.f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize,0.f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,   ImVec2(0.f, 0.f));
        constexpr ImGuiWindowFlags dock_flags =
            ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus |
            ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoBackground;
        ImGui::Begin("##analysis_dock", nullptr, dock_flags);
        ImGui::PopStyleVar(3);
        const ImGuiID dock_id = ImGui::GetID("AnalysisDockSpace");
        ImGui::DockSpace(dock_id, ImVec2(0.f, 0.f), ImGuiDockNodeFlags_None);
        ImGui::End();

        draw_canvas_3d();
        draw_panel_surface();
        draw_panel_walkers();
        draw_panel_camera();
        draw_panel_debug();
        m_hotkeys.draw_panel("Hotkeys  [Ctrl+H]", m_show_hotkeys);
        m_perf.draw(m_api.debug_stats());
    }

    [[nodiscard]] std::string_view name() const override { return "Analysis – Sine-Rational"; }

private:
    // ── Core state ────────────────────────────────────────────────────────────
    EngineAPI                                       m_api;
    std::unique_ptr<ndde::math::SineRationalSurface> m_surface;
    ndde::sim::EulerIntegrator                      m_integrator;
    ndde::sim::MilsteinIntegrator                   m_milstein;
    std::vector<AnimatedCurve>                      m_curves;
    ParticleRenderer                                m_particle_renderer;
    HotkeyManager                                   m_hotkeys;
    PerformancePanel                                m_perf;
    Viewport                                        m_vp3d;
    std::mt19937                                    m_rng;

    float m_sim_time  = 0.f;
    float m_sim_speed = 1.f;
    bool  m_paused       = false;
    bool  m_show_hotkeys = false;

    // Geometry cache
    bool              m_wire_dirty = true;
    u32               m_cached_grid = 0;
    std::vector<Vertex> m_wire_cache;
    u32               m_wire_vcount = 0;
    std::vector<Vertex> m_fill_cache;
    u32               m_fill_vcount = 0;
    u32               m_grid_lines  = 60;
    float             m_curv_scale  = 3.f;

    // Spawn params
    float m_epsilon    = 0.15f;
    float m_walk_speed = 0.7f;
    float m_noise_sigma = 0.0f;
    u32   m_spawn_count = 0;

    // ── Spawn ─────────────────────────────────────────────────────────────────

    void spawn_walker() {
        // Pick a random position in the domain
        std::uniform_real_distribution<float> du(
            m_surface->u_min() + 0.5f, m_surface->u_max() - 0.5f);
        std::uniform_real_distribution<float> dv(
            m_surface->v_min() + 0.5f, m_surface->v_max() - 0.5f);

        const float u0 = du(m_rng);
        const float v0 = dv(m_rng);
        const float z0 = m_surface->height(u0, v0);

        ndde::sim::LevelCurveWalker::Params p;
        p.z0         = z0;
        p.epsilon    = m_epsilon;
        p.walk_speed = m_walk_speed;

        auto eq = std::make_unique<ndde::sim::LevelCurveWalker>(p);

        // Optionally add Brownian noise on top via a composed approach:
        // for now the walker handles its own dynamics; noise can be layered
        // by using MilsteinIntegrator with noise_coefficient.
        const ndde::sim::IIntegrator* integrator = (m_noise_sigma > 1e-6f)
            ? static_cast<const ndde::sim::IIntegrator*>(&m_milstein)
            : static_cast<const ndde::sim::IIntegrator*>(&m_integrator);

        AnimatedCurve c = AnimatedCurve::with_equation(
            u0, v0,
            AnimatedCurve::Role::Leader,
            m_spawn_count % AnimatedCurve::MAX_SLOTS,
            m_surface.get(), std::move(eq), integrator);

        // Pre-warm 120 frames so trail is visible immediately
        for (int i = 0; i < 120; ++i)
            c.advance(1.f/60.f, m_sim_speed);

        m_curves.push_back(std::move(c));
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
        const float bx =  2.f*cpos.x/swx + sx - 1.f;
        const float by = -(2.f*cpos.y/swy + sy - 1.f);

        Mat4 remap(0.f);
        remap[0][0] = sx;  remap[1][1] = sy;  remap[2][2] = 1.f;
        remap[3][0] = bx;  remap[3][1] = by;  remap[3][3] = 1.f;

        const float dist   = m_vp3d.base_extent / m_vp3d.zoom * 3.f;
        const float ex     = dist * std::cos(m_vp3d.pitch) * std::sin(m_vp3d.yaw);
        const float ey     = dist * std::sin(m_vp3d.pitch);
        const float ez     = dist * std::cos(m_vp3d.pitch) * std::cos(m_vp3d.yaw);
        const Mat4 proj    = glm::perspective(glm::radians(45.f), cw/ch, 0.01f, 500.f);
        const Mat4 view    = glm::lookAt(glm::vec3(ex,ey,ez),
                                         glm::vec3(0.f,0.f,0.f),
                                         glm::vec3(0.f,1.f,0.f));
        return remap * proj * view;
    }

    // ── Surface geometry ──────────────────────────────────────────────────────

    static Vec4 height_color(float z, float scale) noexcept {
        const float t = std::clamp(z / (scale + 1e-9f), -1.f, 1.f);
        if (t >= 0.f)
            return {0.50f + t*0.35f, 0.50f - t*0.38f, 0.50f - t*0.42f, 0.82f};
        const float s = -t;
        return {0.50f - s*0.40f, 0.50f + s*0.18f - s*s*0.46f, 0.50f + s*0.35f, 0.82f};
    }

    void rebuild_geometry_if_needed() {
        const bool dirty = m_wire_dirty || (m_grid_lines != m_cached_grid);
        if (!dirty) return;

        // Wireframe
        const u32 wn = m_surface->wireframe_vertex_count(m_grid_lines, m_grid_lines);
        m_wire_cache.resize(wn);
        m_surface->tessellate_wireframe({m_wire_cache.data(), wn},
                                         m_grid_lines, m_grid_lines, 0.f,
                                         {0.3f, 0.6f, 0.9f, 0.55f});
        m_wire_vcount = wn;

        // Filled (curvature-coloured)
        const u32 N = m_grid_lines;
        const u32 fn = N * N * 6;
        m_fill_cache.resize(fn);
        const float u0 = m_surface->u_min(), u1 = m_surface->u_max();
        const float v0 = m_surface->v_min(), v1 = m_surface->v_max();
        const float du = (u1-u0)/static_cast<float>(N);
        const float dv = (v1-v0)/static_cast<float>(N);
        u32 idx = 0;
        for (u32 i = 0; i < N; ++i) {
            const float ua = u0 + static_cast<float>(i)*du;
            const float ub = ua + du;
            for (u32 j = 0; j < N; ++j) {
                const float va = v0 + static_cast<float>(j)*dv;
                const float vb = va + dv;
                const Vec3 p00 = m_surface->evaluate(ua,va);
                const Vec3 p10 = m_surface->evaluate(ub,va);
                const Vec3 p01 = m_surface->evaluate(ua,vb);
                const Vec3 p11 = m_surface->evaluate(ub,vb);
                const float K   = m_surface->gaussian_curvature((ua+ub)*.5f,(va+vb)*.5f);
                const Vec4  col = height_color(K, m_curv_scale);
                m_fill_cache[idx++]={p00,col}; m_fill_cache[idx++]={p10,col};
                m_fill_cache[idx++]={p11,col}; m_fill_cache[idx++]={p00,col};
                m_fill_cache[idx++]={p11,col}; m_fill_cache[idx++]={p01,col};
            }
        }
        m_fill_vcount = idx;

        m_cached_grid = m_grid_lines;
        m_wire_dirty  = false;
    }

    // ── 3D canvas window ──────────────────────────────────────────────────────

    void draw_canvas_3d() {
        ImGui::SetNextWindowPos(ImVec2(330.f, 20.f),  ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(750.f, 660.f), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowBgAlpha(0.f);
        const ImGuiWindowFlags flags =
            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
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
        rebuild_geometry_if_needed();

        // Filled surface
        if (m_fill_vcount > 0) {
            auto sl = m_api.acquire(m_fill_vcount);
            std::memcpy(sl.vertices(), m_fill_cache.data(), m_fill_vcount*sizeof(Vertex));
            m_api.submit_to(RenderTarget::Primary3D, sl, Topology::TriangleList, DrawMode::VertexColor, {1,1,1,1}, mvp);
        }
        // Wireframe overlay
        if (m_wire_vcount > 0) {
            auto sl = m_api.acquire(m_wire_vcount);
            std::memcpy(sl.vertices(), m_wire_cache.data(), m_wire_vcount*sizeof(Vertex));
            m_api.submit_to(RenderTarget::Primary3D, sl, Topology::LineList, DrawMode::VertexColor, {1,1,1,1}, mvp);
        }

        // Particles
        m_particle_renderer.show_frenet = false;
        m_particle_renderer.submit_all(m_curves, *m_surface, m_sim_time,
                                       mvp, -1, -1, false);

        // Level-band indicator lines per walker
        for (const auto& c : m_curves) {
            if (!c.has_trail()) continue;
            if (const auto* eq = dynamic_cast<const ndde::sim::LevelCurveWalker*>(c.equation())) {
                const float z0  = eq->params().z0;
                const float eps = eq->params().epsilon;
                // Draw small dot at head coloured by how close we are to z0
                const Vec3 hp = c.head_world();
                const float dz = std::abs(hp.z - z0);
                const float t  = std::clamp(dz / (eps + 1e-6f), 0.f, 1.f);
                const Vec4 col = {1.f - t, 0.3f + 0.5f*(1.f-t), t*0.8f, 1.f};
                auto sl = m_api.acquire(1);
                sl.vertices()[0] = {hp, col};
                m_api.submit_to(RenderTarget::Primary3D, sl, Topology::LineStrip, DrawMode::UniformColor, col, mvp);
            }
        }

        ImDrawList* dl = ImGui::GetWindowDrawList();
        dl->AddText(ImVec2(cpos.x+8, cpos.y+6),
            IM_COL32(200,200,200,180),
            "Right-drag: orbit   Scroll: zoom   Ctrl+W: add walker");
        if (m_paused)
            dl->AddText(ImVec2(cpos.x+8, cpos.y+22),
                IM_COL32(255,210,60,240), "PAUSED  [Ctrl+P]");

        ImGui::End();
    }

    // ── Panels ────────────────────────────────────────────────────────────────

    void draw_panel_surface() {
        ImGui::SetNextWindowPos(ImVec2(20.f, 20.f),   ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(290.f, 340.f), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowBgAlpha(0.88f);
        if (!ImGui::Begin("Analysis \xe2\x80\x93 Surface")) { ImGui::End(); return; }

        ImGui::SeparatorText("Function");
        ImGui::TextDisabled("f(x,y) = [3/(1+(x+y+1)\xc2\xb2)] sin(2x)cos(2y)");
        ImGui::TextDisabled("       + 0.1 sin(5x) sin(5y)");
        ImGui::Spacing();

        ImGui::SeparatorText("Geometry");
        {
            int gl = static_cast<int>(m_grid_lines);
            if (ImGui::SliderInt("Grid lines##as", &gl, 8, 120)) {
                m_grid_lines = static_cast<u32>(gl);
                m_wire_dirty = true;
            }
            if (ImGui::SliderFloat("K scale##as", &m_curv_scale, 0.1f, 10.f, "%.2f"))
                m_wire_dirty = true;
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
            const float gm = std::sqrt(gx*gx + gy*gy);
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

        ImGui::End();
    }

    void draw_panel_walkers() {
        ImGui::SetNextWindowPos(ImVec2(20.f, 370.f),  ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(290.f, 380.f), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowBgAlpha(0.88f);
        if (!ImGui::Begin("Analysis \xe2\x80\x93 Walkers")) { ImGui::End(); return; }

        ImGui::SeparatorText("Spawn parameters");
        ImGui::SliderFloat("Epsilon (band)##w",  &m_epsilon,    0.02f, 1.f,  "%.3f");
        ImGui::SliderFloat("Walk speed##w",       &m_walk_speed, 0.1f,  2.f,  "%.2f");
        ImGui::SliderFloat("Noise sigma##w",      &m_noise_sigma,0.f,   0.5f, "%.3f");
        ImGui::SliderFloat("Sim speed##w",        &m_sim_speed,  0.1f,  5.f,  "%.2f");
        ImGui::Spacing();

        if (ImGui::Button("Spawn walker  [Ctrl+W]", ImVec2(-1.f, 0.f)))
            spawn_walker();
        if (ImGui::Button("Clear all", ImVec2(-1.f, 0.f))) {
            m_curves.clear();
            m_spawn_count = 0;
        }

        ImGui::SeparatorText("Active walkers");
        ImGui::TextDisabled("%zu walker(s)", m_curves.size());
        ImGui::Spacing();

        // Per-walker live readout and editable params
        for (u32 wi = 0; wi < static_cast<u32>(m_curves.size()); ++wi) {
            ImGui::PushID(static_cast<int>(wi));
            auto& c = m_curves[wi];
            auto* eq = dynamic_cast<ndde::sim::LevelCurveWalker*>(c.equation());
            if (!eq) { ImGui::PopID(); continue; }

            auto& p = eq->params();
            const Vec3 hp = c.head_world();
            const float dz = hp.z - p.z0;

            const ImVec4 col = std::abs(dz) < p.epsilon
                ? ImVec4(0.4f,1.f,0.4f,1.f) : ImVec4(1.f,0.5f,0.2f,1.f);
            ImGui::TextColored(col, "Walker %u  z=%.3f  dz=%+.3f", wi, hp.z, dz);

            ImGui::SetNextItemWidth(120.f);
            ImGui::SliderFloat("z0##lw",  &p.z0,         -2.f,  2.f, "%.3f");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(100.f);
            ImGui::SliderFloat("eps##lw", &p.epsilon,     0.02f, 1.f, "%.3f");

            ImGui::SetNextItemWidth(120.f);
            ImGui::SliderFloat("spd##lw", &p.walk_speed,  0.1f,  2.f, "%.2f");

            ImGui::PopID();
            ImGui::Separator();
        }
        ImGui::End();
    }

    void draw_panel_camera() {
        ImGui::SetNextWindowPos(ImVec2(20.f, 760.f),  ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(290.f, 160.f), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowBgAlpha(0.88f);
        if (!ImGui::Begin("Analysis \xe2\x80\x93 Camera")) { ImGui::End(); return; }

        ImGui::SliderFloat("Yaw##ac",   &m_vp3d.yaw,
                           -std::numbers::pi_v<float>, std::numbers::pi_v<float>, "%.2f");
        ImGui::SliderFloat("Pitch##ac", &m_vp3d.pitch, -1.5f, 1.5f, "%.2f");
        ImGui::SliderFloat("Zoom##ac",  &m_vp3d.zoom,   0.1f,  8.f, "%.2f");
        if (ImGui::Button("Reset##ac")) m_vp3d.reset();

        ImGui::SeparatorText("Pause");
        ImGui::Checkbox("Paused  [Ctrl+P]", &m_paused);
        ImGui::End();
    }

    void draw_panel_debug() {
        ImGui::SetNextWindowPos(ImVec2(20.f, 930.f),  ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(290.f, 100.f),  ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowBgAlpha(0.88f);
        if (!ImGui::Begin("Analysis \xe2\x80\x93 Debug")) { ImGui::End(); return; }

        ImGui::SeparatorText("Scene");
        if (ImGui::Button("Switch to Surface Sim", ImVec2(-1.f, 0.f))) {
            m_api.switch_scene([](EngineAPI api) -> std::unique_ptr<IScene> {
                return std::make_unique<SurfaceSimScene>(std::move(api));
            });
        }
        ImGui::Spacing();
        ImGui::SeparatorText("Stats");
        if (ImGui::Button("Perf stats")) m_perf.visible() = !m_perf.visible();
        const auto& s = m_api.debug_stats();
        const ImVec4 fc = s.fps >= 55.f ? ImVec4(0.4f,1.f,0.4f,1.f)
                        : s.fps >= 30.f ? ImVec4(1.f,0.8f,0.f,1.f)
                                        : ImVec4(1.f,0.3f,0.3f,1.f);
        ImGui::SameLine();
        ImGui::TextColored(fc, "%.0f fps", s.fps);
        ImGui::End();
    }
};

} // namespace ndde

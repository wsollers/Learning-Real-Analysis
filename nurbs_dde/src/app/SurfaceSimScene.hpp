#pragma once
// app/SurfaceSimScene.hpp
// Gaussian surface simulation scene.
// Two dockable/tearable ImGui windows:
//   "Surface 3D"  — Vulkan perspective wireframe + animated curve + Frenet frame
//   "Contour 2D"  — ImDrawList heatmap + contour lines + trail (no Vulkan, always clipped)
// Floating panel:
//   "Simulation"  — controls + live Frenet readout
// Ctrl+D toggles the CoordDebugPanel.

#include "engine/EngineAPI.hpp"
#include "app/GaussianSurface.hpp"
#include "app/Viewport.hpp"
#include "app/HoverResult.hpp"
#include "app/PerformancePanel.hpp"
#include "app/CoordDebugPanel.hpp"

#include <imgui.h>
#include <memory>
#include <vector>
#include <string>

namespace ndde {

class SurfaceSimScene {
public:
    explicit SurfaceSimScene(EngineAPI api);
    ~SurfaceSimScene() = default;

    void on_frame(f32 dt);

private:
    EngineAPI        m_api;
    AnimatedCurve    m_curve;
    PerformancePanel m_perf;
    CoordDebugPanel  m_coord_debug;

    // ── Viewports ──────────────────────────────────────────────────────────────
    Viewport m_vp3d;   // perspective — used for aspect ratio and orbit state
    Viewport m_vp2d;   // ortho       — used for world↔pixel transforms in 2D

    // ── Canvas rect (updated each frame by draw_surface_3d_window) ─────────────
    // Stored so canvas_mvp_3d can be called from update_hover independently.
    ImVec2 m_canvas3d_pos{};
    ImVec2 m_canvas3d_sz{};

    // ── Simulation state ───────────────────────────────────────────────────────
    f32  m_sim_speed    = 1.f;
    bool m_sim_paused   = false;
    bool m_show_T       = true;
    bool m_show_N       = true;
    bool m_show_B       = true;
    bool m_show_osc     = true;
    bool m_show_contours= true;
    f32  m_frame_scale  = 0.25f;
    u32  m_grid_lines   = 28;
    bool m_debug_open   = false;

    // ── Hover / snap ──────────────────────────────────────────────────────────
    int  m_snap_idx     = -1;
    bool m_snap_on_curve= false;
    HoverResult m_hover;

    bool m_wireframe_dirty   = true;
    u32  m_cached_grid_lines = 0;
    bool m_ctrl_d_prev       = false;
    bool m_dock_built        = false;   ///< default dock layout applied once

    // ── Internal methods ──────────────────────────────────────────────────────
    void advance_simulation(f32 dt);
    void handle_hotkeys();

    // Builds a perspective MVP with an NDC→canvas-rect remap baked in,
    // so Vulkan geometry renders precisely into the ImGui window's screen area.
    [[nodiscard]] Mat4 canvas_mvp_3d(const ImVec2& cpos, const ImVec2& csz) const noexcept;

    void draw_simulation_panel();
    void draw_surface_3d_window();
    void draw_contour_2d_window();  // pure ImDrawList — no Vulkan

    void submit_wireframe_3d(const Mat4& mvp);
    void submit_trail_3d(const Mat4& mvp);
    void submit_head_dot_3d(const Mat4& mvp);
    void submit_frenet_3d(u32 trail_idx, const Mat4& mvp);
    void submit_osc_circle_3d(u32 trail_idx, const Mat4& mvp);
    void submit_contour_second_window();  ///< Vulkan to second OS window via submit2

    // Arrow helper (Vulkan only — used for 3D)
    void submit_arrow(Vec3 origin, Vec3 dir, Vec4 color, f32 length,
                      const Mat4& mvp, Topology topo = Topology::LineList);

    void update_hover(const ImVec2& canvas_pos, const ImVec2& canvas_size, bool is_3d);

    // Contour levels
    static constexpr f32 k_levels[] = {
        -1.2f,-0.9f,-0.6f,-0.3f, 0.f, 0.3f, 0.6f, 0.9f, 1.2f
    };
    static constexpr u32 k_n_levels = 9u;
};

} // namespace ndde

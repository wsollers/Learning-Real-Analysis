#pragma once
// app/SurfaceSimScene.hpp
// Gaussian surface simulation scene.
//
// Hotkeys:
//   Ctrl+F  — toggle Frenet frame  (T, N, B arrows + osculating circle)
//   Ctrl+D  — toggle surface frame (Dx, Dy tangents + κ_n / κ_g readout)
//   Ctrl+P  — toggle normal plane patch at particle
//   Ctrl+Q  — toggle coordinate debug panel

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
    std::vector<AnimatedCurve> m_curves;  ///< all active particles; Ctrl+L adds one
    PerformancePanel m_perf;
    CoordDebugPanel  m_coord_debug;

    // ── Viewports ─────────────────────────────────────────────────────────────
    Viewport m_vp3d;   // perspective
    Viewport m_vp2d;   // ortho — used by contour panel + second window

    // Canvas rect for the 3D window (updated each frame)
    ImVec2 m_canvas3d_pos{};
    ImVec2 m_canvas3d_sz{};

    // ── Simulation state ──────────────────────────────────────────────────────
    f32  m_sim_speed     = 1.f;
    bool m_sim_paused    = false;
    f32  m_frame_scale   = 0.25f;
    u32  m_grid_lines    = 28;
    bool m_show_contours = true;

    // ── Visibility toggles (hotkeys) ──────────────────────────────────────────
    bool m_show_frenet       = true;   ///< Ctrl+F
    bool m_show_dir_deriv    = false;  ///< Ctrl+D — surface frame Dx/Dy
    bool m_show_normal_plane = false;  ///< Ctrl+P — normal plane patch
    bool m_debug_open        = false;  ///< Ctrl+Q

    // Frenet sub-toggles (Simulation panel checkboxes)
    bool m_show_T   = true;
    bool m_show_N   = true;
    bool m_show_B   = true;
    bool m_show_osc = true;

    // ── Hotkey edge-detection state ───────────────────────────────────────────
    bool m_ctrl_f_prev = false;
    bool m_ctrl_d_prev = false;
    bool m_ctrl_p_prev = false;
    bool m_ctrl_q_prev = false;
    bool m_ctrl_l_prev = false;  ///< Ctrl+L: spawn new particle

    // ── Hover / snap ──────────────────────────────────────────────────────────
    int  m_snap_curve   = 0;     ///< which curve the snap belongs to
    int  m_snap_idx     = -1;
    bool m_snap_on_curve= false;
    HoverResult m_hover;

    bool m_wireframe_dirty   = true;
    u32  m_cached_grid_lines = 0;
    bool m_dock_built        = false;

    // ── Internal methods ──────────────────────────────────────────────────────
    void advance_simulation(f32 dt);
    void handle_hotkeys();

    [[nodiscard]] Mat4 canvas_mvp_3d(const ImVec2& cpos,
                                      const ImVec2& csz) const noexcept;

    void draw_simulation_panel();
    void draw_surface_3d_window();
    void draw_contour_2d_window();   ///< ImDrawList overlay on primary window

    void submit_wireframe_3d(const Mat4& mvp);
    void submit_trail_3d(const AnimatedCurve& c, const Mat4& mvp);
    void submit_head_dot_3d(const AnimatedCurve& c, const Mat4& mvp);
    void submit_frenet_3d(const AnimatedCurve& c, u32 trail_idx, const Mat4& mvp);
    void submit_osc_circle_3d(const AnimatedCurve& c, u32 trail_idx, const Mat4& mvp);
    void submit_surface_frame_3d(const AnimatedCurve& c, u32 trail_idx, const Mat4& mvp);
    void submit_normal_plane_3d(const AnimatedCurve& c, u32 trail_idx, const Mat4& mvp);

    void submit_contour_second_window();

    void submit_arrow(Vec3 origin, Vec3 dir, Vec4 color, f32 length,
                      const Mat4& mvp, Topology topo = Topology::LineList);

    void update_hover(const ImVec2& canvas_pos, const ImVec2& canvas_size,
                      bool is_3d);

    static constexpr f32 k_levels[] = {
        -1.2f,-0.9f,-0.6f,-0.3f, 0.f, 0.3f, 0.6f, 0.9f, 1.2f
    };
    static constexpr u32 k_n_levels = 9u;
};

} // namespace ndde

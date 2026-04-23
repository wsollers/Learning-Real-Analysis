// app/SurfaceSimScene.cpp
#include "app/SurfaceSimScene.hpp"
#include <imgui.h>
#include <imgui_internal.h>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <numbers>
#include <format>
#include <algorithm>

namespace ndde {

// ── Constructor ───────────────────────────────────────────────────────────────

SurfaceSimScene::SurfaceSimScene(EngineAPI api)
    : m_api(std::move(api))
{
    // 3D camera: dist = base_extent/zoom * 3 = 4/1.2*3 = 10.
    // tan(22.5°)*10 = 4.1 — surface spans ±4 in XY, fits the 45° frustum.
    m_vp3d.base_extent = 4.f;
    m_vp3d.zoom        = 1.2f;
    m_vp3d.yaw         = 0.55f;
    m_vp3d.pitch       = 0.42f;

    // 2D ortho viewport — world domain [-4,4]x[-4,4], half_h = base_extent/zoom = 4
    m_vp2d.base_extent = 4.f;
    m_vp2d.zoom        = 1.f;
    m_vp2d.pan_x       = 0.f;
    m_vp2d.pan_y       = 0.f;

    m_curve.reset();
    for (int i = 0; i < 400; ++i)
        m_curve.advance(1.f/60.f, 1.f);
}

// ── on_frame ──────────────────────────────────────────────────────────────────

void SurfaceSimScene::on_frame(f32 dt) {
    handle_hotkeys();
    advance_simulation(dt);

    // ── Fullscreen dockspace ──────────────────────────────────────────────────
    // Covers the entire OS window. The two view panels and the simulation panel
    // dock into this space. Users can drag panels to any edge to split the view.
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(vp->WorkPos);
    ImGui::SetNextWindowSize(vp->WorkSize);
    ImGui::SetNextWindowViewport(vp->ID);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));
    constexpr ImGuiWindowFlags dock_flags =
        ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoBackground;
    ImGui::Begin("##dockspace_root", nullptr, dock_flags);
    ImGui::PopStyleVar(3);
    const ImGuiID dock_id = ImGui::GetID("MainDockSpace");
    // No PassthruCentralNode — we want the Surface 3D window to be the
    // central node and fill the entire dockspace, not be transparent.
    ImGui::DockSpace(dock_id, ImVec2(0.f, 0.f), ImGuiDockNodeFlags_None);

    // Build default layout once on first run.
    // The "Contour 2D" ImGui panel (now a fallback overlay on the primary
    // window) fills the right half; "Surface 3D" fills the left.
    if (!m_dock_built) {
        m_dock_built = true;
        ImGui::DockBuilderRemoveNode(dock_id);
        ImGui::DockBuilderAddNode(dock_id,
            static_cast<ImGuiDockNodeFlags>(ImGuiDockNodeFlags_DockSpace));
        ImGui::DockBuilderSetNodeSize(dock_id, vp->WorkSize);
        // Surface 3D occupies the full dockspace on the primary window.
        // The contour view lives on the second OS window entirely.
        ImGui::DockBuilderDockWindow("Surface 3D", dock_id);
        ImGui::DockBuilderFinish(dock_id);
    }
    ImGui::End();

    draw_simulation_panel();
    draw_surface_3d_window();
    // draw_contour_2d_window() removed — 2D view lives in the second OS window
    submit_contour_second_window();  // Vulkan geometry on second OS window
    m_coord_debug.draw();
    m_perf.draw(m_api.debug_stats());
}

// ── advance_simulation ────────────────────────────────────────────────────────

void SurfaceSimScene::advance_simulation(f32 dt) {
    if (!m_sim_paused)
        m_curve.advance(dt, m_sim_speed);
}

// ── handle_hotkeys ────────────────────────────────────────────────────────────

void SurfaceSimScene::handle_hotkeys() {
    const ImGuiIO& io = ImGui::GetIO();
    const bool ctrl_d = io.KeyCtrl && ImGui::IsKeyDown(ImGuiKey_D);
    if (ctrl_d && !m_ctrl_d_prev)
        m_debug_open = !m_debug_open;
    m_ctrl_d_prev = ctrl_d;
    m_coord_debug.visible() = m_debug_open;
}

// ── canvas_mvp_3d ─────────────────────────────────────────────────────────────
// Builds a perspective MVP that maps world-space geometry into the pixel
// rectangle [cx, cy, cx+cw, cy+ch] on the swapchain (dimensions sw × sh).
//
// Standard NDC→screen:  screen_x = (ndx+1)/2 * sw
// We want:              screen_x = cx + (ndx+1)/2 * cw
//
// The adjustment is a post-projection linear remap of NDC X and Y.
// We compose it as:  M_remap * proj * view
//   scale_x = cw/sw,  scale_y = ch/sh
//   bias_x  = 2*cx/sw + cw/sw - 1,  bias_y = -(2*cy/sh + ch/sh - 1)
// (Y is flipped because the Vulkan negative-height viewport already handles Y.)

Mat4 SurfaceSimScene::canvas_mvp_3d(const ImVec2& cpos, const ImVec2& csz) const noexcept {
    const Vec2 sw_sz = m_api.viewport_size();
    const f32  sw    = sw_sz.x > 0.f ? sw_sz.x : 1.f;
    const f32  sh    = sw_sz.y > 0.f ? sw_sz.y : 1.f;
    const f32  cw    = csz.x   > 0.f ? csz.x   : 1.f;
    const f32  ch    = csz.y   > 0.f ? csz.y   : 1.f;
    // ViewportsEnable is off: cpos is always window-relative = swapchain-relative.
    const f32  cx    = cpos.x;
    const f32  cy    = cpos.y;

    // Standard NDC→screen:  screen_x = (ndx+1)/2 * sw
    // Desired:               screen_x = cx + (ndx_adj+1)/2 * cw
    // ⇒ ndx_adj = ndx*(cw/sw) + (2*cx/sw + cw/sw - 1)
    //
    // For Y: Vulkan uses a negative-height viewport, so NDC Y=+1 → screen top (y=0).
    // screen_y = (1 - ndy)/2 * sh
    // Desired:  screen_y = cy + (1 - ndy_adj)/2 * ch
    // ⇒ ndy_adj = ndy*(ch/sh) + (1 - 2*cy/sh - ch/sh)
    //           = ndy*(ch/sh) - (2*cy/sh + ch/sh - 1)
    //
    // Implemented as a post-projection NDC linear remap matrix (column-major GLM):
    //   col 0: (sx, 0,  0, 0)
    //   col 1: (0,  sy, 0, 0)
    //   col 2: (0,  0,  1, 0)
    //   col 3: (bx, by, 0, 1)
    // where sx=cw/sw, sy=ch/sh, bx=2cx/sw+sx-1, by=-(2cy/sh+sy-1)
    const f32 sx = cw / sw;
    const f32 sy = ch / sh;
    const f32 bx =  2.f*cx/sw + sx - 1.f;
    const f32 by = -(2.f*cy/sh + sy - 1.f);

    // GLM mat4 is column-major: mat[col][row]
    Mat4 remap(0.f);
    remap[0][0] = sx;   // col0 row0: scale X
    remap[1][1] = sy;   // col1 row1: scale Y
    remap[2][2] = 1.f;  // col2 row2: pass Z through
    remap[3][0] = bx;   // col3 row0: bias X
    remap[3][1] = by;   // col3 row1: bias Y
    remap[3][3] = 1.f;  // col3 row3: homogeneous

    const f32  aspect = cw / ch;
    const f32  dist   = m_vp3d.base_extent / m_vp3d.zoom * 3.f;
    const f32  ex     = m_vp3d.pan_x + dist*std::cos(m_vp3d.pitch)*std::sin(m_vp3d.yaw);
    const f32  ey     = m_vp3d.pan_y + dist*std::sin(m_vp3d.pitch);
    const f32  ez     =                dist*std::cos(m_vp3d.pitch)*std::cos(m_vp3d.yaw);
    const Mat4 proj   = glm::perspective(glm::radians(45.f), aspect, 0.01f, 500.f);
    const Mat4 view   = glm::lookAt(
        glm::vec3(ex, ey, ez),
        glm::vec3(m_vp3d.pan_x, m_vp3d.pan_y, 0.f),
        glm::vec3(0.f, 1.f, 0.f));

    return remap * proj * view;
}

// ── draw_simulation_panel ─────────────────────────────────────────────────────

void SurfaceSimScene::draw_simulation_panel() {
    ImGui::SetNextWindowPos(ImVec2(20.f, 20.f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(300.f, 0.f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowBgAlpha(0.88f);
    ImGui::Begin("Simulation");

    ImGui::SeparatorText("Curve");
    ImGui::Checkbox("Paused", &m_sim_paused);
    ImGui::SameLine();
    if (ImGui::Button("Reset")) m_curve.reset();
    ImGui::SliderFloat("Speed##sim",  &m_sim_speed,   0.1f, 5.f, "%.2f");
    ImGui::SliderFloat("Arrow scale", &m_frame_scale, 0.05f,0.8f,"%.2f");
    int gl = static_cast<int>(m_grid_lines);
    if (ImGui::SliderInt("Grid lines", &gl, 8, 60)) {
        m_grid_lines = static_cast<u32>(gl);
        m_wireframe_dirty = true;
    }

    ImGui::SeparatorText("Frenet frame");
    ImGui::Checkbox("T (tangent)",  &m_show_T);
    ImGui::SameLine();
    ImGui::Checkbox("N (normal)",   &m_show_N);
    ImGui::SameLine();
    ImGui::Checkbox("B (binormal)", &m_show_B);
    ImGui::Checkbox("Osc. circle",  &m_show_osc);
    ImGui::Checkbox("Contour lines",&m_show_contours);

    ImGui::SeparatorText("At head");
    if (m_curve.has_trail()) {
        const u32 hi = m_curve.trail_size() > 2 ? m_curve.trail_size()-2 : 0;
        const FrenetFrame fr = m_curve.frenet_at(hi);
        const Vec3 hp = m_curve.head_world();
        ImGui::TextDisabled("f(x,y) = %.4f", GaussianSurface::eval(hp.x, hp.y));
        ImGui::TextColored(ImVec4(1.f,0.5f,0.1f,1.f),
            "T (%.3f, %.3f, %.3f)", fr.T.x, fr.T.y, fr.T.z);
        ImGui::TextColored(ImVec4(0.2f,1.f,0.4f,1.f),
            "N (%.3f, %.3f, %.3f)", fr.N.x, fr.N.y, fr.N.z);
        ImGui::TextColored(ImVec4(0.3f,0.6f,1.f,1.f),
            "B (%.3f, %.3f, %.3f)", fr.B.x, fr.B.y, fr.B.z);
        ImGui::TextDisabled("kappa = %.5f  tau = %.5f", fr.kappa, fr.tau);
        const f32 osc_r = fr.kappa > 1e-5f ? 1.f/fr.kappa : 0.f;
        ImGui::TextDisabled("osc. radius = %.4f", osc_r);
        const f32 K = GaussianSurface::gaussian_curvature(hp.x, hp.y);
        const f32 H = GaussianSurface::mean_curvature(hp.x, hp.y);
        ImGui::TextDisabled("K = %.5f  H = %.5f", K, H);
    }

    ImGui::SeparatorText("3D camera");
    ImGui::SliderFloat("Yaw",  &m_vp3d.yaw,  -std::numbers::pi_v<f32>, std::numbers::pi_v<f32>, "%.2f");
    ImGui::SliderFloat("Pitch",&m_vp3d.pitch, -1.5f, 1.5f, "%.2f");
    ImGui::SliderFloat("Zoom##3d",&m_vp3d.zoom, 0.1f, 5.f, "%.2f");
    if (ImGui::Button("Reset 3D")) m_vp3d.reset();
    ImGui::SameLine();
    if (ImGui::Button("Reset 2D")) m_vp2d.reset();

    ImGui::SeparatorText("Debug");
    if (ImGui::Button("Coord Debug (Ctrl+D)"))
        m_debug_open = !m_debug_open;
    ImGui::SameLine();
    if (ImGui::Button("Perf")) m_perf.visible() = !m_perf.visible();
    const auto& s = m_api.debug_stats();
    const ImVec4 fc = s.fps >= 55.f ? ImVec4(0.4f,1.f,0.4f,1.f)
                    : s.fps >= 30.f ? ImVec4(1.f,0.8f,0.f,1.f)
                                    : ImVec4(1.f,0.3f,0.3f,1.f);
    ImGui::SameLine();
    ImGui::TextColored(fc, "%.0f fps", s.fps);

    ImGui::End();
}

// ── draw_surface_3d_window ────────────────────────────────────────────────────

void SurfaceSimScene::draw_surface_3d_window() {
    ImGui::SetNextWindowPos(ImVec2(330.f, 20.f),  ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(750.f, 660.f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowBgAlpha(0.0f);  // fully transparent — geometry is the background
    ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
    ImGui::Begin("Surface 3D", nullptr, flags);

    const ImVec2 cpos = ImGui::GetCursorScreenPos();
    const ImVec2 csz  = ImGui::GetContentRegionAvail();

    // Update the 3D viewport dimensions for aspect-ratio and mouse projection.
    m_vp3d.fb_w = csz.x;  m_vp3d.fb_h = csz.y;
    m_vp3d.dp_w = csz.x;  m_vp3d.dp_h = csz.y;

    // Store canvas rect so geometry submissions can build the corrected MVP.
    m_canvas3d_pos = cpos;
    m_canvas3d_sz  = csz;

    // InvisibleButton over content area for input capture.
    ImGui::InvisibleButton("3d_canvas", csz,
        ImGuiButtonFlags_MouseButtonLeft  |
        ImGuiButtonFlags_MouseButtonRight |
        ImGuiButtonFlags_MouseButtonMiddle);
    const bool hovered = ImGui::IsItemHovered();

    if (hovered) {
        const ImGuiIO& io = ImGui::GetIO();
        if (std::abs(io.MouseWheel) > 0.f)
            m_vp3d.zoom = std::clamp(m_vp3d.zoom*(1.f + 0.12f*io.MouseWheel), 0.05f, 20.f);
        if (ImGui::IsMouseDragging(ImGuiMouseButton_Right) ||
            ImGui::IsMouseDragging(ImGuiMouseButton_Middle))
            m_vp3d.orbit(io.MouseDelta.x, io.MouseDelta.y);
        update_hover(cpos, csz, true);
    } else {
        m_snap_on_curve = false;
        m_snap_idx = -1;
    }

    // Submit Vulkan geometry using the canvas-corrected MVP.
    const Mat4 mvp3d = canvas_mvp_3d(cpos, csz);
    submit_wireframe_3d(mvp3d);
    if (m_curve.has_trail()) {
        submit_trail_3d(mvp3d);
        submit_head_dot_3d(mvp3d);
        const u32 sidx = (m_snap_idx >= 0)
            ? static_cast<u32>(m_snap_idx)
            : (m_curve.trail_size() > 2 ? m_curve.trail_size()-2 : 0);
        submit_frenet_3d(sidx, mvp3d);
        if (m_show_osc) submit_osc_circle_3d(sidx, mvp3d);
    }

    // Text overlay.
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddText(ImVec2(cpos.x+8, cpos.y+6),
        IM_COL32(200,200,200,180), "Right-drag: orbit   Scroll: zoom");
    if (m_snap_on_curve && m_snap_idx >= 0) {
        const FrenetFrame fr = m_curve.frenet_at(static_cast<u32>(m_snap_idx));
        char buf[128];
        std::snprintf(buf,sizeof(buf), "k=%.4f  t=%.4f  osc.r=%.3f",
            fr.kappa, fr.tau, fr.kappa>1e-5f ? 1.f/fr.kappa : 0.f);
        dl->AddText(ImVec2(cpos.x+8, cpos.y+22), IM_COL32(255,220,100,220), buf);
    }

    ImGui::End();
}

// ── draw_contour_2d_window ────────────────────────────────────────────────────
// The 2D contour view uses ImDrawList exclusively — no Vulkan geometry.
// This means it is automatically clipped to the window bounds and works
// correctly when the window is torn off to a different monitor.

void SurfaceSimScene::draw_contour_2d_window() {
    ImGui::SetNextWindowPos(ImVec2(330.f,  700.f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(750.f, 560.f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowBgAlpha(0.0f);
    ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
    ImGui::Begin("Contour 2D", nullptr, flags);

    const ImVec2 cpos = ImGui::GetCursorScreenPos();
    const ImVec2 csz  = ImGui::GetContentRegionAvail();

    m_vp2d.fb_w = csz.x;  m_vp2d.fb_h = csz.y;
    m_vp2d.dp_w = csz.x;  m_vp2d.dp_h = csz.y;

    ImGui::InvisibleButton("2d_canvas", csz,
        ImGuiButtonFlags_MouseButtonLeft  |
        ImGuiButtonFlags_MouseButtonRight |
        ImGuiButtonFlags_MouseButtonMiddle);
    const bool hovered = ImGui::IsItemHovered();

    if (hovered) {
        const ImGuiIO& io = ImGui::GetIO();
        if (std::abs(io.MouseWheel) > 0.f)
            m_vp2d.zoom_toward(io.MousePos.x - cpos.x,
                               io.MousePos.y - cpos.y, io.MouseWheel);
        if (ImGui::IsMouseDragging(ImGuiMouseButton_Middle) ||
            (io.KeyAlt && ImGui::IsMouseDragging(ImGuiMouseButton_Left)))
            m_vp2d.pan_by_pixels(io.MouseDelta.x, io.MouseDelta.y);
        update_hover(cpos, csz, false);
    }

    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->PushClipRect(cpos, ImVec2(cpos.x+csz.x, cpos.y+csz.y), true);

    // ── Heatmap background ────────────────────────────────────────────────────
    // 80×80 tiles mapped to world coords via vp2d.
    {
        const int NX = 80, NY = 80;
        const f32 tw = csz.x/NX, th = csz.y/NY;
        for (int i = 0; i < NX; ++i) {
            for (int j = 0; j < NY; ++j) {
                const f32 wx = m_vp2d.left()+(i+.5f)/NX*(m_vp2d.right()-m_vp2d.left());
                const f32 wy = m_vp2d.top() -(j+.5f)/NY*(m_vp2d.top()  -m_vp2d.bottom());
                const Vec4 c = GaussianSurface::height_color(GaussianSurface::eval(wx,wy));
                dl->AddRectFilled(
                    ImVec2(cpos.x+i*tw,     cpos.y+j*th),
                    ImVec2(cpos.x+(i+1)*tw, cpos.y+(j+1)*th),
                    IM_COL32(int(c.r*255),int(c.g*255),int(c.b*255),210));
            }
        }
    }

    // Helper: world → canvas pixel
    auto w2s = [&](f32 wx, f32 wy) -> ImVec2 {
        const Vec2 sp = m_vp2d.world_to_pixel(wx, wy);
        return ImVec2(cpos.x + sp.x, cpos.y + sp.y);
    };

    // ── Contour lines ─────────────────────────────────────────────────────────
    if (m_show_contours) {
        constexpr int GN = 80;
        const f32 dx = (GaussianSurface::XMAX-GaussianSurface::XMIN)/GN;
        const f32 dy = (GaussianSurface::YMAX-GaussianSurface::YMIN)/GN;
        for (u32 li = 0; li < k_n_levels; ++li) {
            const f32 lv = k_levels[li];
            // Colour contours by level: blue→white→red
            const f32 t  = (lv - k_levels[0]) / (k_levels[k_n_levels-1] - k_levels[0]);
            const ImU32 cc = IM_COL32(int(t*200+55), int(255-t*100), int((1.f-t)*200+55), 200);
            for (int i = 0; i < GN; ++i) {
                for (int j = 0; j < GN; ++j) {
                    const f32 x0=GaussianSurface::XMIN+i*dx, x1=x0+dx;
                    const f32 y0=GaussianSurface::YMIN+j*dy, y1=y0+dy;
                    const f32 v00=GaussianSurface::eval(x0,y0),v10=GaussianSurface::eval(x1,y0);
                    const f32 v01=GaussianSurface::eval(x0,y1),v11=GaussianSurface::eval(x1,y1);
                    f32 pts[8]; u32 np=0;
                    auto edge=[&](f32 va,f32 vb,f32 xa,f32 ya,f32 xb,f32 yb){
                        if((va-lv)*(vb-lv)<0.f){
                            const f32 t2=(lv-va)/(vb-va);
                            pts[np++]=xa+t2*(xb-xa); pts[np++]=ya+t2*(yb-ya);
                        }
                    };
                    edge(v00,v10,x0,y0,x1,y0); edge(v10,v11,x1,y0,x1,y1);
                    edge(v11,v01,x1,y1,x0,y1); edge(v01,v00,x0,y1,x0,y0);
                    if(np>=4)
                        dl->AddLine(w2s(pts[0],pts[1]), w2s(pts[2],pts[3]), cc, 1.2f);
                }
            }
        }
    }

    // ── Trail ─────────────────────────────────────────────────────────────────
    const u32 nt = m_curve.trail_size();
    if (nt >= 2) {
        for (u32 i = 1; i < nt; ++i) {
            const Vec3 a = m_curve.trail_pt(i-1);
            const Vec3 b = m_curve.trail_pt(i);
            const f32  t = static_cast<f32>(i)/static_cast<f32>(nt-1);
            const ImU32 tc = IM_COL32(int(t*255), int(t*200+55), 255, int(80+175*t));
            dl->AddLine(w2s(a.x,a.y), w2s(b.x,b.y), tc, 1.8f);
        }
        // Head dot
        const Vec3 hp = m_curve.head_world();
        dl->AddCircleFilled(w2s(hp.x,hp.y), 4.f, IM_COL32(255,255,255,230));
    }

    // ── Frenet arrows at snap/head ────────────────────────────────────────────
    {
        const u32 sidx = (m_snap_idx >= 0 && static_cast<u32>(m_snap_idx) < nt)
            ? static_cast<u32>(m_snap_idx)
            : (nt > 2 ? nt-2 : 0);
        if (nt >= 3 && sidx > 0) {
            const FrenetFrame fr = m_curve.frenet_at(sidx);
            const Vec3 o3 = m_curve.trail_pt(sidx);
            const ImVec2 os = w2s(o3.x, o3.y);
            // Scale: 1 world unit → how many canvas pixels?
            const f32 ppu = csz.x / (m_vp2d.right()-m_vp2d.left());
            const f32 scl = m_frame_scale * ppu * 0.5f;
            auto arrow2d = [&](Vec3 dir, ImU32 col) {
                const Vec2 end_w = m_vp2d.world_to_pixel(o3.x+dir.x*m_frame_scale*0.5f,
                                                          o3.y+dir.y*m_frame_scale*0.5f);
                const ImVec2 es  = ImVec2(cpos.x+end_w.x, cpos.y+end_w.y);
                dl->AddLine(os, es, col, 2.f);
                // Arrowhead
                const f32 ax = es.x-os.x, ay = es.y-os.y;
                const f32 len2 = std::sqrt(ax*ax+ay*ay)+1e-5f;
                const f32 nx2 = ax/len2, ny2 = ay/len2;
                const f32 sz = 6.f;
                dl->AddTriangleFilled(
                    es,
                    ImVec2(es.x-sz*nx2+sz*0.5f*(-ny2), es.y-sz*ny2+sz*0.5f*nx2),
                    ImVec2(es.x-sz*nx2-sz*0.5f*(-ny2), es.y-sz*ny2-sz*0.5f*nx2),
                    col);
                (void)scl;
            };
            if (m_show_T) arrow2d(fr.T, IM_COL32(255,90,20,230));
            if (m_show_N) arrow2d(fr.N, IM_COL32(40,255,90,230));
            if (m_show_B) {
                // B is mostly into/out of screen in 2D — draw as a circle indicator
                const f32 Bz = fr.B.z;
                const ImU32 bc = Bz > 0 ? IM_COL32(60,130,255,200) : IM_COL32(60,80,200,200);
                dl->AddCircle(os, 7.f, bc, 12, 2.f);
                dl->AddText(ImVec2(os.x+9,os.y-7), bc, Bz>0 ? "B+" : "B-");
            }

            // Osculating circle in 2D (project circle onto XY plane)
            if (m_show_osc && fr.kappa > 1e-5f) {
                const f32   R  = 1.f / fr.kappa;
                const ImVec2 ctr = w2s(o3.x + fr.N.x*R, o3.y + fr.N.y*R);
                const f32   rpx = R * ppu;
                if (rpx < 2000.f)
                    dl->AddCircle(ctr, rpx, IM_COL32(180,80,255,140), 48, 1.5f);
            }

            // Snap indicator dot
            dl->AddCircle(os, 5.f, IM_COL32(255,255,255,200), 12, 1.5f);
        }
    }

    // ── Tooltip overlay ───────────────────────────────────────────────────────
    {
        dl->AddText(ImVec2(cpos.x+8,cpos.y+6),
            IM_COL32(220,220,220,160), "Mid-drag: pan   Scroll: zoom");
        if (m_snap_on_curve && m_snap_idx >= 0 &&
            static_cast<u32>(m_snap_idx) < nt)
        {
            const Vec3 pt = m_curve.trail_pt(static_cast<u32>(m_snap_idx));
            const FrenetFrame fr = m_curve.frenet_at(static_cast<u32>(m_snap_idx));
            char buf[160];
            std::snprintf(buf,sizeof(buf),
                "f(%.2f,%.2f)=%.4f  k=%.4f  K=%.4f",
                pt.x, pt.y, GaussianSurface::eval(pt.x,pt.y),
                fr.kappa, GaussianSurface::gaussian_curvature(pt.x,pt.y));
            dl->AddText(ImVec2(cpos.x+8,cpos.y+22), IM_COL32(255,220,100,230), buf);
        }
    }

    dl->PopClipRect();
    ImGui::End();
}

// ── update_hover ──────────────────────────────────────────────────────────────

void SurfaceSimScene::update_hover(const ImVec2& canvas_pos,
                                    const ImVec2& canvas_size,
                                    bool is_3d)
{
    const ImGuiIO& io    = ImGui::GetIO();
    const f32      mx    = io.MousePos.x - canvas_pos.x;
    const f32      my    = io.MousePos.y - canvas_pos.y;
    constexpr f32  SNAP  = 20.f;

    m_snap_idx      = -1;
    m_snap_on_curve = false;
    const u32 n = m_curve.trail_size();
    if (n == 0) return;

    if (is_3d) {
        const Mat4 mvp = canvas_mvp_3d(canvas_pos, canvas_size);
        const Vec2 sw_sz = m_api.viewport_size();
        const f32  sw = sw_sz.x > 0.f ? sw_sz.x : 1.f;
        const f32  sh = sw_sz.y > 0.f ? sw_sz.y : 1.f;
        f32 best = SNAP;
        for (u32 i = 0; i < n; ++i) {
            const Vec3 wp = m_curve.trail_pt(i);
            const glm::vec4 clip = mvp * glm::vec4(wp.x, wp.y, wp.z, 1.f);
            if (clip.w <= 0.f) continue;
            // remap is baked into mvp: NDC already maps to canvas screen rect.
            // NDC → swapchain pixel, then subtract canvas origin for canvas-relative.
            const f32 px = (clip.x/clip.w + 1.f) * 0.5f * sw - canvas_pos.x;
            const f32 py = (1.f - clip.y/clip.w) * 0.5f * sh - canvas_pos.y;
            const f32 d  = std::hypot(px - mx, py - my);
            if (d < best) { best = d; m_snap_idx = static_cast<int>(i); }
        }
    } else {
        f32 best = SNAP;
        for (u32 i = 0; i < n; ++i) {
            const Vec3 wp = m_curve.trail_pt(i);
            const Vec2 sp = m_vp2d.world_to_pixel(wp.x, wp.y);
            const f32  d  = std::hypot(sp.x-mx, sp.y-my);
            if (d < best) { best=d; m_snap_idx=static_cast<int>(i); }
        }
    }
    if (m_snap_idx >= 0) m_snap_on_curve = true;
}

// ── submit_arrow ──────────────────────────────────────────────────────────────

void SurfaceSimScene::submit_arrow(Vec3 origin, Vec3 dir, Vec4 color,
                                    f32 length, const Mat4& mvp, Topology topo)
{
    auto slice = m_api.acquire(2);
    Vertex* v  = slice.vertices();
    v[0] = { origin,              color };
    v[1] = { origin + dir*length, color };
    m_api.submit(slice, topo, DrawMode::VertexColor, color, mvp);
}

// ── submit_wireframe_3d ───────────────────────────────────────────────────────

void SurfaceSimScene::submit_wireframe_3d(const Mat4& mvp) {
    const u32 n = GaussianSurface::wireframe_vertex_count(m_grid_lines, m_grid_lines);
    auto slice  = m_api.acquire(n);
    GaussianSurface::tessellate_wireframe({slice.vertices(), n},
                                           m_grid_lines, m_grid_lines);
    m_api.submit(slice, Topology::LineList, DrawMode::VertexColor, {1,1,1,1}, mvp);
}

// ── submit_trail_3d ───────────────────────────────────────────────────────────

void SurfaceSimScene::submit_trail_3d(const Mat4& mvp) {
    const u32 n = m_curve.trail_vertex_count();
    if (n < 2) return;
    auto slice = m_api.acquire(n);
    m_curve.tessellate_trail({slice.vertices(), n});
    m_api.submit(slice, Topology::LineStrip, DrawMode::VertexColor, {1,1,1,1}, mvp);
}

// ── submit_head_dot_3d ────────────────────────────────────────────────────────

void SurfaceSimScene::submit_head_dot_3d(const Mat4& mvp) {
    const Vec3 hp = m_curve.head_world();
    auto slice = m_api.acquire(1);
    slice.vertices()[0] = { hp, {1.f,1.f,1.f,1.f} };
    m_api.submit(slice, Topology::LineStrip, DrawMode::UniformColor, {1,1,1,1}, mvp);
}

// ── submit_frenet_3d ──────────────────────────────────────────────────────────

void SurfaceSimScene::submit_frenet_3d(u32 trail_idx, const Mat4& mvp) {
    if (!m_curve.has_trail() || trail_idx == 0) return;
    const FrenetFrame fr = m_curve.frenet_at(trail_idx);
    const Vec3 o = m_curve.trail_pt(trail_idx);
    if (m_show_T) submit_arrow(o, fr.T, {1.f,0.35f,0.05f,1.f}, m_frame_scale, mvp);
    if (m_show_N) submit_arrow(o, fr.N, {0.15f,1.f,0.3f,1.f},  m_frame_scale, mvp);
    if (m_show_B) submit_arrow(o, fr.B, {0.2f,0.5f,1.f,1.f},   m_frame_scale, mvp);
}

// ── submit_osc_circle_3d ─────────────────────────────────────────────────────

void SurfaceSimScene::submit_osc_circle_3d(u32 trail_idx, const Mat4& mvp) {
    if (!m_curve.has_trail() || trail_idx == 0) return;
    const FrenetFrame fr = m_curve.frenet_at(trail_idx);
    if (fr.kappa < 1e-5f) return;
    const f32  R      = 1.f / fr.kappa;
    const Vec3 o      = m_curve.trail_pt(trail_idx);
    const Vec3 centre = o + fr.N * R;
    constexpr u32 SEG = 64;
    const Vec4 col = {0.7f,0.3f,1.f,0.75f};
    auto slice = m_api.acquire(SEG+1);
    Vertex* v  = slice.vertices();
    for (u32 i = 0; i <= SEG; ++i) {
        const f32  theta = (static_cast<f32>(i)/SEG)*2.f*std::numbers::pi_v<f32>;
        v[i] = { centre + R*(-std::cos(theta)*fr.N + std::sin(theta)*fr.T), col };
    }
    m_api.submit(slice, Topology::LineStrip, DrawMode::VertexColor, col, mvp);
}

// ── submit_contour_second_window ───────────────────────────────────────────────────
// Submits the heatmap + contours + trail via api.submit2 to the second OS
// window. Called every frame AFTER draw_contour_2d_window (ImGui panel).

void SurfaceSimScene::submit_contour_second_window() {
    const Vec2 sz2 = m_api.viewport_size2();
    if (sz2.x <= 0.f || sz2.y <= 0.f) return;

    // ── Ortho MVP: surface domain fills the second window exactly ──────────────
    // The surface lives in world-space [-4,4]x[-4,4]. We want it to fill the
    // full second window. Respect the window's aspect ratio by extending the
    // shorter world-space axis: the domain covers [-4,4] in both X and Y, and
    // we expand whichever axis is needed so no black bars appear.
    //
    // pan/zoom from m_vp2d lets the Simulation panel control the 2D view.
    const f32 domain = GaussianSurface::XMAX; // 4.0
    const f32 aspect = sz2.x / sz2.y;
    // half extents in world space that fill the window
    const f32 half_y = domain / m_vp2d.zoom;
    const f32 half_x = half_y * aspect;
    const f32 left2  = m_vp2d.pan_x - half_x;
    const f32 right2 = m_vp2d.pan_x + half_x;
    // glm::ortho: bottom = pan_y - half_y, top = pan_y + half_y
    // Vulkan negative-height viewport flips Y, so we pass bottom < top and let
    // the hardware flip handle it — consistent with how the primary swapchain works.
    const f32 bot2   = m_vp2d.pan_y - half_y;
    const f32 top2   = m_vp2d.pan_y + half_y;
    const Mat4 mvp2  = glm::ortho(left2, right2, bot2, top2, -10.f, 10.f);

    // ── Heatmap triangle mesh ─────────────────────────────────────────────────
    {
        constexpr u32 NX=60, NY=60;
        auto slice=m_api.acquire(NX*NY*6);
        Vertex* v=slice.vertices(); u32 idx=0;
        const f32 dx=(GaussianSurface::XMAX-GaussianSurface::XMIN)/NX;
        const f32 dy=(GaussianSurface::YMAX-GaussianSurface::YMIN)/NY;
        for (u32 i=0;i<NX;++i) for (u32 j=0;j<NY;++j) {
            const f32 x0=GaussianSurface::XMIN+i*dx,x1=x0+dx;
            const f32 y0=GaussianSurface::YMIN+j*dy,y1=y0+dy;
            const Vec4 c00=GaussianSurface::height_color(GaussianSurface::eval(x0,y0));
            const Vec4 c10=GaussianSurface::height_color(GaussianSurface::eval(x1,y0));
            const Vec4 c01=GaussianSurface::height_color(GaussianSurface::eval(x0,y1));
            const Vec4 c11=GaussianSurface::height_color(GaussianSurface::eval(x1,y1));
            v[idx++]={Vec3{x0,y0,0},c00}; v[idx++]={Vec3{x1,y0,0},c10}; v[idx++]={Vec3{x1,y1,0},c11};
            v[idx++]={Vec3{x0,y0,0},c00}; v[idx++]={Vec3{x1,y1,0},c11}; v[idx++]={Vec3{x0,y1,0},c01};
        }
        memory::ArenaSlice tr=slice; tr.vertex_count=idx;
        m_api.submit2(tr,Topology::TriangleList,DrawMode::VertexColor,{1,1,1,1},mvp2);
    }

    // ── Contour lines ────────────────────────────────────────────────────────
    if (m_show_contours) {
        const u32 max_v=GaussianSurface::contour_max_vertices(80u,k_n_levels);
        auto slice=m_api.acquire(max_v);
        const u32 actual=GaussianSurface::tessellate_contours(
            {slice.vertices(),max_v},80u,k_levels,k_n_levels,{1,1,1,0.8f});
        if (actual>0) { memory::ArenaSlice tr=slice; tr.vertex_count=actual;
            m_api.submit2(tr,Topology::LineList,DrawMode::VertexColor,{1,1,1,1},mvp2); }
    }

    // ── Trail ─────────────────────────────────────────────────────────────────
    const u32 nt=m_curve.trail_size();
    if (nt>=2) {
        auto slice=m_api.acquire(nt); Vertex* vt=slice.vertices();
        for (u32 i=0;i<nt;++i) {
            const Vec3 pt=m_curve.trail_pt(i);
            const f32 t=static_cast<f32>(i)/static_cast<f32>(nt-1);
            vt[i]={Vec3{pt.x,pt.y,0},{t,t*0.8f+0.2f,1.f,0.3f+0.7f*t}};
        }
        m_api.submit2(slice,Topology::LineStrip,DrawMode::VertexColor,{1,1,1,1},mvp2);
    }

    // ── Frenet arrows (2D projected) ──────────────────────────────────────
    if (nt>=3) {
        const u32 sidx=(m_snap_idx>=0&&static_cast<u32>(m_snap_idx)<nt)
            ?static_cast<u32>(m_snap_idx):nt-2;
        if (sidx>0) {
            const FrenetFrame fr=m_curve.frenet_at(sidx);
            const Vec3 o3=m_curve.trail_pt(sidx);
            const Vec3 o={o3.x,o3.y,0};
            const f32 scl=m_frame_scale*0.4f;
            auto arr=[&](Vec3 dir,Vec4 col){
                if (glm::length(dir)<1e-5f) return;
                auto s=m_api.acquire(2);
                s.vertices()[0]={o,col};
                s.vertices()[1]={o+glm::normalize(dir)*scl,col};
                m_api.submit2(s,Topology::LineList,DrawMode::VertexColor,col,mvp2);
            };
            if (m_show_T) arr({fr.T.x,fr.T.y,0},{1.f,0.35f,0.05f,1.f});
            if (m_show_N) arr({fr.N.x,fr.N.y,0},{0.15f,1.f,0.3f,1.f});
            if (m_show_osc&&fr.kappa>1e-5f) {
                const f32 R=1.f/fr.kappa;
                constexpr u32 SEG=48;
                auto sc2=m_api.acquire(SEG+1); Vertex* vc=sc2.vertices();
                const Vec3 ctr={o3.x+fr.N.x*R,o3.y+fr.N.y*R,0};
                for (u32 k=0;k<=SEG;++k) {
                    const f32 th=(static_cast<f32>(k)/SEG)*2.f*std::numbers::pi_v<f32>;
                    vc[k]={ctr+Vec3{std::cos(th)*R,std::sin(th)*R,0},{0.7f,0.3f,1.f,0.65f}};
                }
                m_api.submit2(sc2,Topology::LineStrip,DrawMode::VertexColor,{0.7f,0.3f,1.f,0.65f},mvp2);
            }
        }
    }
}

} // namespace ndde

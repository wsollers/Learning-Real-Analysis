#include "app/SurfaceSimScene.hpp"
#include "engine/CanvasInput.hpp"
#include "engine/RenderSubmission.hpp"
#include "numeric/ops.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <numbers>

namespace ndde {

// ── draw_surface_3d_window ────────────────────────────────────────────────────

void SurfaceSimScene::draw_surface_3d_window() {
    ImGui::SetNextWindowPos(ImVec2(330.f, 20.f),  ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(750.f, 660.f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowBgAlpha(0.0f);
    ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse |
        ImGuiWindowFlags_NoBackground;
    ImGui::Begin("Surface 3D", nullptr, flags);

    const ImVec2 csz  = ImGui::GetContentRegionAvail();
    const CanvasInputFrame canvas = begin_canvas_input("3d_canvas", csz);
    const ImVec2 cpos = canvas.pos;

    m_vp3d.fb_w = csz.x;  m_vp3d.fb_h = csz.y;
    m_vp3d.dp_w = csz.x;  m_vp3d.dp_h = csz.y;
    m_hover_state.canvas3d_pos = cpos;
    m_hover_state.canvas3d_sz  = csz;

    if (canvas.hovered) {
        if (std::abs(canvas.mouse_wheel) > 0.f)
            m_vp3d.zoom = std::clamp(m_vp3d.zoom*(1.f + 0.12f*canvas.mouse_wheel), 0.05f, 20.f);
        if (canvas.orbit_drag)
            m_vp3d.orbit(canvas.mouse_delta.x, canvas.mouse_delta.y);
        update_hover(cpos, csz, true);
    } else {
        m_hover_state.snap_on_curve = false;
        m_hover_state.snap_idx      = -1;
    }

    const Mat4 mvp3d = canvas_mvp_3d(cpos, csz);

    switch (m_display.mode) {
        case SurfaceDisplay::Wireframe: submit_wireframe_3d(mvp3d); break;
        case SurfaceDisplay::Filled:    submit_filled_3d(mvp3d);    break;
        case SurfaceDisplay::Both:
            submit_filled_3d(mvp3d);
            submit_wireframe_3d(mvp3d);
            break;
    }

    // Sync flags and delegate particle rendering to ParticleRenderer (B2).
    m_particle_renderer.show_frenet       = m_overlays.show_frenet;
    m_particle_renderer.show_T            = m_overlays.show_T;
    m_particle_renderer.show_N            = m_overlays.show_N;
    m_particle_renderer.show_B            = m_overlays.show_B;
    m_particle_renderer.show_osc          = m_overlays.show_osc;
    m_particle_renderer.show_dir_deriv    = m_overlays.show_dir_deriv;
    m_particle_renderer.show_normal_plane = m_overlays.show_normal_plane;
    m_particle_renderer.show_torsion      = m_overlays.show_torsion;
    m_particle_renderer.frame_scale       = m_display.frame_scale;
    m_particle_renderer.submit_all(m_curves, *m_surface, m_sim_time,
                                   mvp3d, m_hover_state.snap_curve, m_hover_state.snap_idx, m_hover_state.snap_on_curve);

    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddText(ImVec2(cpos.x+8, cpos.y+6),
        IM_COL32(200,200,200,180),
        "Right-drag: orbit   Scroll: zoom   Ctrl+L: add particle");

    if (m_paused)
        dl->AddText(ImVec2(cpos.x+8, cpos.y+24),
            IM_COL32(255, 210, 60, 240), "PAUSED  [Ctrl+P to resume]");

    if (m_hover_state.snap_on_curve && m_hover_state.snap_idx >= 0 && m_hover_state.snap_curve >= 0 &&
        m_hover_state.snap_curve < static_cast<int>(m_curves.size()))
    {
        const FrenetFrame fr = m_curves[m_hover_state.snap_curve].frenet_at(
            static_cast<u32>(m_hover_state.snap_idx));
        char buf[128];
        std::snprintf(buf, sizeof(buf), "k=%.4f  t=%.4f  osc.r=%.3f",
            fr.kappa, fr.tau, fr.kappa>1e-5f ? 1.f/fr.kappa : 0.f);
        dl->AddText(ImVec2(cpos.x+8, cpos.y+22), IM_COL32(255,220,100,220), buf);
    }

    ImGui::End();
}

// ── draw_contour_2d_window ────────────────────────────────────────────────────

void SurfaceSimScene::draw_contour_2d_window() {
    // Not called from on_frame() -- second window handles 2D rendering.
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

    m_hover_state.snap_idx      = -1;
    m_hover_state.snap_curve    = 0;
    m_hover_state.snap_on_curve = false;

    if (m_curves.empty()) return;

    if (is_3d) {
        const Mat4 mvp   = canvas_mvp_3d(canvas_pos, canvas_size);
        const Vec2 sw_sz = m_api.viewport_size();
        const f32  sw    = sw_sz.x > 0.f ? sw_sz.x : 1.f;
        const f32  sh    = sw_sz.y > 0.f ? sw_sz.y : 1.f;
        f32 best = SNAP;
        for (u32 ci = 0; ci < static_cast<u32>(m_curves.size()); ++ci) {
            const AnimatedCurve& c = m_curves[ci];
            for (u32 i = 0; i < c.trail_size(); ++i) {
                const Vec3 wp = c.trail_pt(i);
                const glm::vec4 clip = mvp * glm::vec4(wp.x, wp.y, wp.z, 1.f);
                if (clip.w <= 0.f) continue;
                const f32 px = (clip.x/clip.w + 1.f) * 0.5f * sw - canvas_pos.x;
                const f32 py = (1.f - clip.y/clip.w) * 0.5f * sh - canvas_pos.y;
                const f32 d  = std::hypot(px - mx, py - my);
                if (d < best) { best = d; m_hover_state.snap_idx = (int)i; m_hover_state.snap_curve = (int)ci; }
            }
        }
    } else {
        f32 best = SNAP;
        for (u32 ci = 0; ci < static_cast<u32>(m_curves.size()); ++ci) {
            const AnimatedCurve& c = m_curves[ci];
            for (u32 i = 0; i < c.trail_size(); ++i) {
                const Vec3 wp = c.trail_pt(i);
                const Vec2 sp = m_vp2d.world_to_pixel(wp.x, wp.y);
                const f32  d  = std::hypot(sp.x-mx, sp.y-my);
                if (d < best) { best = d; m_hover_state.snap_idx = (int)i; m_hover_state.snap_curve = (int)ci; }
            }
        }
    }

    if (m_hover_state.snap_idx >= 0) m_hover_state.snap_on_curve = true;
}

// ── Surface geometry caches ───────────────────────────────────────────────────

namespace {

SurfaceMeshOptions surface_mesh_options(u32 grid_lines, float time, float color_scale) {
    return SurfaceMeshOptions{
        .grid_lines = grid_lines,
        .time = time,
        .color_scale = color_scale,
        .wire_color = {1.f, 1.f, 1.f, 1.f},
        .fill_color_mode = SurfaceFillColorMode::GaussianCurvatureCell,
        .build_contour = false
    };
}

} // namespace

void SurfaceSimScene::submit_wireframe_3d(const Mat4& mvp) {
    m_display.mesh.rebuild_if_needed(*m_surface, surface_mesh_options(m_display.grid_lines, m_sim_time, m_display.curv_scale));
    if (m_display.mesh.wire_count() == 0) return;
    submit_vertices(m_api, RenderTarget::Primary3D, m_display.mesh.wire_vertices(), m_display.mesh.wire_count(),
                    Topology::LineList, DrawMode::VertexColor, {1,1,1,1}, mvp);
}

void SurfaceSimScene::submit_filled_3d(const Mat4& mvp) {
    m_display.mesh.rebuild_if_needed(*m_surface, surface_mesh_options(m_display.grid_lines, m_sim_time, m_display.curv_scale));
    if (m_display.mesh.fill_count() == 0) return;
    submit_vertices(m_api, RenderTarget::Primary3D, m_display.mesh.fill_vertices(), m_display.mesh.fill_count(),
                    Topology::TriangleList, DrawMode::VertexColor, {1,1,1,1}, mvp);
}

// ── submit_contour_second_window ──────────────────────────────────────────────

void SurfaceSimScene::submit_contour_second_window() {
    const Vec2 sz2 = m_api.viewport_size2();
    if (sz2.x <= 0.f || sz2.y <= 0.f) return;

    const f32 domain = m_surface->u_max();
    const f32 aspect = sz2.x / sz2.y;
    const f32 half_y = domain / m_vp2d.zoom;
    const f32 half_x = half_y * aspect;
    const Mat4 mvp2  = glm::ortho(
        m_vp2d.pan_x - half_x, m_vp2d.pan_x + half_x,
        m_vp2d.pan_y - half_y, m_vp2d.pan_y + half_y,
        -10.f, 10.f);

    const auto* gs = dynamic_cast<const GaussianSurface*>(m_surface.get());
    if (gs) {
        constexpr u32 NX=80, NY=80;
        auto slice = m_api.acquire(NX*NY*6);
        Vertex* v  = slice.vertices(); u32 idx = 0;
        const f32 dx = (gs->u_max()-gs->u_min())/NX;
        const f32 dy = (gs->v_max()-gs->v_min())/NY;
        for (u32 i=0;i<NX;++i) for (u32 j=0;j<NY;++j) {
            const f32 x0=gs->u_min()+i*dx, x1=x0+dx;
            const f32 y0=gs->v_min()+j*dy, y1=y0+dy;
            const Vec4 c00=GaussianSurface::height_color(GaussianSurface::eval(x0,y0));
            const Vec4 c10=GaussianSurface::height_color(GaussianSurface::eval(x1,y0));
            const Vec4 c01=GaussianSurface::height_color(GaussianSurface::eval(x0,y1));
            const Vec4 c11=GaussianSurface::height_color(GaussianSurface::eval(x1,y1));
            v[idx++]={Vec3{x0,y0,0},c00}; v[idx++]={Vec3{x1,y0,0},c10}; v[idx++]={Vec3{x1,y1,0},c11};
            v[idx++]={Vec3{x0,y0,0},c00}; v[idx++]={Vec3{x1,y1,0},c11}; v[idx++]={Vec3{x0,y1,0},c01};
        }
        memory::ArenaSlice tr=slice; tr.vertex_count=idx;
        m_api.submit_to(RenderTarget::Contour2D, tr, Topology::TriangleList, DrawMode::VertexColor, {1,1,1,1}, mvp2);

        if (m_display.show_contours) {
            const u32 max_v = GaussianSurface::contour_max_vertices(100u, k_n_levels);
            auto cslice = m_api.acquire(max_v);
            const u32 actual = GaussianSurface::tessellate_contours(
                {cslice.vertices(),max_v}, 100u, k_levels, k_n_levels, {1,1,1,0.8f});
            if (actual > 0) {
                memory::ArenaSlice tr2=cslice; tr2.vertex_count=actual;
                m_api.submit_to(RenderTarget::Contour2D, tr2, Topology::LineList, DrawMode::VertexColor, {1,1,1,1}, mvp2);
            }
        }
    }

    for (const AnimatedCurve& c : m_curves) {
        const u32 nt = c.trail_size();
        if (nt < 2) continue;
        auto slice = m_api.acquire(nt);
        Vertex* vt = slice.vertices();
        for (u32 i = 0; i < nt; ++i) {
            const Vec3 pt = c.trail_pt(i);
            const f32  t  = static_cast<f32>(i)/static_cast<f32>(nt-1);
            vt[i] = { Vec3{pt.x,pt.y,0},
                      AnimatedCurve::trail_colour(c.role(), c.colour_slot(), t) };
        }
        m_api.submit_to(RenderTarget::Contour2D, slice, Topology::LineStrip, DrawMode::VertexColor, {1,1,1,1}, mvp2);
    }

    {
        const int ci = (m_hover_state.snap_on_curve && m_hover_state.snap_curve >= 0 &&
                        m_hover_state.snap_curve < static_cast<int>(m_curves.size()))
                       ? m_hover_state.snap_curve : 0;
        if (ci < static_cast<int>(m_curves.size())) {
            const AnimatedCurve& ac = m_curves[ci];
            const u32 nt = ac.trail_size();
            const u32 sidx = (m_hover_state.snap_on_curve && m_hover_state.snap_curve == ci &&
                              m_hover_state.snap_idx >= 0 && static_cast<u32>(m_hover_state.snap_idx) < nt)
                ? static_cast<u32>(m_hover_state.snap_idx)
                : (nt > 2 ? nt-2 : 0);
            if (nt >= 3 && sidx > 0) {
                const FrenetFrame fr = ac.frenet_at(sidx);
                const Vec3 o3 = ac.trail_pt(sidx);
                const Vec3 o  = {o3.x, o3.y, 0};
                const f32  scl = m_display.frame_scale * 0.4f;
                auto arr = [&](Vec3 dir, Vec4 col) {
                    if (glm::length(dir) < 1e-5f) return;
                    auto s = m_api.acquire(2);
                    s.vertices()[0] = {o, col};
                    s.vertices()[1] = {o + glm::normalize(dir)*scl, col};
                    m_api.submit_to(RenderTarget::Contour2D, s, Topology::LineList, DrawMode::VertexColor, col, mvp2);
                };
                if (m_overlays.show_T) arr({fr.T.x,fr.T.y,0},{1.f,0.35f,0.05f,1.f});
                if (m_overlays.show_N) arr({fr.N.x,fr.N.y,0},{0.15f,1.f,0.3f,1.f});
                if (m_overlays.show_osc && fr.kappa > 1e-5f) {
                    const f32 R = 1.f/fr.kappa;
                    constexpr u32 SEG = 64;
                    auto sc2 = m_api.acquire(SEG+1);
                    Vertex* vc = sc2.vertices();
                    const Vec3 ctr = {o3.x+fr.N.x*R, o3.y+fr.N.y*R, 0};
                    for (u32 k=0; k<=SEG; ++k) {
                        const f32 th = (static_cast<f32>(k)/SEG)*2.f*std::numbers::pi_v<f32>;
                        vc[k] = { ctr+Vec3{ops::cos(th)*R,ops::sin(th)*R,0}, {0.7f,0.3f,1.f,0.65f} };
                    }
                    m_api.submit_to(RenderTarget::Contour2D, sc2, Topology::LineStrip, DrawMode::VertexColor,
                                  {0.7f,0.3f,1.f,0.65f}, mvp2);
                }
                if (m_overlays.show_torsion && fr.kappa > 1e-5f && std::abs(fr.tau) > 1e-4f) {
                    const Vec4 tau_col = fr.tau > 0.f
                        ? Vec4{1.f, 0.55f, 0.1f, 0.9f}
                        : Vec4{0.3f, 0.8f, 1.f,  0.9f};
                    const Vec3 dBds_xy = { -fr.tau*fr.N.x, -fr.tau*fr.N.y, 0.f };
                    arr(dBds_xy, tau_col);
                    const f32 len = std::clamp(std::abs(fr.tau) * scl * 2.f, 0.02f, 1.5f);
                    const Vec3 tip_dir = glm::length(dBds_xy) > 1e-6f
                        ? glm::normalize(dBds_xy) : Vec3{0,1,0};
                    const Vec3 tip = o + tip_dir * len;
                    auto dot = m_api.acquire(1);
                    dot.vertices()[0] = { tip, tau_col };
                    m_api.submit_to(RenderTarget::Contour2D, dot, Topology::LineStrip, DrawMode::UniformColor, tau_col, mvp2);
                }
            }
        }
    }
}


// ── canvas_mvp_3d ─────────────────────────────────────────────────────────────

Mat4 SurfaceSimScene::canvas_mvp_3d(const ImVec2& cpos, const ImVec2& csz) const noexcept {
    const Vec2 sw_sz = m_api.viewport_size();
    const f32  sw    = sw_sz.x > 0.f ? sw_sz.x : 1.f;
    const f32  sh    = sw_sz.y > 0.f ? sw_sz.y : 1.f;
    const f32  cw    = csz.x   > 0.f ? csz.x   : 1.f;
    const f32  ch    = csz.y   > 0.f ? csz.y   : 1.f;
    const ImVec2 vp0 = ImGui::GetMainViewport()->Pos;
    const f32  cx    = cpos.x - vp0.x;
    const f32  cy    = cpos.y - vp0.y;

    const f32 sx = cw / sw;
    const f32 sy = ch / sh;
    const f32 bx =  2.f*cx/sw + sx - 1.f;
    const f32 by = -(2.f*cy/sh + sy - 1.f);

    Mat4 remap(0.f);
    remap[0][0] = sx;  remap[1][1] = sy;  remap[2][2] = 1.f;
    remap[3][0] = bx;  remap[3][1] = by;  remap[3][3] = 1.f;

    const f32  aspect = cw / ch;
    const f32  dist   = m_vp3d.base_extent / m_vp3d.zoom * 3.f;
    const f32  ex     = m_vp3d.pan_x + dist*ops::cos(m_vp3d.pitch)*ops::sin(m_vp3d.yaw);
    const f32  ey     = m_vp3d.pan_y + dist*ops::sin(m_vp3d.pitch);
    const f32  ez     =                dist*ops::cos(m_vp3d.pitch)*ops::cos(m_vp3d.yaw);
    const Mat4 proj   = glm::perspective(glm::radians(45.f), aspect, 0.01f, 500.f);
    const Mat4 view   = glm::lookAt(
        glm::vec3(ex, ey, ez),
        glm::vec3(m_vp3d.pan_x, m_vp3d.pan_y, 0.f),
        glm::vec3(0.f, 1.f, 0.f));
    return remap * proj * view;
}

} // namespace ndde

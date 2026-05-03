#include "app/SurfaceSimView.hpp"

#include "app/GaussianSurface.hpp"
#include "engine/CanvasInput.hpp"
#include "numeric/ops.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <glm/gtc/matrix_transform.hpp>
#include <numbers>

namespace ndde {

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

constexpr f32 k_levels[] = {
    -1.2f,-0.9f,-0.6f,-0.3f, 0.f, 0.3f, 0.6f, 0.9f, 1.2f
};
constexpr u32 k_n_levels = 9u;

} // namespace

SurfaceSimView::SurfaceSimView(EngineAPI& api)
    : m_api(api)
    , m_primitives(api)
    , m_particles(api)
{}

void SurfaceSimView::draw_surface_3d_window(SurfaceSimViewContext ctx) {
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

    ctx.vp3d.fb_w = csz.x;  ctx.vp3d.fb_h = csz.y;
    ctx.vp3d.dp_w = csz.x;  ctx.vp3d.dp_h = csz.y;
    ctx.hover.canvas3d_pos = cpos;
    ctx.hover.canvas3d_sz  = csz;

    if (canvas.hovered) {
        if (std::abs(canvas.mouse_wheel) > 0.f)
            ctx.vp3d.zoom = std::clamp(ctx.vp3d.zoom*(1.f + 0.12f*canvas.mouse_wheel), 0.05f, 20.f);
        if (canvas.orbit_drag)
            ctx.vp3d.orbit(canvas.mouse_delta.x, canvas.mouse_delta.y);
        update_hover(ctx, cpos, csz, true);
    } else {
        ctx.hover.snap_on_curve = false;
        ctx.hover.snap_idx      = -1;
    }

    const Mat4 mvp3d = canvas_mvp_3d(ctx.vp3d, cpos, csz);

    switch (ctx.display.mode) {
        case SurfaceDisplay::Wireframe: submit_wireframe_3d(ctx, mvp3d); break;
        case SurfaceDisplay::Filled:    submit_filled_3d(ctx, mvp3d);    break;
        case SurfaceDisplay::Both:
            submit_filled_3d(ctx, mvp3d);
            submit_wireframe_3d(ctx, mvp3d);
            break;
    }

    m_particles.show_frenet       = ctx.overlays.show_frenet;
    m_particles.show_T            = ctx.overlays.show_T;
    m_particles.show_N            = ctx.overlays.show_N;
    m_particles.show_B            = ctx.overlays.show_B;
    m_particles.show_osc          = ctx.overlays.show_osc;
    m_particles.show_dir_deriv    = ctx.overlays.show_dir_deriv;
    m_particles.show_normal_plane = ctx.overlays.show_normal_plane;
    m_particles.show_torsion      = ctx.overlays.show_torsion;
    m_particles.frame_scale       = ctx.display.frame_scale;
    m_particles.submit_all(ctx.curves, ctx.surface, ctx.sim_time,
                           mvp3d, ctx.hover.snap_curve, ctx.hover.snap_idx,
                           ctx.hover.snap_on_curve);

    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddText(ImVec2(cpos.x+8, cpos.y+6),
        IM_COL32(200,200,200,180),
        "Right-drag: orbit   Scroll: zoom   Ctrl+L: add particle");

    if (ctx.paused)
        dl->AddText(ImVec2(cpos.x+8, cpos.y+24),
            IM_COL32(255, 210, 60, 240), "PAUSED  [Ctrl+P to resume]");

    if (ctx.hover.snap_on_curve && ctx.hover.snap_idx >= 0 && ctx.hover.snap_curve >= 0 &&
        ctx.hover.snap_curve < static_cast<int>(ctx.curves.size()))
    {
        const FrenetFrame fr = ctx.curves[ctx.hover.snap_curve].frenet_at(
            static_cast<u32>(ctx.hover.snap_idx));
        char buf[128];
        std::snprintf(buf, sizeof(buf), "k=%.4f  t=%.4f  osc.r=%.3f",
            fr.kappa, fr.tau, fr.kappa>1e-5f ? 1.f/fr.kappa : 0.f);
        dl->AddText(ImVec2(cpos.x+8, cpos.y+22), IM_COL32(255,220,100,220), buf);
    }

    ImGui::End();
}

void SurfaceSimView::update_hover(SurfaceSimViewContext ctx,
                                  const ImVec2& canvas_pos,
                                  const ImVec2& canvas_size,
                                  bool is_3d)
{
    const ImGuiIO& io    = ImGui::GetIO();
    const f32      mx    = io.MousePos.x - canvas_pos.x;
    const f32      my    = io.MousePos.y - canvas_pos.y;
    constexpr f32  SNAP  = 20.f;

    ctx.hover.snap_idx      = -1;
    ctx.hover.snap_curve    = 0;
    ctx.hover.snap_on_curve = false;

    if (ctx.curves.empty()) return;

    if (is_3d) {
        const Mat4 mvp   = canvas_mvp_3d(ctx.vp3d, canvas_pos, canvas_size);
        const Vec2 sw_sz = m_api.viewport_size();
        const f32  sw    = sw_sz.x > 0.f ? sw_sz.x : 1.f;
        const f32  sh    = sw_sz.y > 0.f ? sw_sz.y : 1.f;
        f32 best = SNAP;
        for (u32 ci = 0; ci < static_cast<u32>(ctx.curves.size()); ++ci) {
            const AnimatedCurve& c = ctx.curves[ci];
            for (u32 i = 0; i < c.trail_size(); ++i) {
                const Vec3 wp = c.trail_pt(i);
                const glm::vec4 clip = mvp * glm::vec4(wp.x, wp.y, wp.z, 1.f);
                if (clip.w <= 0.f) continue;
                const f32 px = (clip.x/clip.w + 1.f) * 0.5f * sw - canvas_pos.x;
                const f32 py = (1.f - clip.y/clip.w) * 0.5f * sh - canvas_pos.y;
                const f32 d  = std::hypot(px - mx, py - my);
                if (d < best) { best = d; ctx.hover.snap_idx = static_cast<int>(i); ctx.hover.snap_curve = static_cast<int>(ci); }
            }
        }
    } else {
        f32 best = SNAP;
        for (u32 ci = 0; ci < static_cast<u32>(ctx.curves.size()); ++ci) {
            const AnimatedCurve& c = ctx.curves[ci];
            for (u32 i = 0; i < c.trail_size(); ++i) {
                const Vec3 wp = c.trail_pt(i);
                const Vec2 sp = ctx.vp2d.world_to_pixel(wp.x, wp.y);
                const f32  d  = std::hypot(sp.x-mx, sp.y-my);
                if (d < best) { best = d; ctx.hover.snap_idx = static_cast<int>(i); ctx.hover.snap_curve = static_cast<int>(ci); }
            }
        }
    }

    if (ctx.hover.snap_idx >= 0) ctx.hover.snap_on_curve = true;
}

void SurfaceSimView::submit_wireframe_3d(SurfaceSimViewContext ctx, const Mat4& mvp) {
    ctx.display.mesh.rebuild_if_needed(ctx.surface,
        surface_mesh_options(ctx.display.grid_lines, ctx.sim_time, ctx.display.curv_scale));
    if (ctx.display.mesh.wire_count() == 0) return;
    m_primitives.vertices(RenderTarget::Primary3D, ctx.display.mesh.wire_vertices(),
                          ctx.display.mesh.wire_count(), Topology::LineList,
                          DrawMode::VertexColor, {1,1,1,1}, mvp);
}

void SurfaceSimView::submit_filled_3d(SurfaceSimViewContext ctx, const Mat4& mvp) {
    ctx.display.mesh.rebuild_if_needed(ctx.surface,
        surface_mesh_options(ctx.display.grid_lines, ctx.sim_time, ctx.display.curv_scale));
    if (ctx.display.mesh.fill_count() == 0) return;
    m_primitives.vertices(RenderTarget::Primary3D, ctx.display.mesh.fill_vertices(),
                          ctx.display.mesh.fill_count(), Topology::TriangleList,
                          DrawMode::VertexColor, {1,1,1,1}, mvp);
}

void SurfaceSimView::submit_contour_second_window(SurfaceSimViewContext ctx) {
    const Vec2 sz2 = m_api.viewport_size2();
    if (sz2.x <= 0.f || sz2.y <= 0.f) return;

    const f32 domain = ctx.surface.u_max();
    const f32 aspect = sz2.x / sz2.y;
    const f32 half_y = domain / ctx.vp2d.zoom;
    const f32 half_x = half_y * aspect;
    const Mat4 mvp2  = glm::ortho(
        ctx.vp2d.pan_x - half_x, ctx.vp2d.pan_x + half_x,
        ctx.vp2d.pan_y - half_y, ctx.vp2d.pan_y + half_y,
        -10.f, 10.f);

    const auto* gs = dynamic_cast<const GaussianSurface*>(&ctx.surface);
    if (gs) {
        constexpr u32 NX=80, NY=80;
        m_primitives.generated(RenderTarget::Contour2D, NX*NY*6, Topology::TriangleList,
            DrawMode::VertexColor, {1,1,1,1}, mvp2,
            [gs](Vertex& out, u32 i) {
                const u32 cell = i / 6u;
                const u32 tri = i % 6u;
                const u32 ix = cell / NY;
                const u32 iy = cell % NY;
                const f32 dx = (gs->u_max()-gs->u_min())/NX;
                const f32 dy = (gs->v_max()-gs->v_min())/NY;
                const f32 x0=gs->u_min()+ix*dx, x1=x0+dx;
                const f32 y0=gs->v_min()+iy*dy, y1=y0+dy;
                const Vec3 p[6] = {
                    {x0,y0,0}, {x1,y0,0}, {x1,y1,0},
                    {x0,y0,0}, {x1,y1,0}, {x0,y1,0}
                };
                const Vec4 c[6] = {
                    GaussianSurface::height_color(GaussianSurface::eval(x0,y0)),
                    GaussianSurface::height_color(GaussianSurface::eval(x1,y0)),
                    GaussianSurface::height_color(GaussianSurface::eval(x1,y1)),
                    GaussianSurface::height_color(GaussianSurface::eval(x0,y0)),
                    GaussianSurface::height_color(GaussianSurface::eval(x1,y1)),
                    GaussianSurface::height_color(GaussianSurface::eval(x0,y1))
                };
                out = {p[tri], c[tri]};
            });

        if (ctx.display.show_contours) {
            const u32 max_v = GaussianSurface::contour_max_vertices(100u, k_n_levels);
            auto cslice = m_api.acquire(max_v);
            const u32 actual = GaussianSurface::tessellate_contours(
                {cslice.vertices(),max_v}, 100u, k_levels, k_n_levels, {1,1,1,0.8f});
            if (actual > 0) {
                cslice.vertex_count = actual;
                m_api.submit_to(RenderTarget::Contour2D, cslice, Topology::LineList,
                                DrawMode::VertexColor, {1,1,1,1}, mvp2);
            }
        }
    }

    for (const AnimatedCurve& c : ctx.curves) {
        const u32 nt = c.trail_size();
        if (nt < 2) continue;
        m_primitives.generated(RenderTarget::Contour2D, nt, Topology::LineStrip,
            DrawMode::VertexColor, {1,1,1,1}, mvp2,
            [&c, nt](Vertex& out, u32 i) {
                const Vec3 pt = c.trail_pt(i);
                const f32  t  = static_cast<f32>(i)/static_cast<f32>(nt-1);
                out = { Vec3{pt.x,pt.y,0},
                        AnimatedCurve::trail_colour(c.role(), c.colour_slot(), t) };
            });
    }

    const int ci = (ctx.hover.snap_on_curve && ctx.hover.snap_curve >= 0 &&
                    ctx.hover.snap_curve < static_cast<int>(ctx.curves.size()))
                   ? ctx.hover.snap_curve : 0;
    if (ci >= static_cast<int>(ctx.curves.size())) return;

    const AnimatedCurve& ac = ctx.curves[ci];
    const u32 nt = ac.trail_size();
    const u32 sidx = (ctx.hover.snap_on_curve && ctx.hover.snap_curve == ci &&
                      ctx.hover.snap_idx >= 0 && static_cast<u32>(ctx.hover.snap_idx) < nt)
        ? static_cast<u32>(ctx.hover.snap_idx)
        : (nt > 2 ? nt-2 : 0);
    if (nt < 3 || sidx == 0) return;

    const FrenetFrame fr = ac.frenet_at(sidx);
    const Vec3 o3 = ac.trail_pt(sidx);
    const Vec3 o  = {o3.x, o3.y, 0};
    const f32  scl = ctx.display.frame_scale * 0.4f;
    auto arr = [&](Vec3 dir, Vec4 col) {
        if (glm::length(dir) < 1e-5f) return;
        m_primitives.line(RenderTarget::Contour2D, o, o + glm::normalize(dir)*scl, col, mvp2);
    };
    if (ctx.overlays.show_T) arr({fr.T.x,fr.T.y,0},{1.f,0.35f,0.05f,1.f});
    if (ctx.overlays.show_N) arr({fr.N.x,fr.N.y,0},{0.15f,1.f,0.3f,1.f});
    if (ctx.overlays.show_osc && fr.kappa > 1e-5f) {
        const f32 R = 1.f/fr.kappa;
        constexpr u32 SEG = 64;
        const Vec3 ctr = {o3.x+fr.N.x*R, o3.y+fr.N.y*R, 0};
        m_primitives.generated(RenderTarget::Contour2D, SEG+1, Topology::LineStrip,
            DrawMode::VertexColor, {0.7f,0.3f,1.f,0.65f}, mvp2,
            [=](Vertex& out, u32 k) {
                const f32 th = (static_cast<f32>(k)/SEG)*2.f*std::numbers::pi_v<f32>;
                out = { ctr+Vec3{ops::cos(th)*R,ops::sin(th)*R,0}, {0.7f,0.3f,1.f,0.65f} };
            });
    }
    if (ctx.overlays.show_torsion && fr.kappa > 1e-5f && std::abs(fr.tau) > 1e-4f) {
        const Vec4 tau_col = fr.tau > 0.f
            ? Vec4{1.f, 0.55f, 0.1f, 0.9f}
            : Vec4{0.3f, 0.8f, 1.f,  0.9f};
        const Vec3 dBds_xy = { -fr.tau*fr.N.x, -fr.tau*fr.N.y, 0.f };
        arr(dBds_xy, tau_col);
        const f32 len = std::clamp(std::abs(fr.tau) * scl * 2.f, 0.02f, 1.5f);
        const Vec3 tip_dir = glm::length(dBds_xy) > 1e-6f
            ? glm::normalize(dBds_xy) : Vec3{0,1,0};
        m_primitives.point(RenderTarget::Contour2D, o + tip_dir * len, tau_col, mvp2);
    }
}

Mat4 SurfaceSimView::canvas_mvp_3d(const Viewport& vp3d,
                                   const ImVec2& cpos,
                                   const ImVec2& csz) const noexcept {
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
    const f32  dist   = vp3d.base_extent / vp3d.zoom * 3.f;
    const f32  ex     = vp3d.pan_x + dist*ops::cos(vp3d.pitch)*ops::sin(vp3d.yaw);
    const f32  ey     = vp3d.pan_y + dist*ops::sin(vp3d.pitch);
    const f32  ez     =              dist*ops::cos(vp3d.pitch)*ops::cos(vp3d.yaw);
    const Mat4 proj   = glm::perspective(glm::radians(45.f), aspect, 0.01f, 500.f);
    const Mat4 view   = glm::lookAt(
        glm::vec3(ex, ey, ez),
        glm::vec3(vp3d.pan_x, vp3d.pan_y, 0.f),
        glm::vec3(0.f, 1.f, 0.f));
    return remap * proj * view;
}

} // namespace ndde

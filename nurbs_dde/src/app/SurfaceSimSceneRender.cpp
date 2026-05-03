#include "app/SurfaceSimScene.hpp"

namespace ndde {

SurfaceSimViewContext SurfaceSimScene::view_context() noexcept {
    return SurfaceSimViewContext{
        .surface = *m_surface,
        .curves = m_curves,
        .vp3d = m_vp3d,
        .vp2d = m_vp2d,
        .display = m_display,
        .overlays = m_overlays,
        .hover = m_hover_state,
        .sim_time = m_sim_time,
        .paused = m_paused
    };
}

void SurfaceSimScene::draw_surface_3d_window() {
    m_view.draw_surface_3d_window(view_context());
}

void SurfaceSimScene::draw_contour_2d_window() {
    // Not called from on_frame() -- second window handles 2D rendering.
}

void SurfaceSimScene::submit_contour_second_window() {
    m_view.submit_contour_second_window(view_context());
}

} // namespace ndde

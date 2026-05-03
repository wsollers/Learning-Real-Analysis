#pragma once
// app/SurfaceSimView.hpp
// Scene-domain surface visualization built on engine primitive submission.

#include "app/AnimatedCurve.hpp"
#include "app/ParticleRenderer.hpp"
#include "app/SurfaceSimSceneState.hpp"
#include "app/Viewport.hpp"
#include "engine/EngineAPI.hpp"
#include "engine/PrimitiveRenderer.hpp"
#include "math/Surfaces.hpp"

#include <imgui.h>
#include <vector>

namespace ndde {

struct SurfaceSimViewContext {
    ndde::math::ISurface& surface;
    std::vector<AnimatedCurve>& curves;
    Viewport& vp3d;
    Viewport& vp2d;
    SurfaceDisplayState& display;
    SurfaceOverlayState& overlays;
    SurfaceHoverState& hover;
    float sim_time = 0.f;
    bool paused = false;
};

class SurfaceSimView {
public:
    explicit SurfaceSimView(EngineAPI& api);

    void draw_surface_3d_window(SurfaceSimViewContext ctx);
    void submit_contour_second_window(SurfaceSimViewContext ctx);

private:
    EngineAPI& m_api;
    PrimitiveRenderer m_primitives;
    ParticleRenderer m_particles;

    [[nodiscard]] Mat4 canvas_mvp_3d(const Viewport& vp3d,
                                      const ImVec2& cpos,
                                      const ImVec2& csz) const noexcept;

    void update_hover(SurfaceSimViewContext ctx,
                      const ImVec2& canvas_pos,
                      const ImVec2& canvas_size,
                      bool is_3d);
    void submit_wireframe_3d(SurfaceSimViewContext ctx, const Mat4& mvp);
    void submit_filled_3d(SurfaceSimViewContext ctx, const Mat4& mvp);
};

} // namespace ndde

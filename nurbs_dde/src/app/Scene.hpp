#pragma once
// app/Scene.hpp

#include "engine/EngineAPI.hpp"
#include "engine/AppConfig.hpp"
#include "math/Axes.hpp"
#include "math/Conics.hpp"
#include "app/AnalysisPanel.hpp"
#include "app/HoverResult.hpp"
#include "app/Viewport.hpp"
#include "app/CoordDebugPanel.hpp"

#include <imgui.h>
#include <memory>
#include <vector>
#include <string>
#include <utility>

namespace ndde {

struct ConicEntry {
    std::string                          name;
    bool                                 enabled      = true;
    Vec4                                 color        = { 1.f, 1.f, 1.f, 1.f };
    u32                                  tessellation = 256;
    std::unique_ptr<math::IConic>        conic;
    int                                  type         = 0;

    float par_a = -1.f, par_b = 0.f, par_c = 0.f;
    float par_tmin = -2.f, par_tmax = 2.f;

    float hyp_a = 1.f, hyp_b = 1.f, hyp_h = 0.f, hyp_k = 0.f, hyp_range = 2.f;
    bool  hyp_two_branch = true;
    int   hyp_axis       = 0;

    bool needs_rebuild = true;
    void mark_dirty()  { needs_rebuild = true; }

    std::vector<std::pair<float, float>> snap_cache;
    void rebuild();
};

class Scene {
public:
    explicit Scene(EngineAPI api);
    ~Scene() = default;

    void on_frame();

private:
    EngineAPI               m_api;
    Viewport                m_vp;
    math::AxesConfig        m_axes_cfg;
    AnalysisPanel           m_analysis_panel;
    HoverResult             m_hover;
    CoordDebugPanel         m_coord_debug;
    std::vector<ConicEntry> m_conics;

    void sync_viewport();
    void update_camera();
    void update_hover();

    void submit_grid();
    void submit_axes();
    void submit_conics();
    void submit_epsilon_ball();
    void submit_interval_lines();
    void submit_interval_labels();   ///< ImGui text labels on the axes
    void submit_secant_line();
    void submit_tangent_line();

    void draw_main_panel();
    void draw_conic_panel(ConicEntry& e, int idx);

    void add_parabola();
    void add_hyperbola();
};

} // namespace ndde

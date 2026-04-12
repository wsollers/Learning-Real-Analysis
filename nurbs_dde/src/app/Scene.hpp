#pragma once
// app/Scene.hpp
// Application frame logic. The only engine type Scene sees is EngineAPI.
// No raw Vulkan anywhere in this file.
// See docs/ADDING_A_MATH_COMPONENT.md to add new geometry.

#include "engine/EngineAPI.hpp"
#include "engine/AppConfig.hpp"
#include "math/Axes.hpp"
#include "math/Conics.hpp"
#include "app/AnalysisPanel.hpp"
#include "app/HoverResult.hpp"

#include <imgui.h>
#include <memory>
#include <vector>
#include <string>
#include <glm/gtc/matrix_transform.hpp>

namespace ndde {

struct ConicEntry {
    std::string               name;
    bool                      enabled      = true;
    Vec4                      color        = { 1.f, 1.f, 1.f, 1.f };
    u32                       tessellation = 256;
    std::unique_ptr<math::IConic> conic;
    int                       type         = 0; // 0=Parabola, 1=Hyperbola

    float par_a = 1.f, par_b = 0.f, par_c = 0.f;
    float par_tmin = -1.5f, par_tmax = 1.5f;

    float hyp_a = 1.f, hyp_b = 1.f, hyp_h = 0.f, hyp_k = 0.f, hyp_range = 2.f;
    bool  hyp_two_branch = true;
    int   hyp_axis       = 0;

    bool needs_rebuild = true;
    void mark_dirty()  { needs_rebuild = true; }
    void rebuild();
};

class Scene {
public:
    explicit Scene(EngineAPI api);
    ~Scene() = default;

    void on_frame();

private:
    EngineAPI             m_api;
    math::AxesConfig      m_axes_cfg;
    AnalysisPanel         m_analysis_panel;
    HoverResult           m_hover;
    std::vector<ConicEntry> m_conics;

    void draw_main_panel();
    void draw_conic_panel(ConicEntry& entry, int idx);

    void submit_axes();
    void submit_grid();
    void submit_conics();

    [[nodiscard]] Mat4 ortho_mvp() const noexcept;

    void add_parabola();
    void add_hyperbola();
};

} // namespace ndde

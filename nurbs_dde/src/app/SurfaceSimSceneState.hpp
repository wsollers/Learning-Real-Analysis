#pragma once
// app/SurfaceSimSceneState.hpp
// Named state groups for the legacy SurfaceSimScene service scene.

#include "app/GaussianRipple.hpp"
#include "app/HoverResult.hpp"
#include "app/SurfaceMeshCache.hpp"
#include "math/ExtremumTable.hpp"
#include "sim/BiasedBrownianLeader.hpp"
#include "sim/BrownianMotion.hpp"
#include "sim/DelayPursuitEquation.hpp"
#include "sim/LeaderSeekerEquation.hpp"

#include <imgui.h>

namespace ndde {

enum class SurfaceType : u8 { Gaussian = 0, Torus = 1, GaussianRipple = 2, Extremum = 3 };
enum class SurfaceDisplay : u8 { Wireframe = 0, Filled = 1, Both = 2 };
enum class LeaderMode : u8 { Deterministic = 0, StochasticBiased = 1 };
enum class PursuitMode : u8 { Direct = 0, Delayed = 1, Momentum = 2 };

struct SurfaceSelectionState {
    SurfaceType type = SurfaceType::Gaussian;
    f32 torus_R = 2.0f;
    f32 torus_r = 0.7f;
    GaussianRipple::Params ripple_params;
};

struct SurfaceDisplayState {
    f32 frame_scale = 0.25f;
    u32 grid_lines = 64;
    bool show_contours = true;
    SurfaceDisplay mode = SurfaceDisplay::Wireframe;
    f32 curv_scale = 2.f;
    SurfaceMeshCache mesh;
};

struct SurfaceOverlayState {
    bool show_frenet = true;
    bool show_dir_deriv = false;
    bool show_normal_plane = false;
    bool show_torsion = false;
    bool show_T = true;
    bool show_N = true;
    bool show_B = true;
    bool show_osc = true;
};

struct SurfaceUiState {
    bool debug_open = false;
    bool hotkey_panel_open = false;
};

struct SurfaceSpawnState {
    u32 leader_count = 0;
    u32 chaser_count = 0;
    u32 delay_pursuit_count = 0;
    bool spawning_pursuer = false;
};

struct SurfaceBehaviorParams {
    ndde::sim::BrownianMotion::Params brownian;
    ndde::sim::DelayPursuitEquation::Params delay_pursuit;
};

struct SurfacePursuitState {
    bool pair_collision = false;
    float min_dist = 0.3f;

    ndde::math::ExtremumTable extremum_table;
    u32 extremum_rebuild_countdown = 0;

    LeaderMode leader_mode = LeaderMode::Deterministic;
    ndde::sim::LeaderSeekerEquation::Params leader_seeker;
    ndde::sim::BiasedBrownianLeader::Params biased_brownian_leader;

    PursuitMode pursuit_mode = PursuitMode::Direct;
    float pursuit_tau = 1.5f;
    float pursuit_window = 1.5f;
};

struct SurfaceHoverState {
    ImVec2 canvas3d_pos{};
    ImVec2 canvas3d_sz{};
    int snap_curve = 0;
    int snap_idx = -1;
    bool snap_on_curve = false;
    HoverResult hover;
};

} // namespace ndde

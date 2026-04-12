#pragma once
// engine/EngineAPI.hpp
// The narrow contract between Engine and Scene.
// Scene sees only this struct — no raw Vulkan, no subsystem headers.

#include "memory/ArenaSlice.hpp"
#include "engine/AppConfig.hpp"
#include "math/Types.hpp"
#include <functional>

struct ImFont;

namespace ndde {

struct EngineAPI {
    // Allocate vertex_count Vertex slots from the per-frame arena.
    std::function<memory::ArenaSlice(u32 vertex_count)> acquire;

    // Submit a populated slice for rendering this frame.
    std::function<void(const memory::ArenaSlice& slice,
                       Topology  topology,
                       DrawMode  mode,
                       Vec4      color,
                       Mat4      mvp)> submit;

    // STIX math font handles. Guard: if (font) ImGui::PushFont(font);
    std::function<ImFont*()> math_font_body;
    std::function<ImFont*()> math_font_small;

    // Read-only access to the loaded config. Lifetime owned by Engine.
    std::function<const AppConfig&()> config;

    // Live window dimensions — updated on every resize.
    // Use this (not config().window) for aspect ratio calculations.
    std::function<Vec2()> viewport_size;
};

} // namespace ndde

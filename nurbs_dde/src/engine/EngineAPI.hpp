#pragma once
// engine/EngineAPI.hpp
// The narrow contract between Engine and Scene.
// Scene sees only this struct — no raw Vulkan, no subsystem headers.
//
// All functions must be called from the render thread.
// The Engine wires these lambdas in Engine::make_api() during start().

#include "memory/ArenaSlice.hpp"
#include "engine/AppConfig.hpp"
#include "math/Types.hpp"
#include <functional>

struct ImFont;

namespace ndde {

struct EngineAPI {
    // Allocate vertex_count Vertex slots from the per-frame arena.
    // Throws on overflow — increase arena_size_mb in engine_config.json.
    std::function<memory::ArenaSlice(u32 vertex_count)> acquire;

    // Submit a populated slice for rendering this frame.
    std::function<void(const memory::ArenaSlice& slice,
                       Topology  topology,
                       DrawMode  mode,
                       Vec4      color,
                       Mat4      mvp)> submit;

    // STIX math font handles. May be nullptr if font file was not found.
    // Always guard: if (font) ImGui::PushFont(font);
    std::function<ImFont*()> math_font_body;
    std::function<ImFont*()> math_font_small;

    // Read-only access to the loaded config. Lifetime owned by Engine.
    std::function<const AppConfig&()> config;
};

} // namespace ndde

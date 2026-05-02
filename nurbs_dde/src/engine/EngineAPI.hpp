#pragma once
// engine/EngineAPI.hpp
// The narrow contract between Engine and Scene.
// Scene sees only this struct — no raw Vulkan, no subsystem headers.

#include "memory/ArenaSlice.hpp"
#include "engine/AppConfig.hpp"
#include "math/Scalars.hpp"
#include <functional>
#include <string_view>

struct ImFont;

namespace ndde {

// ── Debug statistics ──────────────────────────────────────────────────────────
// Populated once per frame by Engine and made available to Scene via EngineAPI.
// No Vulkan types — safe to include from any translation unit.

struct DebugStats {
    // ── Arena (BufferManager) ─────────────────────────────────────────────────
    u64 arena_bytes_used  = 0;   ///< bytes written this frame
    u64 arena_bytes_total = 0;   ///< capacity of the arena
    f32 arena_utilisation = 0.f; ///< used / total  [0, 1]
    u64 arena_vertex_count = 0;  ///< total vertices submitted this frame

    // ── Renderer ──────────────────────────────────────────────────────────────
    u32 draw_calls   = 0;   ///< vkCmdDraw invocations this frame
    u32 swapchain_w  = 0;   ///< current swapchain extent
    u32 swapchain_h  = 0;

    // ── Timing ────────────────────────────────────────────────────────────────
    f32 frame_ms     = 0.f; ///< last frame wall-clock time in milliseconds
    f32 fps          = 0.f; ///< 1000 / frame_ms
};

// ── EngineAPI ─────────────────────────────────────────────────────────────────

struct EngineAPI {
    // Allocate vertex_count Vertex slots from the per-frame arena.
    std::function<memory::ArenaSlice(u32 vertex_count)> acquire;

    // Submit a populated slice to a named render target.
    // Known targets: "3d" (primary window), "contour" (second window).
    // Submitting to an unknown target or a closed window is a no-op.
    std::function<void(std::string_view             target,
                       const memory::ArenaSlice&    slice,
                       Topology                     topology,
                       DrawMode                     mode,
                       Vec4                         color,
                       Mat4                         mvp)> submit_to;

    // Dimensions of each window's framebuffer.
    std::function<Vec2()> viewport_size;   ///< primary window (3D)
    std::function<Vec2()> viewport_size2;  ///< second  window (2D)

    // STIX math font handles. Guard: if (font) ImGui::PushFont(font);
    std::function<ImFont*()> math_font_body;
    std::function<ImFont*()> math_font_small;

    // Read-only access to the loaded config. Lifetime owned by Engine.
    std::function<const AppConfig&()> config;

    // Per-frame debug statistics — arena, renderer, timing.
    // Populated by Engine just before Scene::on_frame() is called.
    std::function<const DebugStats&()> debug_stats;
};

} // namespace ndde

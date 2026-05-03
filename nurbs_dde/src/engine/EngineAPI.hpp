#pragma once
// engine/EngineAPI.hpp
// The narrow contract between Engine and Scene.
// Scene sees only this struct — no raw Vulkan, no subsystem headers.

#include "memory/ArenaSlice.hpp"
#include "engine/AppConfig.hpp"
#include "math/Scalars.hpp"
#include <cstddef>
#include <functional>

namespace ndde {

enum class RenderTarget : u8 {
    Primary3D,
    Contour2D
};

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

    // Submit a populated slice to a render target.
    // Contour2D submissions are ignored if the second window is unavailable.
    std::function<void(RenderTarget                 target,
                       const memory::ArenaSlice&    slice,
                       Topology                     topology,
                       DrawMode                     mode,
                       Vec4                         color,
                       Mat4                         mvp)> submit_to;

    // Dimensions of each window's framebuffer.
    std::function<Vec2()> viewport_size;   ///< primary window (3D)
    std::function<Vec2()> viewport_size2;  ///< second  window (2D)

    // Push/pop the STIX math font for the current ImGui scope.
    // push_math_font(false) -> body size  push_math_font(true) -> small size
    // Always pair every push with a pop before the frame ends.
    // No-op if the font asset failed to load (safe to call unconditionally).
    std::function<void(bool small)> push_math_font;
    std::function<void()>           pop_math_font;

    // Read-only access to the loaded config. Lifetime owned by Engine.
    std::function<const AppConfig&()> config;

    // Per-frame debug statistics — arena, renderer, timing.
    // Populated by Engine just before Scene::on_frame() is called.
    std::function<const DebugStats&()> debug_stats;

    // Request a first-class simulation switch by registry index.
    // Index 0 is Ctrl+1, index 1 is Ctrl+2, etc.
    std::function<void(std::size_t)> switch_simulation;
};

} // namespace ndde

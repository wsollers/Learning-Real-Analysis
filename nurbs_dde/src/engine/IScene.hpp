#pragma once
// engine/IScene.hpp
// IScene: the minimal contract between Engine and any scene.
//
// Design
// ──────
// Engine knows nothing about surfaces, particles, panels, or simulation.
// It knows only that a scene can:
//   - advance and render itself each frame  (on_frame)
//   - be notified when the swapchain resizes (on_resize)
//   - report a human-readable name          (name)
//
// Panels, hotkeys, and UI layout are the scene's private concern.
// The engine does not iterate panels or manage windows.
//
// Lifetime contract
// ─────────────────
// Engine owns the active scene via unique_ptr<IScene>.
// Switching scenes: scenes request a switch through EngineAPI. Engine defers
// the actual replacement until the current frame has fully ended. All Vulkan
// work is flushed before destruction.
//
// EngineAPI is passed at construction time (not through the interface) so
// scenes are free to choose their own constructor signature.  The factory
// pattern (make_surface_sim_scene, make_analysis_scene, ...) gives Engine
// type-erased construction without coupling it to scene headers.

#include "math/Scalars.hpp"  // f32
#include <string_view>

namespace ndde {

class IScene {
public:
    virtual ~IScene() = default;

    // Called once per frame after ImGui::NewFrame() and before
    // imgui_render(). dt is wall-clock seconds since the previous frame.
    // The scene is responsible for all ImGui window calls this frame.
    virtual void on_frame(f32 dt) = 0;

    // Called when the primary swapchain has been recreated.
    // The scene should rebuild any projection matrices or viewport state
    // that depends on framebuffer dimensions.
    // Default: no-op (most scenes recompute projections every frame anyway).
    virtual void on_resize(u32 /*w*/, u32 /*h*/) {}

    // Short human-readable identifier used in the scene selector UI.
    [[nodiscard]] virtual std::string_view name() const = 0;
};

} // namespace ndde

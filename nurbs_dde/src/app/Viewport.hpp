#pragma once
// app/Viewport.hpp
// Single source of truth for all coordinate transforms.
//
// Two pixel spaces:
//   fb_w/h  = framebuffer pixels  (GPU). Source: EngineAPI::viewport_size().
//             Used for: MVP aspect ratio.
//   dp_w/h  = display/logical pixels (ImGui). Source: ImGui::GetIO().DisplaySize.
//             Used for: MousePos, all input.
//
// On standard displays these are equal.
// On HiDPI/scaled displays they differ by the DPI scale factor.
//
// ImGui sentinel: when the mouse is outside the window, ImGui sets
// MousePos to (-FLT_MAX, -FLT_MAX).  All functions that take pixel
// coordinates must guard against this.  Use mouse_valid() before
// calling pixel_to_world / zoom_toward / pan_by_pixels.

#include "math/Scalars.hpp"
#include <imgui.h>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>
#include <limits>

namespace ndde {

struct Viewport {
    float pan_x      = 0.f;
    float pan_y      = 0.f;
    float zoom       = 1.f;
    float base_extent = 2.4f;

    float yaw   = 0.f;
    float pitch = 0.3f;

    static constexpr float k_zoom_min    = 0.05f;
    static constexpr float k_zoom_max    = 50.f;
    static constexpr float k_zoom_step   = 0.12f;
    static constexpr float k_orbit_speed = 0.008f;

    float fb_w = 1280.f;
    float fb_h =  720.f;
    float dp_w = 1280.f;
    float dp_h =  720.f;

    // ── Sanity guard ──────────────────────────────────────────────────────────
    // ImGui sets MousePos to (-FLT_MAX, -FLT_MAX) when the mouse is outside
    // the application window or unavailable.  Any pixel coordinate with a
    // magnitude > half the maximum float is considered invalid.
    [[nodiscard]] static bool mouse_valid(float lx, float ly) noexcept {
        constexpr float k_limit = 1e15f;  // well below FLT_MAX ≈ 3.4e38
        return lx > -k_limit && lx < k_limit &&
               ly > -k_limit && ly < k_limit;
    }

    // ── Derived ───────────────────────────────────────────────────────────────

    [[nodiscard]] float fb_aspect() const noexcept { return (fb_h > 0.f) ? fb_w / fb_h : 1.f; }
    [[nodiscard]] float half_h()    const noexcept { return base_extent / zoom; }
    [[nodiscard]] float half_w()    const noexcept { return half_h() * fb_aspect(); }

    // ── World bounds ──────────────────────────────────────────────────────────

    [[nodiscard]] float left()   const noexcept { return pan_x - half_w(); }
    [[nodiscard]] float right()  const noexcept { return pan_x + half_w(); }
    [[nodiscard]] float bottom() const noexcept { return pan_y - half_h(); }
    [[nodiscard]] float top()    const noexcept { return pan_y + half_h(); }

    // ── MVP ───────────────────────────────────────────────────────────────────

    [[nodiscard]] Mat4 ortho_mvp() const noexcept {
        return glm::ortho(left(), right(), bottom(), top(), -10.f, 10.f);
    }

    [[nodiscard]] Mat4 perspective_mvp() const noexcept {
        const float dist = base_extent / zoom * 3.f;
        const float cx = pan_x + dist * std::cos(pitch) * std::sin(yaw);
        const float cy = pan_y + dist * std::sin(pitch);
        const float cz =         dist * std::cos(pitch) * std::cos(yaw);
        const Mat4 proj = glm::perspective(glm::radians(45.f), fb_aspect(), 0.01f, 500.f);
        const Mat4 view = glm::lookAt(
            glm::vec3(cx, cy, cz),
            glm::vec3(pan_x, pan_y, 0.f),
            glm::vec3(0.f, 1.f, 0.f));
        return proj * view;
    }

    // ── Coordinate transforms ─────────────────────────────────────────────────
    // MousePos from ImGui::GetIO() is always window-relative (ViewportsEnable
    // is disabled). No screen-to-window conversion needed.

    // Passthrough: kept for call-site compatibility after the viewports revert.
    [[nodiscard]] static Vec2 screen_to_window(float sx, float sy) noexcept {
        return { sx, sy };
    }

    // pixel_to_world: lx/ly are window-relative logical pixels.
    [[nodiscard]] Vec2 pixel_to_world(float lx, float ly) const noexcept {
        if (dp_w <= 0.f || dp_h <= 0.f) return {};
        if (!mouse_valid(lx, ly))        return {};  // guard against ImGui sentinel
        const float nx = lx / dp_w;
        const float ny = ly / dp_h;
        return {
            left()  + nx * (right()  - left()),
            top()   + ny * (bottom() - top())
        };
    }

    [[nodiscard]] Vec2 world_to_pixel(float wx, float wy) const noexcept {
        if (dp_w <= 0.f || dp_h <= 0.f) return {};
        return {
            ((wx - left())  / (right()  - left())) * dp_w,
            ((wy - top())   / (bottom() - top()))  * dp_h
        };
    }

    // ── Camera input ──────────────────────────────────────────────────────────

    void zoom_toward(float lx, float ly, float ticks) noexcept {
        if (!mouse_valid(lx, ly)) return;
        const Vec2 before = pixel_to_world(lx, ly);
        zoom = std::clamp(zoom * (1.f + k_zoom_step * ticks), k_zoom_min, k_zoom_max);
        const Vec2 after  = pixel_to_world(lx, ly);
        pan_x -= after.x - before.x;
        pan_y -= after.y - before.y;
    }

    void pan_by_pixels(float dpx, float dpy) noexcept {
        if (dp_w <= 0.f || dp_h <= 0.f) return;
        const float wx_per_px = (right() - left()) / dp_w;
        const float wy_per_px = (top() - bottom()) / dp_h;
        pan_x += dpx * wx_per_px;
        pan_y -= dpy * wy_per_px;
    }

    void orbit(float dpx, float dpy) noexcept {
        yaw   += dpx * k_orbit_speed;
        pitch  = std::clamp(pitch + dpy * k_orbit_speed, -1.5f, 1.5f);
    }

    void reset() noexcept {
        pan_x = 0.f; pan_y = 0.f; zoom = 1.f;
        yaw   = 0.f; pitch = 0.3f;
    }
};

} // namespace ndde

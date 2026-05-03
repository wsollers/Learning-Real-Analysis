#pragma once
// engine/RenderService.hpp
// Renderer-neutral render view and packet queue service.

#include "engine/ServiceHandle.hpp"
#include "math/GeometryTypes.hpp"
#include "math/Scalars.hpp"

#include <algorithm>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace ndde {

using RenderViewId = u64;
using RenderViewHandle = ServiceHandle;

enum class RenderViewKind : u8 {
    Main,
    Alternate
};

enum class AlternateViewMode : u8 {
    Contour,
    LevelCurves,
    VectorField,
    Isoclines,
    Flow
};

enum class VectorFieldMode : u8 {
    Gradient,
    NegativeGradient,
    LevelTangent,
    ParticleVelocity
};

enum class CameraProjection : u8 {
    Perspective,
    Orthographic
};

struct CameraState {
    Vec3 target{0.f, 0.f, 0.f};
    f32 yaw = 0.72f;
    f32 pitch = 0.55f;
    f32 zoom = 1.f;
};

enum class CameraPreset : u8 {
    Home,
    Top,
    Front,
    Side
};

struct ViewOverlayState {
    bool show_axes = false;
    bool show_grid = false;
    bool show_frame = false;
    bool show_labels = false;
    bool show_hover_frenet = true;
    bool show_osculating_circle = true;
};

struct AlternateViewSettings {
    f32 isocline_direction_angle = 0.f;
    f32 isocline_target_slope = 0.f;
    f32 isocline_tolerance = 0.5f;
    u32 isocline_bands = 5u;

    VectorFieldMode vector_mode = VectorFieldMode::NegativeGradient;
    u32 vector_samples = 18u;
    f32 vector_scale = 1.f;

    u32 flow_seed_count = 9u;
    u32 flow_steps = 36u;
    f32 flow_step_size = 0.14f;
};

struct RenderViewDescriptor {
    std::string title;
    RenderViewKind kind = RenderViewKind::Main;
    AlternateViewMode alternate_mode = AlternateViewMode::Contour;
    CameraProjection projection = CameraProjection::Perspective;
    f32 viewport_aspect = 16.f / 9.f;
    Vec2 viewport_size{16.f, 9.f};
    CameraState camera{};
    ViewOverlayState overlays{};
    AlternateViewSettings alternate{};
};

struct RenderViewDomain {
    f32 u_min = -1.f;
    f32 u_max = 1.f;
    f32 v_min = -1.f;
    f32 v_max = 1.f;
    f32 z_min = -1.f;
    f32 z_max = 1.f;
};

struct SurfacePerturbCommand {
    RenderViewId view = 0;
    Vec2 uv{};
    Vec2 screen_ndc{};
    f32 viewport_aspect = 16.f / 9.f;
    f32 amplitude = 0.35f;
    f32 radius = 0.9f;
    f32 falloff = 1.f;
    u32 seed = 0;
    bool use_ray_pick = false;
};

struct RenderViewSnapshot {
    RenderViewId id = 0;
    std::string title;
    RenderViewKind kind = RenderViewKind::Main;
    AlternateViewMode alternate_mode = AlternateViewMode::Contour;
    CameraProjection projection = CameraProjection::Perspective;
    f32 viewport_aspect = 16.f / 9.f;
    Vec2 viewport_size{16.f, 9.f};
    CameraState camera{};
    ViewOverlayState overlays{};
    AlternateViewSettings alternate{};
    RenderViewDomain domain{};
};

struct RenderPacket {
    RenderViewId view = 0;
    Topology topology = Topology::LineList;
    DrawMode mode = DrawMode::VertexColor;
    Vec4 color{1.f, 1.f, 1.f, 1.f};
    Mat4 mvp{1.f};
    std::vector<Vertex> vertices;
};

class RenderService {
public:
    [[nodiscard]] RenderViewHandle register_view(RenderViewDescriptor descriptor, RenderViewId* out_id = nullptr) {
        const RenderViewId id = m_next_id++;
        m_views.push_back(RenderViewEntry{
            .id = id,
            .descriptor = std::move(descriptor),
            .active = true
        });
        if (out_id) *out_id = id;
        return RenderViewHandle([this, id] { unregister(id); });
    }

    void submit(RenderViewId view, std::span<const Vertex> vertices,
                Topology topology, DrawMode mode, Vec4 color, Mat4 mvp)
    {
        if (!is_active(view) || vertices.empty()) return;
        RenderPacket packet;
        packet.view = view;
        packet.topology = topology;
        packet.mode = mode;
        packet.color = color;
        packet.mvp = mvp;
        packet.vertices.assign(vertices.begin(), vertices.end());
        m_packets.push_back(std::move(packet));
    }

    void clear_packets() noexcept { m_packets.clear(); }

    void set_view_domain(RenderViewId id, RenderViewDomain domain) noexcept {
        if (auto* entry = find_active_entry(id))
            entry->domain = domain;
    }

    [[nodiscard]] RenderViewDomain view_domain(RenderViewId id) const noexcept {
        if (const auto* entry = find_active_entry(id))
            return entry->domain;
        return {};
    }

    [[nodiscard]] RenderViewDescriptor* descriptor(RenderViewId id) noexcept {
        if (auto* entry = find_active_entry(id))
            return &entry->descriptor;
        return nullptr;
    }

    [[nodiscard]] const RenderViewDescriptor* descriptor(RenderViewId id) const noexcept {
        if (const auto* entry = find_active_entry(id))
            return &entry->descriptor;
        return nullptr;
    }

    [[nodiscard]] RenderViewId first_active_main_view() const noexcept {
        for (const auto& entry : m_views) {
            if (entry.active && entry.descriptor.kind == RenderViewKind::Main)
                return entry.id;
        }
        return 0;
    }

    void orbit_main_cameras(f32 dx, f32 dy) noexcept {
        for (auto& entry : m_views) {
            if (!entry.active || entry.descriptor.kind != RenderViewKind::Main) continue;
            auto& camera = entry.descriptor.camera;
            camera.yaw += dx * 0.0065f;
            camera.pitch = std::clamp(camera.pitch + dy * 0.0065f, -1.35f, 1.35f);
        }
    }

    void zoom_main_cameras(f32 wheel_delta) noexcept {
        if (wheel_delta == 0.f) return;
        for (auto& entry : m_views) {
            if (!entry.active || entry.descriptor.kind != RenderViewKind::Main) continue;
            auto& camera = entry.descriptor.camera;
            camera.zoom = std::clamp(camera.zoom * (1.f + wheel_delta * 0.12f), 0.08f, 30.f);
        }
    }

    void pan_main_cameras(f32 dx, f32 dy) noexcept {
        for (auto& entry : m_views) {
            if (!entry.active || entry.descriptor.kind != RenderViewKind::Main) continue;
            const auto domain = entry.domain;
            const f32 scale = std::max(domain.u_max - domain.u_min, domain.v_max - domain.v_min)
                / std::max(entry.descriptor.camera.zoom, 0.05f);
            entry.descriptor.camera.target.x -= dx * scale * 0.0012f;
            entry.descriptor.camera.target.y += dy * scale * 0.0012f;
        }
    }

    void set_axes_visible(bool visible) noexcept {
        for (auto& entry : m_views) {
            if (entry.active)
                entry.descriptor.overlays.show_axes = visible;
        }
    }

    void set_main_view_aspect(f32 aspect) noexcept {
        if (aspect <= 0.f) return;
        for (auto& entry : m_views) {
            if (entry.active && entry.descriptor.kind == RenderViewKind::Main)
                entry.descriptor.viewport_aspect = aspect;
        }
    }

    void set_viewport_size(RenderViewId id, Vec2 size) noexcept {
        if (size.x <= 0.f || size.y <= 0.f) return;
        if (auto* entry = find_active_entry(id)) {
            entry->descriptor.viewport_size = size;
            entry->descriptor.viewport_aspect = size.x / size.y;
        }
    }

    void set_viewport_size(RenderViewKind kind, Vec2 size) noexcept {
        if (size.x <= 0.f || size.y <= 0.f) return;
        for (auto& entry : m_views) {
            if (entry.active && entry.descriptor.kind == kind) {
                entry.descriptor.viewport_size = size;
                entry.descriptor.viewport_aspect = size.x / size.y;
            }
        }
    }

    void reset_main_cameras(CameraPreset preset = CameraPreset::Home) noexcept {
        for (auto& entry : m_views) {
            if (!entry.active || entry.descriptor.kind != RenderViewKind::Main) continue;
            entry.descriptor.camera = preset_camera(preset, entry.domain);
        }
    }

    [[nodiscard]] bool axes_visible() const noexcept {
        for (const auto& entry : m_views) {
            if (entry.active && entry.descriptor.overlays.show_axes)
                return true;
        }
        return false;
    }

    void queue_surface_perturbation(SurfacePerturbCommand command) {
        if (command.view == 0)
            command.view = first_active_main_view();
        if (command.view == 0) return;
        m_surface_commands.push_back(command);
    }

    [[nodiscard]] std::vector<SurfacePerturbCommand> consume_surface_perturbations(RenderViewId view) {
        std::vector<SurfacePerturbCommand> out;
        auto it = m_surface_commands.begin();
        while (it != m_surface_commands.end()) {
            if (it->view == view) {
                out.push_back(*it);
                it = m_surface_commands.erase(it);
            } else {
                ++it;
            }
        }
        return out;
    }

    [[nodiscard]] std::size_t active_view_count() const noexcept {
        return static_cast<std::size_t>(std::count_if(m_views.begin(), m_views.end(),
            [](const RenderViewEntry& entry) { return entry.active; }));
    }

    [[nodiscard]] std::size_t packet_count(RenderViewId view) const noexcept {
        return static_cast<std::size_t>(std::count_if(m_packets.begin(), m_packets.end(),
            [view](const RenderPacket& packet) { return packet.view == view; }));
    }

    [[nodiscard]] bool contains_view(std::string_view title) const {
        return std::any_of(m_views.begin(), m_views.end(),
            [title](const RenderViewEntry& entry) {
                return entry.active && entry.descriptor.title == title;
            });
    }

    [[nodiscard]] RenderViewKind view_kind(RenderViewId id) const noexcept {
        for (const auto& entry : m_views) {
            if (entry.active && entry.id == id)
                return entry.descriptor.kind;
        }
        return RenderViewKind::Main;
    }

    [[nodiscard]] const std::vector<RenderPacket>& packets() const noexcept { return m_packets; }

    [[nodiscard]] std::vector<RenderViewSnapshot> active_view_snapshots() const {
        std::vector<RenderViewSnapshot> out;
        out.reserve(m_views.size());
        for (const auto& entry : m_views) {
            if (!entry.active) continue;
            out.push_back(RenderViewSnapshot{
                .id = entry.id,
                .title = entry.descriptor.title,
                .kind = entry.descriptor.kind,
                .alternate_mode = entry.descriptor.alternate_mode,
                .projection = entry.descriptor.projection,
                .viewport_aspect = entry.descriptor.viewport_aspect,
                .viewport_size = entry.descriptor.viewport_size,
                .camera = entry.descriptor.camera,
                .overlays = entry.descriptor.overlays,
                .alternate = entry.descriptor.alternate,
                .domain = entry.domain
            });
        }
        return out;
    }

private:
    struct RenderViewEntry {
        RenderViewId id = 0;
        RenderViewDescriptor descriptor;
        RenderViewDomain domain;
        bool active = false;
    };

    RenderViewId m_next_id = 1;
    std::vector<RenderViewEntry> m_views;
    std::vector<RenderPacket> m_packets;
    std::vector<SurfacePerturbCommand> m_surface_commands;

    [[nodiscard]] static CameraState preset_camera(CameraPreset preset, RenderViewDomain domain) noexcept {
        const Vec3 target{
            (domain.u_min + domain.u_max) * 0.5f,
            (domain.v_min + domain.v_max) * 0.5f,
            (domain.z_min + domain.z_max) * 0.5f
        };
        switch (preset) {
            case CameraPreset::Top:
                return CameraState{.target = target, .yaw = 0.f, .pitch = 1.35f, .zoom = 1.f};
            case CameraPreset::Front:
                return CameraState{.target = target, .yaw = 1.5707963f, .pitch = 0.10f, .zoom = 1.f};
            case CameraPreset::Side:
                return CameraState{.target = target, .yaw = 0.f, .pitch = 0.10f, .zoom = 1.f};
            case CameraPreset::Home:
            default:
                return CameraState{.target = target, .yaw = 0.72f, .pitch = 0.55f, .zoom = 1.f};
        }
    }

    [[nodiscard]] bool is_active(RenderViewId id) const noexcept {
        return std::any_of(m_views.begin(), m_views.end(),
            [id](const RenderViewEntry& entry) { return entry.active && entry.id == id; });
    }

    [[nodiscard]] RenderViewEntry* find_active_entry(RenderViewId id) noexcept {
        for (auto& entry : m_views) {
            if (entry.active && entry.id == id)
                return &entry;
        }
        return nullptr;
    }

    [[nodiscard]] const RenderViewEntry* find_active_entry(RenderViewId id) const noexcept {
        for (const auto& entry : m_views) {
            if (entry.active && entry.id == id)
                return &entry;
        }
        return nullptr;
    }

    void unregister(RenderViewId id) noexcept {
        for (auto& entry : m_views) {
            if (entry.id == id) {
                entry.active = false;
                return;
            }
        }
    }
};

} // namespace ndde

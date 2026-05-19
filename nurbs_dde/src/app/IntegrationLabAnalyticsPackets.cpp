#include "app/IntegrationLabAnalyticsPackets.hpp"

#include <algorithm>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>

namespace ndde {
namespace {

[[nodiscard]] Vec4 axis_color() noexcept { return Vec4{0.56f, 0.58f, 0.64f, 0.65f}; }
[[nodiscard]] Vec4 convergence_color() noexcept { return Vec4{0.28f, 0.78f, 1.00f, 0.95f}; }
[[nodiscard]] Vec4 comparison_midpoint_color() noexcept { return Vec4{1.00f, 0.70f, 0.22f, 0.90f}; }
[[nodiscard]] Vec4 comparison_trapezoid_color() noexcept { return Vec4{0.34f, 0.84f, 0.48f, 0.90f}; }

void push_line(memory::FrameVector<Vertex>& out, Vec2 a, Vec2 b, Vec4 color) {
    out.push_back(Vertex{.pos = Vec3{a.x, a.y, 0.f}, .color = color});
    out.push_back(Vertex{.pos = Vec3{b.x, b.y, 0.f}, .color = color});
}

void push_quad(memory::FrameVector<Vertex>& out, Vec2 min, Vec2 max, Vec4 color) {
    const Vec3 p00{min.x, min.y, 0.f};
    const Vec3 p10{max.x, min.y, 0.f};
    const Vec3 p11{max.x, max.y, 0.f};
    const Vec3 p01{min.x, max.y, 0.f};
    out.push_back(Vertex{.pos = p00, .color = color});
    out.push_back(Vertex{.pos = p10, .color = color});
    out.push_back(Vertex{.pos = p11, .color = color});
    out.push_back(Vertex{.pos = p00, .color = color});
    out.push_back(Vertex{.pos = p11, .color = color});
    out.push_back(Vertex{.pos = p01, .color = color});
}

[[nodiscard]] f32 normalized_error(f64 error, f64 max_error) noexcept {
    if (max_error <= f64(0)) return 0.f;
    const f64 scaled = std::log10(error + f64(1.0e-16)) -
                       std::log10(max_error + f64(1.0e-16));
    return static_cast<f32>(std::clamp((scaled + f64(8)) / f64(8), f64(0), f64(1)));
}

} // namespace

void append_integration_convergence_plot(memory::FrameVector<Vertex>& out,
                                         const IntegrationAnalysisSnapshot& analysis) {
    push_line(out, Vec2{-0.92f, -0.82f}, Vec2{0.92f, -0.82f}, axis_color());
    push_line(out, Vec2{-0.92f, -0.82f}, Vec2{-0.92f, 0.82f}, axis_color());

    const auto& rows = analysis.convergence.rows;
    if (rows.size() < 2u) return;

    f64 max_error = f64(0);
    for (const auto& row : rows) {
        if (row.absolute_error) {
            max_error = std::max(max_error, *row.absolute_error);
        }
    }
    if (max_error <= f64(0)) return;

    Vec2 previous{};
    bool have_previous = false;
    for (std::size_t i = 0; i < rows.size(); ++i) {
        if (!rows[i].absolute_error) continue;
        const f32 x = -0.82f + static_cast<f32>(i) *
            (1.62f / static_cast<f32>(std::max<std::size_t>(rows.size() - 1u, 1u)));
        const f32 y = -0.72f + normalized_error(*rows[i].absolute_error, max_error) * 1.44f;
        const Vec2 current{x, y};
        if (have_previous) {
            push_line(out, previous, current, convergence_color());
        }
        push_line(out, Vec2{x - 0.018f, y}, Vec2{x + 0.018f, y}, convergence_color());
        push_line(out, Vec2{x, y - 0.018f}, Vec2{x, y + 0.018f}, convergence_color());
        previous = current;
        have_previous = true;
    }
}

void append_integration_method_comparison(memory::FrameVector<Vertex>& out,
                                          const IntegrationAnalysisSnapshot& analysis) {
    if (analysis.method_comparison.empty()) return;

    f64 max_abs_error = f64(0);
    for (const auto& row : analysis.method_comparison) {
        if (row.absolute_error) {
            max_abs_error = std::max(max_abs_error, *row.absolute_error);
        }
    }
    if (max_abs_error <= f64(0)) max_abs_error = f64(1);

    const f32 base_y = -0.92f;
    const f32 left = -0.72f;
    const f32 width = 0.36f;
    for (std::size_t i = 0; i < analysis.method_comparison.size(); ++i) {
        const auto& row = analysis.method_comparison[i];
        const f32 height = row.absolute_error
            ? std::max(0.04f, normalized_error(*row.absolute_error, max_abs_error) * 0.62f)
            : 0.04f;
        const f32 x0 = left + static_cast<f32>(i) * 0.52f;
        const Vec4 color = row.method == math::integration::IntegrationMethod2D::Midpoint
            ? comparison_midpoint_color()
            : comparison_trapezoid_color();
        push_quad(out, Vec2{x0, base_y}, Vec2{x0 + width, base_y + height}, color);
    }
}

IntegrationLabAnalyticsRenderStats submit_integration_analytics_packets(
    RenderService& render,
    RenderViewId view,
    const IntegrationAnalysisSnapshot& analysis,
    memory::MemoryService* memory) {
    if (view == RenderViewId(0)) return {};

    memory::FrameVector<Vertex> convergence =
        memory ? memory->frame().make_vector<Vertex>() : memory::FrameVector<Vertex>{};
    memory::FrameVector<Vertex> comparison =
        memory ? memory->frame().make_vector<Vertex>() : memory::FrameVector<Vertex>{};

    append_integration_convergence_plot(convergence, analysis);
    append_integration_method_comparison(comparison, analysis);

    render.set_view_domain(view, RenderViewDomain{
        .u_min = -1.f,
        .u_max = 1.f,
        .v_min = -1.f,
        .v_max = 1.f,
        .z_min = -1.f,
        .z_max = 1.f
    });
    const Mat4 mvp = glm::ortho(-1.f, 1.f, -1.f, 1.f, -1.f, 1.f);
    render.submit(view, convergence, Topology::LineList, DrawMode::VertexColor, convergence_color(), mvp);
    render.submit(view, comparison, Topology::TriangleList, DrawMode::VertexColor, comparison_midpoint_color(), mvp);

    return IntegrationLabAnalyticsRenderStats{
        .convergence_vertices = static_cast<u64>(convergence.size()),
        .comparison_vertices = static_cast<u64>(comparison.size())
    };
}

} // namespace ndde

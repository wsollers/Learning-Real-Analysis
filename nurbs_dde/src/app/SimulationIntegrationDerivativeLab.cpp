#include "app/SimulationIntegrationDerivativeLab.hpp"

#include "engine/RenderService.hpp"
#include "math/GeometryTypes.hpp"
#include "numeric/ops.hpp"

#include <algorithm>
#include <cmath>
#include <format>
#include <imgui.h>
#include <glm/gtc/matrix_transform.hpp>

namespace ndde {

namespace {

constexpr f64 k_pi = f64(3.14159265358979323846264338327950288);

[[nodiscard]] Vec4 graph_color() noexcept { return Vec4{0.20f, 0.76f, 1.00f, 1.0f}; }
[[nodiscard]] Vec4 cell_color() noexcept { return Vec4{0.35f, 0.38f, 0.43f, 0.65f}; }
[[nodiscard]] Vec4 axis_color() noexcept { return Vec4{0.72f, 0.72f, 0.76f, 0.7f}; }
[[nodiscard]] Vec4 tangent_color() noexcept { return Vec4{1.0f, 0.36f, 0.24f, 1.0f}; }

void push_line(memory::FrameVector<Vertex>& vertices, Vec2 a, Vec2 b, Vec4 color) {
    vertices.push_back(Vertex{.pos = Vec3{a.x, a.y, 0.f}, .color = color});
    vertices.push_back(Vertex{.pos = Vec3{b.x, b.y, 0.f}, .color = color});
}

} // namespace

SimulationIntegrationDerivativeLab::SimulationIntegrationDerivativeLab(memory::MemoryService* memory)
    : m_memory(memory)
    , m_presets{{
        FunctionPreset{
            .name = "sin(x)",
            .formula = "f(x) = sin(x)",
            .function = [](f64 x) { return std::sin(x); },
            .antiderivative = [](f64 x) { return -std::cos(x); },
            .interval = {.min = f64(0), .max = k_pi}
        },
        FunctionPreset{
            .name = "x^2",
            .formula = "f(x) = x^2",
            .function = [](f64 x) { return x * x; },
            .antiderivative = [](f64 x) { return (x * x * x) / f64(3); },
            .interval = {.min = f64(-1), .max = f64(2)}
        },
        FunctionPreset{
            .name = "x^3 - x",
            .formula = "f(x) = x^3 - x",
            .function = [](f64 x) { return (x * x * x) - x; },
            .antiderivative = [](f64 x) { return (x * x * x * x) / f64(4) - (x * x) / f64(2); },
            .interval = {.min = f64(-1.5), .max = f64(1.5)}
        },
        FunctionPreset{
            .name = "Runge",
            .formula = "f(x) = 1 / (1 + 25x^2)",
            .function = [](f64 x) { return f64(1) / (f64(1) + f64(25) * x * x); },
            .antiderivative = [](f64 x) { return std::atan(f64(5) * x) / f64(5); },
            .interval = {.min = f64(-1), .max = f64(1)}
        }
    }}
{}

void SimulationIntegrationDerivativeLab::on_register(SimulationHost& host) {
    m_host = &host;
    m_memory = &host.memory();

    m_panel_handles.add(host.panels().register_panel(PanelDescriptor{
        .title = "Lab - Integration",
        .category = "Mathematics",
        .scope = PanelScope::Simulation,
        .draw = [this] { draw_control_panel(); }
    }));

    m_main_handle = host.render().register_view(RenderViewDescriptor{
        .title = "Integration & Derivative Lab",
        .kind = RenderViewKind::Main,
        .projection = CameraProjection::Orthographic,
        .camera_profile = CameraViewProfile::Orthographic2D,
        .overlays = {.show_axes = false, .show_grid = false}
    }, &m_main_view);
}

void SimulationIntegrationDerivativeLab::on_start() {
    const auto& current = preset();
    m_probe_x = (current.interval.min + current.interval.max) * f64(0.5);
    recompute();
}

void SimulationIntegrationDerivativeLab::on_tick(const TickInfo& tick) {
    m_time = tick.time;
    if (!tick.paused) {
        const auto& current = preset();
        const f64 span = current.interval.length();
        const f64 phase = std::fmod(static_cast<f64>(tick.time) * f64(0.15), f64(1));
        m_probe_x = current.interval.min + phase * span;
        recompute();
    }
}

void SimulationIntegrationDerivativeLab::on_submit_render() {
    submit_geometry();
}

void SimulationIntegrationDerivativeLab::on_stop() {
    m_panel_handles.clear();
    m_main_handle.reset();
    m_host = nullptr;
}

SceneSnapshot SimulationIntegrationDerivativeLab::snapshot() const {
    return SceneSnapshot{
        .name = std::string(name()),
        .paused = false,
        .sim_time = m_time,
        .sim_speed = f32(1),
        .particle_count = 0u,
        .status = m_status
    };
}

SimulationMetadata SimulationIntegrationDerivativeLab::metadata() const {
    return SimulationMetadata{
        .name = std::string(name()),
        .surface_name = "1D interval",
        .surface_formula = preset().formula,
        .status = m_status,
        .sim_time = m_time,
        .sim_speed = f32(1),
        .particle_count = 0u,
        .surface_has_analytic_derivatives = true
    };
}

f64 SimulationIntegrationDerivativeLab::reference_value() const {
    const auto& current = preset();
    return current.antiderivative(current.interval.max) - current.antiderivative(current.interval.min);
}

void SimulationIntegrationDerivativeLab::recompute() {
    const auto& current = preset();
    auto result = math::integration::integrate_uniform(
        current.interval,
        current.function,
        m_partition,
        m_method,
        reference_value());
    if (!result) {
        m_status = math::integration::describe_error(result.error());
        return;
    }
    m_result = std::move(*result);
    m_probe_x = std::clamp(m_probe_x, current.interval.min, current.interval.max);
    m_probe_derivative = math::integration::central_difference(current.function, m_probe_x);
    m_status = "Ready";
}

void SimulationIntegrationDerivativeLab::draw_control_panel() {
    ImGui::SetNextWindowPos(ImVec2(24.f, 64.f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(360.f, 360.f), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Lab - Integration")) {
        ImGui::End();
        return;
    }

    bool changed = false;
    ImGui::SeparatorText("Problem");
    const char* function_names[] = {"sin(x)", "x^2", "x^3 - x", "Runge"};
    int function_index = static_cast<int>(m_preset_index);
    if (ImGui::Combo("Function", &function_index, function_names, IM_ARRAYSIZE(function_names))) {
        m_preset_index = static_cast<std::size_t>(function_index);
        const auto& current = preset();
        m_probe_x = (current.interval.min + current.interval.max) * f64(0.5);
        changed = true;
    }

    const char* method_names[] = {"Left", "Right", "Midpoint", "Trapezoid", "Simpson"};
    int method_index = static_cast<int>(m_method);
    if (ImGui::Combo("Method", &method_index, method_names, IM_ARRAYSIZE(method_names))) {
        m_method = static_cast<math::integration::QuadratureMethod>(method_index);
        if (m_method == math::integration::QuadratureMethod::Simpson &&
            (m_partition.cell_count % u32(2)) != u32(0)) {
            ++m_partition.cell_count;
        }
        changed = true;
    }

    int cells = static_cast<int>(m_partition.cell_count);
    if (ImGui::SliderInt("Cells", &cells, 2, 256)) {
        m_partition.cell_count = static_cast<u32>(std::max(cells, 2));
        if (m_method == math::integration::QuadratureMethod::Simpson &&
            (m_partition.cell_count % u32(2)) != u32(0)) {
            ++m_partition.cell_count;
        }
        changed = true;
    }

    const auto& current = preset();
    float probe = static_cast<float>(m_probe_x);
    if (ImGui::SliderFloat("Probe x", &probe,
                           static_cast<float>(current.interval.min),
                           static_cast<float>(current.interval.max))) {
        m_probe_x = static_cast<f64>(probe);
        changed = true;
    }
    changed |= ImGui::Checkbox("Cells", &m_show_cells);
    ImGui::SameLine();
    changed |= ImGui::Checkbox("Tangent", &m_show_tangent);

    if (changed) {
        recompute();
    }

    ImGui::SeparatorText("Result");
    ImGui::TextUnformatted(current.formula);
    ImGui::Text("Method: %s", math::integration::method_name(m_method).data());
    ImGui::Text("Estimate: %.12f", m_result.estimate);
    ImGui::Text("Reference: %.12f", reference_value());
    ImGui::Text("Abs error: %.6e", m_result.absolute_error.value_or(f64(0)));
    ImGui::Text("Evaluations: %llu", static_cast<unsigned long long>(m_result.evaluation_count));
    ImGui::SeparatorText("Derivative Probe");
    ImGui::Text("x: %.6f", m_probe_x);
    ImGui::Text("f(x): %.6f", current.function(m_probe_x));
    ImGui::Text("f'(x): %.6f", m_probe_derivative);
    ImGui::TextDisabled("%s", m_status.c_str());

    ImGui::End();
}

void SimulationIntegrationDerivativeLab::submit_geometry() {
    if (!m_host || m_main_view == RenderViewId(0)) return;

    const auto& current = preset();
    const u32 samples = u32(240);
    memory::FrameVector<Vertex> graph =
        m_memory ? m_memory->frame().make_vector<Vertex>() : memory::FrameVector<Vertex>{};
    graph.reserve(samples + u32(1));

    f64 y_min = f64(0);
    f64 y_max = f64(0);
    for (u32 i = u32(0); i <= samples; ++i) {
        const f64 alpha = static_cast<f64>(i) / static_cast<f64>(samples);
        const f64 x = current.interval.min + alpha * current.interval.length();
        const f64 y = current.function(x);
        y_min = std::min(y_min, y);
        y_max = std::max(y_max, y);
        graph.push_back(Vertex{
            .pos = Vec3{static_cast<f32>(x), static_cast<f32>(y), 0.f},
            .color = graph_color()
        });
    }

    for (const auto& contribution : m_result.contributions) {
        y_min = std::min(y_min, contribution.value);
        y_max = std::max(y_max, contribution.value);
    }
    const f64 y_span = std::max(y_max - y_min, f64(0.5));
    const f64 y_pad = y_span * f64(0.18);
    const f32 left = static_cast<f32>(current.interval.min);
    const f32 right = static_cast<f32>(current.interval.max);
    const f32 bottom = static_cast<f32>(y_min - y_pad);
    const f32 top = static_cast<f32>(y_max + y_pad);
    const Mat4 mvp = glm::ortho(left, right, bottom, top, -1.f, 1.f);

    m_host->render().set_view_domain(m_main_view, RenderViewDomain{
        .u_min = left,
        .u_max = right,
        .v_min = bottom,
        .v_max = top,
        .z_min = -1.f,
        .z_max = 1.f
    });

    memory::FrameVector<Vertex> axes =
        m_memory ? m_memory->frame().make_vector<Vertex>() : memory::FrameVector<Vertex>{};
    push_line(axes, Vec2{left, 0.f}, Vec2{right, 0.f}, axis_color());
    push_line(axes, Vec2{0.f, bottom}, Vec2{0.f, top}, axis_color());
    m_host->render().submit(m_main_view, axes, Topology::LineList, DrawMode::VertexColor, axis_color(), mvp);

    if (m_show_cells && !m_result.contributions.empty()) {
        memory::FrameVector<Vertex> cells =
            m_memory ? m_memory->frame().make_vector<Vertex>() : memory::FrameVector<Vertex>{};
        cells.reserve(m_result.contributions.size() * 8u);
        for (const auto& contribution : m_result.contributions) {
            const f32 a = static_cast<f32>(contribution.cell.a);
            const f32 b = static_cast<f32>(contribution.cell.b);
            const f32 y = static_cast<f32>(contribution.value);
            push_line(cells, Vec2{a, 0.f}, Vec2{a, y}, cell_color());
            push_line(cells, Vec2{a, y}, Vec2{b, y}, cell_color());
            push_line(cells, Vec2{b, y}, Vec2{b, 0.f}, cell_color());
            push_line(cells, Vec2{a, 0.f}, Vec2{b, 0.f}, cell_color());
        }
        m_host->render().submit(m_main_view, cells, Topology::LineList, DrawMode::VertexColor, cell_color(), mvp);
    }

    m_host->render().submit(m_main_view, graph, Topology::LineStrip, DrawMode::VertexColor, graph_color(), mvp);

    if (m_show_tangent) {
        const f64 y = current.function(m_probe_x);
        const f64 half_width = current.interval.length() * f64(0.08);
        memory::FrameVector<Vertex> tangent =
            m_memory ? m_memory->frame().make_vector<Vertex>() : memory::FrameVector<Vertex>{};
        tangent.reserve(4u);
        push_line(tangent,
                  Vec2{static_cast<f32>(m_probe_x - half_width),
                       static_cast<f32>(y - m_probe_derivative * half_width)},
                  Vec2{static_cast<f32>(m_probe_x + half_width),
                       static_cast<f32>(y + m_probe_derivative * half_width)},
                  tangent_color());
        push_line(tangent,
                  Vec2{static_cast<f32>(m_probe_x), bottom},
                  Vec2{static_cast<f32>(m_probe_x), top},
                  Vec4{1.f, 0.9f, 0.35f, 0.55f});
        m_host->render().submit(m_main_view, tangent, Topology::LineList, DrawMode::VertexColor, tangent_color(), mvp);
    }
}

} // namespace ndde

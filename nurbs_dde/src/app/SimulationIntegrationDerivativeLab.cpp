#include "app/SimulationIntegrationDerivativeLab.hpp"

#include "app/IntegrationLabAnalyticsPackets.hpp"
#include "app/IntegrationLabRenderPackets.hpp"
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

[[nodiscard]] const char* integrand_preset_label(math::integration::IntegrandPreset2D preset) noexcept {
    switch (preset) {
        case math::integration::IntegrandPreset2D::Gaussian: return "Gaussian";
        case math::integration::IntegrandPreset2D::Wave: return "Wave";
        case math::integration::IntegrandPreset2D::Polynomial: return "Polynomial";
        case math::integration::IntegrandPreset2D::StepX: return "Step X";
    }
    return "Unknown";
}

[[nodiscard]] const char* method_2d_label(math::integration::IntegrationMethod2D method) noexcept {
    switch (method) {
        case math::integration::IntegrationMethod2D::Midpoint: return "Midpoint";
        case math::integration::IntegrationMethod2D::TensorProductTrapezoid: return "Trapezoid";
    }
    return "Unknown";
}

[[nodiscard]] const char* display_mode_label(IntegrationDisplayMode mode) noexcept {
    switch (mode) {
        case IntegrationDisplayMode::Value: return "Value";
        case IntegrationDisplayMode::Contribution: return "Contribution";
        case IntegrationDisplayMode::LocalError: return "Local Error";
    }
    return "Unknown";
}

[[nodiscard]] Vec2 view_point_from_ndc(RenderViewDomain domain, Vec2 screen_ndc) noexcept {
    const f32 x_t = (screen_ndc.x + f32(1)) * f32(0.5);
    const f32 y_t = (screen_ndc.y + f32(1)) * f32(0.5);
    return Vec2{
        domain.u_min + (domain.u_max - domain.u_min) * x_t,
        domain.v_min + (domain.v_max - domain.v_min) * y_t
    };
}

[[nodiscard]] bool same_view_point(Vec2 a, Vec2 b) noexcept {
    constexpr f32 eps = 1.0e-5f;
    return std::abs(a.x - b.x) <= eps && std::abs(a.y - b.y) <= eps;
}

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

    const auto snapshot = m_workbench.snapshot();
    host.render().set_view_domain(m_main_view, RenderViewDomain{
        .u_min = static_cast<f32>(snapshot.problem.domain.x_min),
        .u_max = static_cast<f32>(snapshot.problem.domain.x_max),
        .v_min = static_cast<f32>(snapshot.problem.domain.y_min),
        .v_max = static_cast<f32>(snapshot.problem.domain.y_max),
        .z_min = -1.f,
        .z_max = 1.f
    });

    m_analytics_handle = host.render().register_view(RenderViewDescriptor{
        .title = "Integration Analytics",
        .kind = RenderViewKind::Alternate,
        .projection = CameraProjection::Orthographic,
        .camera_profile = CameraViewProfile::Orthographic2D,
        .overlays = {.show_axes = false, .show_grid = false}
    }, &m_analytics_view);
}

void SimulationIntegrationDerivativeLab::on_start() {
    const auto& current = preset();
    m_probe_x = (current.interval.min + current.interval.max) * f64(0.5);
    recompute();
}

void SimulationIntegrationDerivativeLab::on_tick(const TickInfo& tick) {
    m_time = tick.time;
    if (!m_show_1d_bridge) {
        handle_2d_workbench_interaction();
    }
    if (!tick.paused) {
        const auto& current = preset();
        const f64 span = current.interval.length();
        const f64 phase = std::fmod(static_cast<f64>(tick.time) * f64(0.15), f64(1));
        m_probe_x = current.interval.min + phase * span;
        recompute();
    }
    submit_geometry();
}

void SimulationIntegrationDerivativeLab::handle_2d_workbench_interaction() {
    if (!m_host || m_main_view == RenderViewId(0)) return;

    const RenderViewDomain domain = m_host->render().view_domain(m_main_view);
    const ViewMouseState mouse = m_host->interaction().mouse_state(m_main_view);
    if (mouse.enabled) {
        const Vec2 point = view_point_from_ndc(domain, mouse.ndc);
        const auto hovered = m_workbench.hover_cell_at(point.x, point.y);
        if (hovered) {
            m_host->interaction().set_hover_view_point(m_main_view, point, Vec3{point.x, point.y, 0.f});
        }
    } else {
        m_workbench.hover_cell(std::nullopt);
    }

    auto picks = m_host->interaction().consume_view_point_picks(m_main_view);
    for (const ViewPointPickRequest& pick : picks) {
        const Vec2 point = view_point_from_ndc(domain, pick.screen_ndc);
        if (m_workbench.select_cell_at(point.x, point.y)) {
            m_host->interaction().select_view_point(m_main_view, point, Vec3{point.x, point.y, 0.f});
            m_last_selected_view_point = point;
        }
    }

    const InteractionTarget selected = m_host->interaction().selected_target(m_main_view);
    if (selected.kind == InteractionTargetKind::ViewPoint2D && selected.valid) {
        const Vec2 point = selected.point2d;
        if (!m_last_selected_view_point || !same_view_point(*m_last_selected_view_point, point)) {
            (void)m_workbench.select_cell_at(point.x, point.y);
            m_last_selected_view_point = point;
        }
    }
}

void SimulationIntegrationDerivativeLab::on_submit_render() {
    submit_geometry();
}

void SimulationIntegrationDerivativeLab::on_stop() {
    m_panel_handles.clear();
    m_main_handle.reset();
    m_analytics_handle.reset();
    m_analytics_view = RenderViewId(0);
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
    const auto snapshot = m_workbench.snapshot();
    return SimulationMetadata{
        .name = std::string(name()),
        .surface_name = "2D rectangle domain",
        .surface_formula = std::string(math::integration::integrand_name(snapshot.problem.integrand_preset)),
        .status = m_status,
        .sim_time = m_time,
        .sim_speed = f32(1),
        .particle_count = static_cast<std::size_t>(snapshot.result.cell_count),
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
    ImGui::SetNextWindowSize(ImVec2(440.f, 560.f), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Lab - Integration")) {
        ImGui::End();
        return;
    }

    draw_2d_workbench_panel();
    ImGui::Separator();
    ImGui::Checkbox("Show 1D bridge", &m_show_1d_bridge);
    if (m_show_1d_bridge) {
        draw_1d_bridge_panel();
    }

    ImGui::End();
}

void SimulationIntegrationDerivativeLab::draw_2d_workbench_panel() {
    auto snapshot = m_workbench.snapshot();

    ImGui::SeparatorText("2D Integration Workbench");
    ImGui::TextDisabled("Integral over D: f(x, y) dA");

    if (ImGui::BeginTabBar("IntegrationWorkbenchTabs")) {
        if (ImGui::BeginTabItem("Problem")) {
            draw_2d_problem_tab();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Partition")) {
            draw_2d_partition_tab();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Display")) {
            draw_2d_display_tab();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Analysis")) {
            draw_2d_analysis_tab();
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    snapshot = m_workbench.snapshot();

    ImGui::SeparatorText("Problem");
    ImGui::Text("Domain: [%.2f, %.2f] x [%.2f, %.2f]",
                snapshot.problem.domain.x_min,
                snapshot.problem.domain.x_max,
                snapshot.problem.domain.y_min,
                snapshot.problem.domain.y_max);
    ImGui::Text("Integrand: %s", integrand_preset_label(snapshot.problem.integrand_preset));
    ImGui::Text("Formula: %s", math::integration::integrand_name(snapshot.problem.integrand_preset).data());
    ImGui::Text("Measure: dA");
    ImGui::Text("Partition: %u x %u = %llu cells",
                snapshot.problem.grid.x_cells,
                snapshot.problem.grid.y_cells,
                static_cast<unsigned long long>(snapshot.result.cell_count));
    ImGui::Text("Method: %s", method_2d_label(snapshot.problem.method));
    ImGui::Text("Display: %s", display_mode_label(snapshot.renderable.display.mode));

    ImGui::SeparatorText("Live Result");
    ImGui::Text("Estimate: %.12f", snapshot.result.estimate);
    if (snapshot.result.reference_value) {
        ImGui::Text("Reference: %.12f", *snapshot.result.reference_value);
    } else {
        ImGui::TextDisabled("Reference: none");
    }
    if (snapshot.result.absolute_error) {
        ImGui::Text("Abs error: %.6e", *snapshot.result.absolute_error);
    }
    if (snapshot.result.relative_error) {
        ImGui::Text("Rel error: %.6e", *snapshot.result.relative_error);
    }
    ImGui::Text("Evaluations: %llu",
                static_cast<unsigned long long>(snapshot.result.evaluation_count));

    ImGui::SeparatorText("Selected Cell");
    if (snapshot.selected_cell.valid) {
        const auto& selected = snapshot.selected_cell.contribution;
        ImGui::Text("id %u  grid (%u, %u)",
                    selected.cell.id,
                    selected.cell.ix,
                    selected.cell.iy);
        ImGui::Text("region x[%.4f, %.4f] y[%.4f, %.4f]",
                    selected.cell.x0,
                    selected.cell.x1,
                    selected.cell.y0,
                    selected.cell.y1);
        ImGui::Text("sample (%.4f, %.4f)",
                    static_cast<double>(selected.sample.x),
                    static_cast<double>(selected.sample.y));
        ImGui::Text("measure %.8f", selected.measure);
        ImGui::Text("f(sample) %.8f", selected.value);
        ImGui::Text("contribution %.8f", selected.contribution);
        ImGui::Text("local error est. %.8e", selected.local_error_estimate);
    } else {
        ImGui::TextDisabled("No cell selected.");
    }

    if (snapshot.metadata.has_error) {
        ImGui::TextDisabled("Last error: %s",
                            math::integration::describe_error(snapshot.metadata.last_error).c_str());
    }
}

void SimulationIntegrationDerivativeLab::draw_2d_problem_tab() {
    auto snapshot = m_workbench.snapshot();
    bool changed = false;

    const char* domain_names[] = {"Rectangle"};
    int domain_index = 0;
    ImGui::Combo("Domain", &domain_index, domain_names, IM_ARRAYSIZE(domain_names));

    const char* integrand_names[] = {"Gaussian", "Wave", "Polynomial", "Step X"};
    int integrand_index = static_cast<int>(snapshot.problem.integrand_preset);
    if (ImGui::Combo("Integrand", &integrand_index, integrand_names, IM_ARRAYSIZE(integrand_names))) {
        changed |= m_workbench.set_integrand(static_cast<math::integration::IntegrandPreset2D>(integrand_index));
    }

    float x_min = static_cast<float>(snapshot.problem.domain.x_min);
    float x_max = static_cast<float>(snapshot.problem.domain.x_max);
    float y_min = static_cast<float>(snapshot.problem.domain.y_min);
    float y_max = static_cast<float>(snapshot.problem.domain.y_max);
    bool domain_changed = false;
    domain_changed |= ImGui::SliderFloat("x min", &x_min, -8.f, 7.5f);
    domain_changed |= ImGui::SliderFloat("x max", &x_max, -7.5f, 8.f);
    domain_changed |= ImGui::SliderFloat("y min", &y_min, -8.f, 7.5f);
    domain_changed |= ImGui::SliderFloat("y max", &y_max, -7.5f, 8.f);
    if (domain_changed) {
        changed |= m_workbench.set_domain(math::integration::RectDomain2D{
            .x_min = static_cast<f64>(x_min),
            .x_max = static_cast<f64>(x_max),
            .y_min = static_cast<f64>(y_min),
            .y_max = static_cast<f64>(y_max)
        });
    }

    if (changed) {
        snapshot = m_workbench.snapshot();
        m_status = snapshot.metadata.has_error ? "2D workbench configuration error" : "Ready";
    }
}

void SimulationIntegrationDerivativeLab::draw_2d_partition_tab() {
    auto snapshot = m_workbench.snapshot();
    bool changed = false;

    const char* partition_names[] = {"Uniform Grid"};
    int partition_index = 0;
    ImGui::Combo("Partition", &partition_index, partition_names, IM_ARRAYSIZE(partition_names));

    int x_cells = static_cast<int>(snapshot.problem.grid.x_cells);
    int y_cells = static_cast<int>(snapshot.problem.grid.y_cells);
    if (ImGui::SliderInt("X cells", &x_cells, 2, 160)) {
        changed |= m_workbench.set_grid(math::integration::UniformGrid2DConfig{
            .x_cells = static_cast<u32>(std::max(x_cells, 2)),
            .y_cells = snapshot.problem.grid.y_cells
        });
    }
    snapshot = m_workbench.snapshot();
    if (ImGui::SliderInt("Y cells", &y_cells, 2, 160)) {
        changed |= m_workbench.set_grid(math::integration::UniformGrid2DConfig{
            .x_cells = snapshot.problem.grid.x_cells,
            .y_cells = static_cast<u32>(std::max(y_cells, 2))
        });
    }

    snapshot = m_workbench.snapshot();
    int selected_cell = snapshot.selected_cell.valid
        ? static_cast<int>(snapshot.selected_cell.cell_id)
        : 0;
    const int max_cell = static_cast<int>(std::max<u64>(snapshot.result.cell_count, u64(1)) - u64(1));
    if (ImGui::SliderInt("Selected cell", &selected_cell, 0, max_cell)) {
        m_workbench.select_cell(static_cast<u32>(selected_cell));
    }

    if (changed) {
        m_status = m_workbench.snapshot().metadata.has_error ? "2D workbench configuration error" : "Ready";
    }
}

void SimulationIntegrationDerivativeLab::draw_2d_display_tab() {
    IntegrationDisplayConfig display = m_workbench.display_config();

    const char* display_names[] = {"Value", "Contribution", "Local Error"};
    int display_index = static_cast<int>(display.mode);
    if (ImGui::Combo("Display mode", &display_index, display_names, IM_ARRAYSIZE(display_names))) {
        display.mode = static_cast<IntegrationDisplayMode>(display_index);
    }
    ImGui::Checkbox("Domain boundary", &display.show_domain_boundary);
    ImGui::Checkbox("Cell grid", &display.show_cells);
    ImGui::Checkbox("Sample points", &display.show_samples);
    ImGui::Checkbox("Axes", &display.show_axes);
    m_workbench.set_display_config(display);
}

void SimulationIntegrationDerivativeLab::draw_2d_analysis_tab() {
    const IntegrationAnalysisSnapshot analysis = m_workbench.analysis_snapshot();
    const char* method_names[] = {"Midpoint", "Trapezoid"};
    int method_index = static_cast<int>(m_workbench.snapshot().problem.method);
    if (ImGui::Combo("Active method", &method_index, method_names, IM_ARRAYSIZE(method_names))) {
        (void)m_workbench.set_method(static_cast<math::integration::IntegrationMethod2D>(method_index));
    }

    ImGui::SeparatorText("Convergence");
    if (ImGui::BeginTable("ConvergenceTable", 4, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("Grid");
        ImGui::TableSetupColumn("Estimate");
        ImGui::TableSetupColumn("Abs error");
        ImGui::TableSetupColumn("Order");
        ImGui::TableHeadersRow();
        for (const auto& row : analysis.convergence.rows) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("%u x %u", row.grid.x_cells, row.grid.y_cells);
            ImGui::TableNextColumn();
            ImGui::Text("%.8f", row.estimate);
            ImGui::TableNextColumn();
            row.absolute_error ? ImGui::Text("%.3e", *row.absolute_error) : ImGui::TextDisabled("-");
            ImGui::TableNextColumn();
            row.observed_order ? ImGui::Text("%.2f", *row.observed_order) : ImGui::TextDisabled("-");
        }
        ImGui::EndTable();
    }

    ImGui::SeparatorText("Method Comparison");
    for (const auto& row : analysis.method_comparison) {
        ImGui::Text("%s  estimate %.8f  error %.3e",
                    method_2d_label(row.method),
                    row.estimate,
                    row.absolute_error.value_or(f64(0)));
    }
}

void SimulationIntegrationDerivativeLab::draw_1d_bridge_panel() {
    bool changed = false;
    ImGui::SeparatorText("1D Bridge");
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
}

void SimulationIntegrationDerivativeLab::submit_geometry() {
    if (!m_host || m_main_view == RenderViewId(0)) return;

    if (!m_show_1d_bridge) {
        (void)submit_integration_workbench_packets(
            m_host->render(),
            m_main_view,
            m_workbench.snapshot(),
            m_memory);
        (void)submit_integration_analytics_packets(
            m_host->render(),
            m_analytics_view,
            m_workbench.analysis_snapshot(),
            m_memory);
        return;
    }

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

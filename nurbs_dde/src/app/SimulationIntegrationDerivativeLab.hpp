#pragma once
// app/SimulationIntegrationDerivativeLab.hpp
// First vertical slice of the integration and derivative mathematical lab.

#include "engine/ISimulation.hpp"
#include "engine/ScopedServiceHandles.hpp"
#include "math/integration/Integration1D.hpp"

#include <array>
#include <string_view>

namespace ndde {

class SimulationIntegrationDerivativeLab final : public ISimulation {
public:
    explicit SimulationIntegrationDerivativeLab(memory::MemoryService* memory = nullptr);

    [[nodiscard]] std::string_view name() const override { return "Integration & Derivative Lab"; }

    void on_register(SimulationHost& host) override;
    void on_start() override;
    void on_tick(const TickInfo& tick) override;
    void on_submit_render() override;
    void on_stop() override;

    [[nodiscard]] SceneSnapshot snapshot() const override;
    [[nodiscard]] SimulationMetadata metadata() const override;

    [[nodiscard]] RenderViewId main_view_id() const noexcept { return m_main_view; }
    [[nodiscard]] const math::integration::IntegrationResult1D& result() const noexcept { return m_result; }

private:
    struct FunctionPreset {
        const char* name = "";
        const char* formula = "";
        math::integration::ScalarFunction1D function;
        math::integration::ScalarFunction1D antiderivative;
        math::integration::Interval1D interval{};
    };

    memory::MemoryService* m_memory = nullptr;
    SimulationHost* m_host = nullptr;
    ScopedServiceHandles<PanelHandle> m_panel_handles;
    RenderViewHandle m_main_handle;
    RenderViewId m_main_view = RenderViewId(0);

    std::array<FunctionPreset, 4> m_presets;
    std::size_t m_preset_index = 0;
    math::integration::QuadratureMethod m_method = math::integration::QuadratureMethod::Midpoint;
    math::integration::UniformPartitionConfig m_partition{.cell_count = u32(32)};
    math::integration::IntegrationResult1D m_result;
    std::string m_status = "Ready";
    f32 m_time = f32(0);
    f64 m_probe_x = f64(1.5707963267948966);
    f64 m_probe_derivative = f64(0);
    bool m_show_cells = true;
    bool m_show_tangent = true;

    void recompute();
    void draw_control_panel();
    void submit_geometry();
    [[nodiscard]] const FunctionPreset& preset() const noexcept { return m_presets[m_preset_index]; }
    [[nodiscard]] f64 reference_value() const;
};

} // namespace ndde

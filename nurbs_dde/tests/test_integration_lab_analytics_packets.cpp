#include "app/IntegrationLabAnalyticsPackets.hpp"
#include "engine/SimulationHost.hpp"

#include <gtest/gtest.h>

namespace {

using namespace ndde;

TEST(IntegrationLabAnalyticsPackets, ConvergencePlotEmitsAxesAndSeries) {
    memory::MemoryService memory;
    IntegrationWorkbenchState state;
    auto vertices = memory.frame().make_vector<Vertex>();

    append_integration_convergence_plot(vertices, state.analysis_snapshot());

    EXPECT_GT(vertices.size(), 4u);
}

TEST(IntegrationLabAnalyticsPackets, MethodComparisonEmitsBars) {
    memory::MemoryService memory;
    IntegrationWorkbenchState state;
    auto vertices = memory.frame().make_vector<Vertex>();

    append_integration_method_comparison(vertices, state.analysis_snapshot());

    EXPECT_EQ(vertices.size(), state.analysis_snapshot().method_comparison.size() * 6u);
}

TEST(IntegrationLabAnalyticsPackets, SubmitCreatesAlternateViewPackets) {
    EngineServices services;
    RenderViewId view = RenderViewId(0);
    auto handle = services.render().register_view(RenderViewDescriptor{
        .title = "Analytics",
        .kind = RenderViewKind::Alternate,
        .projection = CameraProjection::Orthographic,
        .camera_profile = CameraViewProfile::Orthographic2D
    }, &view);
    ASSERT_TRUE(handle);

    IntegrationWorkbenchState state;
    const IntegrationLabAnalyticsRenderStats stats = submit_integration_analytics_packets(
        services.render(),
        view,
        state.analysis_snapshot(),
        &services.memory());

    EXPECT_GT(stats.convergence_vertices, 0u);
    EXPECT_GT(stats.comparison_vertices, 0u);
    EXPECT_EQ(services.render().packet_count(view), 2u);
}

} // namespace

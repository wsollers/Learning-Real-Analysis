#include "engine/HotkeyService.hpp"
#include "engine/ISimulation.hpp"
#include "engine/PanelService.hpp"
#include "engine/RenderService.hpp"
#include "engine/SimulationHost.hpp"

#include <gtest/gtest.h>

namespace {

using namespace ndde;

TEST(PanelService, RegisterUnregisterPanel) {
    PanelService panels;
    int draws = 0;

    auto handle = panels.register_panel(PanelDescriptor{
        .title = "Particles",
        .category = "Simulation",
        .draw = [&draws] { ++draws; }
    });

    EXPECT_EQ(panels.active_count(), 1u);
    EXPECT_EQ(panels.active_count(PanelScope::Simulation), 1u);
    EXPECT_EQ(panels.active_count(PanelScope::Global), 0u);
    EXPECT_TRUE(panels.contains("Particles"));

    panels.draw_registered_panels();
    EXPECT_EQ(draws, 1);

    handle.reset();
    EXPECT_EQ(panels.active_count(), 0u);
    EXPECT_FALSE(panels.contains("Particles"));

    panels.draw_registered_panels();
    EXPECT_EQ(draws, 1);
}

TEST(PanelService, DrawsPanelsByScope) {
    PanelService panels;
    int global_draws = 0;
    int simulation_draws = 0;

    auto global = panels.register_panel(PanelDescriptor{
        .title = "Global",
        .scope = PanelScope::Global,
        .draw = [&global_draws] { ++global_draws; }
    });
    auto simulation = panels.register_panel(PanelDescriptor{
        .title = "Simulation",
        .scope = PanelScope::Simulation,
        .draw = [&simulation_draws] { ++simulation_draws; }
    });

    panels.draw_registered_panels(PanelScope::Global);
    EXPECT_EQ(global_draws, 1);
    EXPECT_EQ(simulation_draws, 0);

    panels.draw_registered_panels(PanelScope::Simulation);
    EXPECT_EQ(global_draws, 1);
    EXPECT_EQ(simulation_draws, 1);
}

TEST(PanelService, HandleUnregistersOnDestruction) {
    PanelService panels;
    {
        auto handle = panels.register_panel(PanelDescriptor{
            .title = "Temporary",
            .draw = [] {}
        });
        EXPECT_TRUE(handle);
        EXPECT_EQ(panels.active_count(), 1u);
    }
    EXPECT_EQ(panels.active_count(), 0u);
}

TEST(HotkeyService, DispatchesMatchingChord) {
    HotkeyService hotkeys;
    int calls = 0;

    auto handle = hotkeys.register_action(HotkeyDescriptor{
        .chord = {.key = 66, .mods = 2},
        .label = "Spawn Brownian",
        .callback = [&calls] { ++calls; }
    });

    EXPECT_EQ(hotkeys.active_count(), 1u);
    EXPECT_TRUE(hotkeys.contains("Spawn Brownian"));
    EXPECT_FALSE(hotkeys.dispatch({.key = 67, .mods = 2}));
    EXPECT_EQ(calls, 0);
    EXPECT_TRUE(hotkeys.dispatch({.key = 66, .mods = 2}));
    EXPECT_EQ(calls, 1);

    handle.reset();
    EXPECT_EQ(hotkeys.active_count(), 0u);
    EXPECT_FALSE(hotkeys.dispatch({.key = 66, .mods = 2}));
    EXPECT_EQ(calls, 1);
}

TEST(RenderService, RegistersMainAndAlternateViewsAndQueuesPackets) {
    RenderService render;
    RenderViewId main_id = 0;
    RenderViewId alt_id = 0;

    auto main = render.register_view(RenderViewDescriptor{
        .title = "Surface 3D",
        .kind = RenderViewKind::Main
    }, &main_id);
    auto alt = render.register_view(RenderViewDescriptor{
        .title = "Alternate View",
        .kind = RenderViewKind::Alternate,
        .alternate_mode = AlternateViewMode::VectorField,
        .projection = CameraProjection::Orthographic
    }, &alt_id);

    ASSERT_NE(main_id, 0u);
    ASSERT_NE(alt_id, 0u);
    EXPECT_EQ(render.active_view_count(), 2u);
    EXPECT_TRUE(render.contains_view("Surface 3D"));
    EXPECT_TRUE(render.contains_view("Alternate View"));
    ASSERT_NE(render.descriptor(alt_id), nullptr);
    EXPECT_EQ(render.descriptor(alt_id)->alternate_mode, AlternateViewMode::VectorField);
    EXPECT_EQ(render.descriptor(alt_id)->projection, CameraProjection::Orthographic);

    render.set_view_domain(main_id, RenderViewDomain{.u_min = -2.f, .u_max = 2.f, .v_min = -3.f, .v_max = 3.f});
    EXPECT_FLOAT_EQ(render.view_domain(main_id).u_min, -2.f);
    const float yaw_before = render.descriptor(main_id)->camera.yaw;
    render.orbit_main_cameras(10.f, -5.f);
    EXPECT_NE(render.descriptor(main_id)->camera.yaw, yaw_before);
    render.zoom_main_cameras(1.f);
    EXPECT_GT(render.descriptor(main_id)->camera.zoom, 1.f);
    render.queue_surface_perturbation(SurfacePerturbCommand{.view = main_id, .uv = {0.25f, -0.5f}, .seed = 42u});
    const auto commands = render.consume_surface_perturbations(main_id);
    ASSERT_EQ(commands.size(), 1u);
    EXPECT_FLOAT_EQ(commands.front().uv.x, 0.25f);

    const Vertex verts[] = {
        {{0.f, 0.f, 0.f}, {1.f, 0.f, 0.f, 1.f}},
        {{1.f, 0.f, 0.f}, {1.f, 0.f, 0.f, 1.f}}
    };
    render.submit(main_id, verts, Topology::LineList, DrawMode::VertexColor, {1,1,1,1}, Mat4{1.f});
    render.submit(alt_id, verts, Topology::LineList, DrawMode::VertexColor, {1,1,1,1}, Mat4{1.f});

    EXPECT_EQ(render.packet_count(main_id), 1u);
    EXPECT_EQ(render.packet_count(alt_id), 1u);
    EXPECT_EQ(render.packets().size(), 2u);

    main.reset();
    EXPECT_EQ(render.active_view_count(), 1u);
    render.submit(main_id, verts, Topology::LineList, DrawMode::VertexColor, {1,1,1,1}, Mat4{1.f});
    EXPECT_EQ(render.packet_count(main_id), 1u);
}

TEST(SimulationClock, AdvancesTickAndTimeAndPauses) {
    SimulationClock clock;

    TickInfo t0 = clock.next(0.25f);
    EXPECT_EQ(t0.tick_index, 1u);
    EXPECT_FLOAT_EQ(t0.dt, 0.25f);
    EXPECT_FLOAT_EQ(t0.time, 0.25f);
    EXPECT_FALSE(t0.paused);

    TickInfo paused = clock.next(0.25f, true);
    EXPECT_EQ(paused.tick_index, 1u);
    EXPECT_FLOAT_EQ(paused.dt, 0.f);
    EXPECT_FLOAT_EQ(paused.time, 0.25f);
    EXPECT_TRUE(paused.paused);

    clock.reset();
    EXPECT_EQ(clock.current().tick_index, 0u);
    EXPECT_FLOAT_EQ(clock.current().time, 0.f);
}

TEST(SimulationHost, ExposesOnlyServiceFacade) {
    EngineServices services;
    SimulationHost host = services.simulation_host();

    auto panel = host.panels().register_panel(PanelDescriptor{
        .title = "Host Panel",
        .draw = [] {}
    });
    auto hotkey = host.hotkeys().register_action(HotkeyDescriptor{
        .chord = {.key = 1, .mods = 0},
        .label = "Host Hotkey",
        .callback = [] {}
    });
    RenderViewId view_id = 0;
    auto view = host.render().register_view(RenderViewDescriptor{
        .title = "Host View",
        .kind = RenderViewKind::Main
    }, &view_id);

    EXPECT_EQ(services.panels().active_count(), 1u);
    EXPECT_EQ(services.hotkeys().active_count(), 1u);
    EXPECT_EQ(services.render().active_view_count(), 1u);
    EXPECT_EQ(host.clock().next(0.1f).tick_index, 1u);
}

class DummySimulation final : public ISimulation {
public:
    std::string_view name() const override { return "Dummy"; }

    void on_register(SimulationHost& host) override {
        panel = host.panels().register_panel(PanelDescriptor{.title = "Dummy Panel", .draw = [] {}});
        hotkey = host.hotkeys().register_action(HotkeyDescriptor{
            .chord = {.key = 4, .mods = 2},
            .label = "Dummy Hotkey",
            .callback = [this] { ++hotkey_calls; }
        });
        view = host.render().register_view(RenderViewDescriptor{.title = "Dummy View"}, &view_id);
    }

    void on_start() override { started = true; }
    void on_tick(const TickInfo& tick) override {
        last_tick = tick;
        ++ticks;
    }
    void on_stop() override {
        stopped = true;
        panel.reset();
        hotkey.reset();
        view.reset();
    }

    PanelHandle panel;
    HotkeyHandle hotkey;
    RenderViewHandle view;
    RenderViewId view_id = 0;
    TickInfo last_tick{};
    int ticks = 0;
    int hotkey_calls = 0;
    bool started = false;
    bool stopped = false;
};

TEST(ISimulation, RegistersServicesAndRollsBackHandles) {
    EngineServices services;
    SimulationHost host = services.simulation_host();
    DummySimulation sim;

    sim.on_register(host);
    sim.on_start();
    sim.on_tick(host.clock().next(0.016f));

    EXPECT_TRUE(sim.started);
    EXPECT_EQ(sim.ticks, 1);
    EXPECT_EQ(sim.last_tick.tick_index, 1u);
    EXPECT_EQ(services.panels().active_count(), 1u);
    EXPECT_EQ(services.hotkeys().active_count(), 1u);
    EXPECT_EQ(services.render().active_view_count(), 1u);

    EXPECT_TRUE(services.hotkeys().dispatch({.key = 4, .mods = 2}));
    EXPECT_EQ(sim.hotkey_calls, 1);

    sim.on_stop();
    EXPECT_TRUE(sim.stopped);
    EXPECT_EQ(services.panels().active_count(), 0u);
    EXPECT_EQ(services.hotkeys().active_count(), 0u);
    EXPECT_EQ(services.render().active_view_count(), 0u);
    EXPECT_FALSE(services.hotkeys().dispatch({.key = 4, .mods = 2}));
}

} // namespace

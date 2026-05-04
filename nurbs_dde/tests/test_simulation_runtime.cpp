#include "engine/SimulationRuntime.hpp"
#include "engine/SimulationHost.hpp"

#include <gtest/gtest.h>

namespace {

using namespace ndde;

class RuntimeDummySimulation final : public ISimulation {
public:
    std::string_view name() const override { return "Dummy Simulation"; }

    void on_register(SimulationHost& host) override {
        panel = host.panels().register_panel(PanelDescriptor{.title = "Runtime Panel", .draw = [] {}});
        hotkey = host.hotkeys().register_action(HotkeyDescriptor{
            .chord = {.key = 88, .mods = 2},
            .label = "Runtime Hotkey",
            .callback = [this] { ++hotkey_calls; }
        });
        view = host.render().register_view(RenderViewDescriptor{
            .title = "Runtime View",
            .kind = RenderViewKind::Main
        }, &view_id);
    }

    void on_start() override { ++starts; }
    void on_tick(const TickInfo& tick) override {
        last_tick = tick;
        ++ticks;
    }
    void on_stop() override {
        ++stops;
        panel.reset();
        hotkey.reset();
        view.reset();
    }

    [[nodiscard]] SceneSnapshot snapshot() const override {
        return SceneSnapshot{
            .name = "Dummy Simulation",
            .paused = last_tick.paused,
            .sim_time = last_tick.time,
            .status = "Simulation Snapshot"
        };
    }

    PanelHandle panel;
    HotkeyHandle hotkey;
    RenderViewHandle view;
    RenderViewId view_id = 0;
    TickInfo last_tick{};
    int starts = 0;
    int stops = 0;
    int ticks = 0;
    int hotkey_calls = 0;
};

TEST(SimulationRuntime, RuntimeOwnsISimulationLifecycle) {
    EngineServices services;
    SimulationHost host = services.simulation_host();
    RuntimeDummySimulation* raw = nullptr;
    SimulationRuntime runtime("Simulation Runtime",
        [&raw](memory::MemoryService& memory) {
            auto sim = memory.simulation().make_unique_as<ISimulation, RuntimeDummySimulation>();
            raw = static_cast<RuntimeDummySimulation*>(sim.get());
            return sim;
        });

    runtime.instantiate(host);
    ASSERT_NE(raw, nullptr);
    EXPECT_TRUE(runtime.paused());
    EXPECT_EQ(services.panels().active_count(), 1u);
    EXPECT_EQ(services.hotkeys().active_count(), 1u);
    EXPECT_EQ(services.render().active_view_count(), 1u);

    runtime.start();
    EXPECT_FALSE(runtime.paused());
    runtime.tick(TickInfo{.tick_index = 1u, .dt = 0.5f, .time = 0.5f, .paused = false});
    runtime.publish();

    EXPECT_EQ(raw->starts, 1);
    EXPECT_EQ(raw->ticks, 1);
    EXPECT_FLOAT_EQ(raw->last_tick.time, 0.5f);
    EXPECT_EQ(runtime.snapshot().name, "Dummy Simulation");
    EXPECT_EQ(runtime.snapshot().status, "Simulation Snapshot");

    EXPECT_TRUE(services.hotkeys().dispatch({.key = 88, .mods = 2}));
    EXPECT_EQ(raw->hotkey_calls, 1);

    runtime.pause();
    EXPECT_TRUE(runtime.paused());
    runtime.tick(TickInfo{.tick_index = 2u, .dt = 0.5f, .time = 1.0f, .paused = false});
    EXPECT_TRUE(raw->last_tick.paused);
    EXPECT_FLOAT_EQ(raw->last_tick.dt, 0.f);

    runtime.stop();
    EXPECT_EQ(services.panels().active_count(), 0u);
    EXPECT_EQ(services.hotkeys().active_count(), 0u);
    EXPECT_EQ(services.render().active_view_count(), 0u);
}

TEST(SimulationRuntime, ReinstantiateRollsBackPreviousRegistrations) {
    EngineServices services;
    SimulationHost host = services.simulation_host();
    int created = 0;
    SimulationRuntime runtime("Simulation Runtime",
        [&created](memory::MemoryService& memory) {
            ++created;
            return memory.simulation().make_unique_as<ISimulation, RuntimeDummySimulation>();
        });

    runtime.instantiate(host);
    EXPECT_EQ(created, 1);
    EXPECT_EQ(services.panels().active_count(), 1u);

    runtime.instantiate(host);
    EXPECT_EQ(created, 2);
    EXPECT_EQ(services.panels().active_count(), 1u);
    EXPECT_EQ(services.hotkeys().active_count(), 1u);
    EXPECT_EQ(services.render().active_view_count(), 1u);
}

TEST(SimulationRuntime, RegistryStoresSimulationRuntimesOnly) {
    EngineServices services;
    SimulationRegistry registry(services.memory());
    registry.add_runtime<RuntimeDummySimulation>("Simulation A");
    registry.add_runtime<RuntimeDummySimulation>("Simulation B");

    EXPECT_EQ(registry.size(), 2u);
    ASSERT_NE(registry.get(0), nullptr);
    ASSERT_NE(registry.get(1), nullptr);
    EXPECT_EQ(registry.get(0)->name(), "Simulation A");
    EXPECT_EQ(registry.get(1)->name(), "Simulation B");
}

} // namespace

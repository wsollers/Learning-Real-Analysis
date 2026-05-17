#include <gtest/gtest.h>

#include "engine/SimulationHost.hpp"
#include "engine/events/EventBusService.hpp"
#include "simulation/events/EngineEventTypes.hpp"

namespace {

struct TestEvent {
    ndde::u64 value = ndde::u64(0);
};

[[nodiscard]] ndde::events::EventRecord test_record(ndde::u64 value) {
    ndde::events::EventRecord record;
    record.kind = ndde::events::EventKind::AlertCustom;
    record.severity = ndde::events::EventSeverity::Notice;
    record.tick = value;
    record.val_a = static_cast<ndde::f32>(value);
    record.set_label("TestEvent");
    return record;
}

} // namespace

TEST(EventBusService, TypedSubscriberReceivesPublishedEvent) {
    ndde::EventBusService events;
    events.init();

    ndde::u64 observed = ndde::u64(0);
    auto sub = events.subscribe<TestEvent>(
        ndde::EventChannelId::Simulation,
        [&](const TestEvent& event) { observed = event.value; });

    events.publish(ndde::EventChannelId::Simulation, TestEvent{.value = ndde::u64(42)}, test_record(42));

    EXPECT_TRUE(sub.active());
    EXPECT_EQ(observed, 42u);
}

TEST(EventBusService, SubscriptionResetStopsDelivery) {
    ndde::EventBusService events;
    events.init();

    ndde::u64 calls = ndde::u64(0);
    auto sub = events.subscribe<TestEvent>(
        ndde::EventChannelId::Simulation,
        [&](const TestEvent&) { ++calls; });

    events.publish(ndde::EventChannelId::Simulation, TestEvent{}, test_record(1));
    sub.reset();
    events.publish(ndde::EventChannelId::Simulation, TestEvent{}, test_record(2));

    EXPECT_FALSE(sub.active());
    EXPECT_EQ(calls, 1u);
}

TEST(EventBusService, MoveOnlySubscriptionUnsubscribesExactlyOnce) {
    ndde::EventBusService events;
    events.init();

    ndde::u64 calls = ndde::u64(0);
    auto sub = events.subscribe<TestEvent>(
        ndde::EventChannelId::Simulation,
        [&](const TestEvent&) { ++calls; });

    ndde::Subscription moved = std::move(sub);
    EXPECT_FALSE(sub.active());
    EXPECT_TRUE(moved.active());

    moved.reset();
    moved.reset();
    events.publish(ndde::EventChannelId::Simulation, TestEvent{}, test_record(1));

    EXPECT_EQ(calls, 0u);
}

TEST(EventBusService, PublishWithRecordDrainsToChannelLog) {
    ndde::EventBusService events;
    events.init(ndde::EventBusConfig{{
        ndde::EventChannelConfig{
            .channel = ndde::EventChannelId::Simulation,
            .ring_capacity_records = ndde::u64(8),
            .max_display_records = ndde::u64(8),
            .record_to_log = true
        }
    }});

    events.publish(ndde::EventChannelId::Simulation, TestEvent{.value = ndde::u64(7)}, test_record(7));
    events.drain(ndde::EventChannelId::Simulation, 0.f, ndde::u64(7));

    const auto& entries = events.log(ndde::EventChannelId::Simulation).entries();
    ASSERT_EQ(entries.size(), 1u);
    EXPECT_NE(entries.front().text.find("TestEvent"), std::string::npos);
}

TEST(EventBusService, ChannelsAreIsolated) {
    ndde::EventBusService events;
    events.init();

    ndde::u64 app_calls = ndde::u64(0);
    ndde::u64 sim_calls = ndde::u64(0);
    auto app_sub = events.subscribe<TestEvent>(
        ndde::EventChannelId::App,
        [&](const TestEvent&) { ++app_calls; });
    auto sim_sub = events.subscribe<TestEvent>(
        ndde::EventChannelId::Simulation,
        [&](const TestEvent&) { ++sim_calls; });

    events.publish(ndde::EventChannelId::App, TestEvent{}, test_record(1));

    EXPECT_EQ(app_calls, 1u);
    EXPECT_EQ(sim_calls, 0u);
}

TEST(EventBusService, ClearScenarioChannelsLeavesAppChannelAlive) {
    ndde::EventBusService events;
    events.init();

    ndde::u64 app_calls = ndde::u64(0);
    ndde::u64 sim_calls = ndde::u64(0);
    auto app_sub = events.subscribe<TestEvent>(
        ndde::EventChannelId::App,
        [&](const TestEvent&) { ++app_calls; });
    auto sim_sub = events.subscribe<TestEvent>(
        ndde::EventChannelId::Simulation,
        [&](const TestEvent&) { ++sim_calls; });

    events.clear_scenario_channels();
    events.publish(ndde::EventChannelId::App, TestEvent{}, test_record(1));
    events.publish(ndde::EventChannelId::Simulation, TestEvent{}, test_record(2));

    EXPECT_EQ(app_calls, 1u);
    EXPECT_EQ(sim_calls, 0u);
}

TEST(EventBusService, KnownEngineEventUsesConvenienceRecordConversion) {
    ndde::EventBusService events;
    events.init();

    events.publish(ndde::EventChannelId::App, ndde::events::AppStarted{.config_path = "test.json"});
    events.drain(ndde::EventChannelId::App, 0.f, ndde::u64(0));

    const auto& entries = events.log(ndde::EventChannelId::App).entries();
    ASSERT_EQ(entries.size(), 1u);
    EXPECT_NE(entries.front().text.find("AppStarted"), std::string::npos);
}

TEST(EngineServices, OwnsEventBusServiceAndPassesItToSimulationHost) {
    ndde::EngineServices services;
    services.events().init();

    ndde::SimulationHost host = services.simulation_host();
    ndde::u64 calls = ndde::u64(0);
    auto sub = host.events().subscribe<TestEvent>(
        ndde::EventChannelId::Simulation,
        [&](const TestEvent&) { ++calls; });

    services.events().publish(ndde::EventChannelId::Simulation, TestEvent{}, test_record(1));

    EXPECT_EQ(calls, 1u);
}

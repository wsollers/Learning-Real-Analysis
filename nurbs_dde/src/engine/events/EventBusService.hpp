#pragma once
// engine/events/EventBusService.hpp
// Engine-owned typed event routing service.

#include "math/Scalars.hpp"
#include "simulation/events/EventBus.hpp"
#include "simulation/events/EventLog.hpp"

#include <array>
#include <functional>
#include <span>
#include <vector>

namespace ndde {

enum class EventChannelId : u8 {
    App,
    Scenario,
    Simulation,
    Ui,
    Worker,
    Count
};

enum class EventScope : u8 {
    App,
    Scenario,
    Simulation,
    View,
    Capture,
    Worker
};

struct EventChannelConfig {
    EventChannelId channel = EventChannelId::Simulation;
    u64 ring_capacity_records = u64(4096);
    u64 max_display_records = u64(512);
    bool record_to_log = true;
};

struct EventBusConfig {
    std::vector<EventChannelConfig> channels;

    [[nodiscard]] static EventBusConfig defaults();
};

struct SubscriptionId {
    u64 value = u64(0);

    friend constexpr bool operator==(SubscriptionId, SubscriptionId) noexcept = default;
};

class Subscription {
public:
    Subscription() = default;
    ~Subscription() { reset(); }

    Subscription(const Subscription&) = delete;
    Subscription& operator=(const Subscription&) = delete;

    Subscription(Subscription&& other) noexcept
        : m_reset(std::move(other.m_reset))
        , m_id(other.m_id)
    {
        other.m_id = {};
    }

    Subscription& operator=(Subscription&& other) noexcept {
        if (this != &other) {
            reset();
            m_reset = std::move(other.m_reset);
            m_id = other.m_id;
            other.m_id = {};
        }
        return *this;
    }

    void reset() noexcept {
        if (m_reset && m_id.value != u64(0)) {
            m_reset(m_id);
            m_reset = {};
            m_id = {};
        }
    }

    [[nodiscard]] bool active() const noexcept { return m_id.value != u64(0); }
    [[nodiscard]] SubscriptionId id() const noexcept { return m_id; }

private:
    friend class EventBusService;

    Subscription(SubscriptionId id, std::function<void(SubscriptionId)> reset)
        : m_reset(std::move(reset))
        , m_id(id)
    {}

    std::function<void(SubscriptionId)> m_reset;
    SubscriptionId m_id;
};

class EventBusService {
public:
    EventBusService() = default;
    ~EventBusService() { shutdown(); }

    EventBusService(const EventBusService&) = delete;
    EventBusService& operator=(const EventBusService&) = delete;
    EventBusService(EventBusService&&) = delete;
    EventBusService& operator=(EventBusService&&) = delete;

    void init(EventBusConfig config = EventBusConfig::defaults());
    void shutdown() noexcept;

    template <class Event>
    [[nodiscard]] Subscription subscribe(EventChannelId channel,
                                         std::function<void(const Event&)> handler) {
        Channel& ch = channel_for(channel);
        const u64 token = ch.bus.subscribe<Event>(std::move(handler));
        return Subscription{
            SubscriptionId{token},
            [this, channel](SubscriptionId id) {
                if (!m_initialised) return;
                channel_for(channel).bus.unsubscribe<Event>(id.value);
            }
        };
    }

    template <class Event>
    void publish(EventChannelId channel, const Event& event) {
        channel_for(channel).bus.dispatch(event);
    }

    template <class Event>
    void publish(EventChannelId channel,
                 const Event& event,
                 const events::EventRecord& record) {
        channel_for(channel).bus.dispatch(event, record);
    }

    void drain(EventChannelId channel, f32 sim_time, u64 tick);
    void drain_all(f32 sim_time, u64 tick);
    void reset_channel(EventChannelId channel) noexcept;
    void clear_scenario_channels() noexcept;

    [[nodiscard]] events::EventLog& log(EventChannelId channel);
    [[nodiscard]] const events::EventLog& log(EventChannelId channel) const;
    [[nodiscard]] events::EventBus& bus(EventChannelId channel);
    [[nodiscard]] const events::EventBus& bus(EventChannelId channel) const;
    [[nodiscard]] bool initialised() const noexcept { return m_initialised; }

private:
    struct Channel {
        events::EventBus bus;
        events::EventLog log;
        bool record_to_log = true;
        bool configured = false;
    };

    std::array<Channel, static_cast<std::size_t>(EventChannelId::Count)> m_channels;
    bool m_initialised = false;

    [[nodiscard]] static std::size_t index_of(EventChannelId channel) noexcept {
        return static_cast<std::size_t>(channel);
    }

    [[nodiscard]] Channel& channel_for(EventChannelId channel);
    [[nodiscard]] const Channel& channel_for(EventChannelId channel) const;
};

} // namespace ndde

// engine/events/EventBusService.cpp

#include "engine/events/EventBusService.hpp"

#include <stdexcept>

namespace ndde {

EventBusConfig EventBusConfig::defaults() {
    return EventBusConfig{{
        EventChannelConfig{
            .channel = EventChannelId::App,
            .ring_capacity_records = u64(2048),
            .max_display_records = u64(512),
            .record_to_log = true
        },
        EventChannelConfig{
            .channel = EventChannelId::Scenario,
            .ring_capacity_records = u64(2048),
            .max_display_records = u64(512),
            .record_to_log = true
        },
        EventChannelConfig{
            .channel = EventChannelId::Simulation,
            .ring_capacity_records = u64(4096),
            .max_display_records = u64(1024),
            .record_to_log = true
        },
        EventChannelConfig{
            .channel = EventChannelId::Ui,
            .ring_capacity_records = u64(1024),
            .max_display_records = u64(256),
            .record_to_log = true
        },
        EventChannelConfig{
            .channel = EventChannelId::Worker,
            .ring_capacity_records = u64(512),
            .max_display_records = u64(256),
            .record_to_log = true
        }
    }};
}

void EventBusService::init(EventBusConfig config) {
    shutdown();

    for (const EventChannelConfig& channel_config : config.channels) {
        Channel& ch = channel_for(channel_config.channel);
        ch.record_to_log = channel_config.record_to_log;
        ch.configured = true;
        ch.next_sequence = u64(1);
        if (channel_config.channel == EventChannelId::Worker) {
            m_worker_mailbox_capacity = channel_config.ring_capacity_records;
        }
        if (ch.record_to_log) {
            ch.log.init(channel_config.ring_capacity_records,
                        channel_config.max_display_records);
            ch.bus.attach_ring(&ch.log.ring());
        }
    }

    m_initialised = true;
}

void EventBusService::shutdown() noexcept {
    for (Channel& channel : m_channels) {
        channel.bus.clear_all_subscribers();
        channel.bus.detach_ring();
        channel.log.destroy();
        channel.next_sequence = u64(1);
        channel.record_to_log = true;
        channel.configured = false;
    }
    {
        std::scoped_lock lock{m_worker_mailbox_mutex};
        m_worker_mailbox.clear();
        m_worker_mailbox_capacity = u64(512);
        m_worker_mailbox_dropped = u64(0);
    }
    m_initialised = false;
}

bool EventBusService::enqueue_worker_record(events::EventRecord record) {
    std::scoped_lock lock{m_worker_mailbox_mutex};
    if (m_worker_mailbox.size() >= static_cast<std::size_t>(m_worker_mailbox_capacity)) {
        ++m_worker_mailbox_dropped;
        return false;
    }
    m_worker_mailbox.push_back(record);
    return true;
}

u64 EventBusService::drain_worker_mailbox() {
    std::vector<events::EventRecord> pending;
    {
        std::scoped_lock lock{m_worker_mailbox_mutex};
        pending.swap(m_worker_mailbox);
    }

    Channel& worker = channel_for(EventChannelId::Worker);
    for (events::EventRecord& record : pending) {
        record.sequence = next_sequence(worker);
        if (worker.record_to_log && worker.configured) {
            (void)worker.log.ring().push(record);
        }
    }
    return static_cast<u64>(pending.size());
}

void EventBusService::drain(EventChannelId channel, f32 sim_time, u64 tick) {
    if (channel == EventChannelId::Worker) {
        (void)drain_worker_mailbox();
    }
    Channel& ch = channel_for(channel);
    if (ch.record_to_log)
        ch.log.drain(sim_time, tick);
}

void EventBusService::drain_all(f32 sim_time, u64 tick) {
    (void)drain_worker_mailbox();
    for (Channel& ch : m_channels) {
        if (ch.configured && ch.record_to_log)
            ch.log.drain(sim_time, tick);
    }
}

void EventBusService::reset_channel(EventChannelId channel) noexcept {
    Channel& ch = m_channels[index_of(channel)];
    ch.bus.clear_all_subscribers();
    ch.log.reset();
    ch.next_sequence = u64(1);
    if (channel == EventChannelId::Worker) {
        std::scoped_lock lock{m_worker_mailbox_mutex};
        m_worker_mailbox.clear();
        m_worker_mailbox_dropped = u64(0);
    }
}

void EventBusService::clear_scenario_channels() noexcept {
    reset_channel(EventChannelId::Scenario);
    reset_channel(EventChannelId::Simulation);
}

events::EventLog& EventBusService::log(EventChannelId channel) {
    return channel_for(channel).log;
}

const events::EventLog& EventBusService::log(EventChannelId channel) const {
    return channel_for(channel).log;
}

events::EventBus& EventBusService::bus(EventChannelId channel) {
    return channel_for(channel).bus;
}

const events::EventBus& EventBusService::bus(EventChannelId channel) const {
    return channel_for(channel).bus;
}

u64 EventBusService::next_sequence_value(EventChannelId channel) const {
    return channel_for(channel).next_sequence;
}

u64 EventBusService::worker_mailbox_size() const {
    std::scoped_lock lock{m_worker_mailbox_mutex};
    return static_cast<u64>(m_worker_mailbox.size());
}

EventBusService::Channel& EventBusService::channel_for(EventChannelId channel) {
    const std::size_t index = index_of(channel);
    if (index >= m_channels.size())
        throw std::out_of_range("[EventBusService] Invalid event channel");
    return m_channels[index];
}

const EventBusService::Channel& EventBusService::channel_for(EventChannelId channel) const {
    const std::size_t index = index_of(channel);
    if (index >= m_channels.size())
        throw std::out_of_range("[EventBusService] Invalid event channel");
    return m_channels[index];
}

} // namespace ndde

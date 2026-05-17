#pragma once
// simulation/events/EventBus.hpp
// Typed synchronous event bus.
//
// Hot-path contract (dispatch):
//   1. Call all typed subscribers inline — zero allocation, wait-free.
//   2. Push an EventRecord into the attached EventRing — one atomic store.
//   3. NO std::format. NO heap allocation. NO mutex.
//
// Formatting and display happen in EventLog::drain(), called once per frame
// after the simulation tick completes.
//
// Thread safety: single-producer model — dispatch() must be called from the
// simulation tick thread only. subscribe/unsubscribe are main-thread only.

#include "simulation/events/EventRecord.hpp"
#include "simulation/events/EventRing.hpp"
#include "math/Scalars.hpp"
#include <any>
#include <functional>
#include <typeindex>
#include <unordered_map>
#include <vector>

namespace ndde::events {

class EventBus {
public:
    EventBus() = default;
    ~EventBus() = default;

    EventBus(const EventBus&)            = delete;
    EventBus& operator=(const EventBus&) = delete;
    EventBus(EventBus&&)                 = delete;
    EventBus& operator=(EventBus&&)      = delete;

    // Attach the ring that dispatch() will push records into.
    // Called by EventLog during init. Ring lifetime must exceed bus.
    void attach_ring(EventRing* ring) noexcept { m_ring = ring; }
    void detach_ring()                noexcept { m_ring = nullptr; }

    // ── Subscribe ─────────────────────────────────────────────────────────────
    // Returns an opaque token. Store it; pass to unsubscribe<E>() to remove.
    // Handlers are called synchronously in dispatch() on the sim thread.
    // Handlers must be fast and non-blocking.
    template <class E>
    [[nodiscard]] u64 subscribe(std::function<void(const E&)> handler) {
        const u64 token = m_next_token++;
        get_list<E>().push_back(Entry{
            token,
            [h = std::move(handler)](const std::any& e) {
                h(std::any_cast<const E&>(e));
            }
        });
        return token;
    }

    template <class E>
    void unsubscribe(u64 token) {
        auto& list = get_list<E>();
        list.erase(
            std::remove_if(list.begin(), list.end(),
                [token](const Entry& e){ return e.token == token; }),
            list.end());
    }

    void clear_all_subscribers() noexcept { m_subscribers.clear(); }

    // ── Dispatch — HOT PATH ───────────────────────────────────────────────────
    // Explicit record overload: caller supplies the EventRecord.
    // This is the preferred overload — zero overhead on the push side.
    template <class E>
    void dispatch(const E& event, const EventRecord& record) {
        for (auto& entry : get_list<E>())
            entry.handler(event);
        if (m_ring)
            (void)m_ring->push(record);
    }

    // Convenience overload: record is built from event via make_record<E>.
    // make_record specialisations are in EventBus.cpp.
    template <class E>
    void dispatch(const E& event) {
        dispatch(event, make_record(event));
    }

    [[nodiscard]] bool has_ring() const noexcept { return m_ring != nullptr; }

private:
    struct Entry {
        u64 token;
        std::function<void(const std::any&)> handler;
    };

    u64 m_next_token = u64(1);
    std::unordered_map<std::type_index, std::vector<Entry>> m_subscribers;
    EventRing* m_ring = nullptr;

    template <class E>
    std::vector<Entry>& get_list() {
        return m_subscribers[std::type_index(typeid(E))];
    }

    // make_record<E>: convert typed event struct to EventRecord.
    // Specialisations defined in EventBus.cpp for every event type.
    template <class E>
    static EventRecord make_record(const E& event);
};

// ── ScopedSub — RAII subscription wrapper ─────────────────────────────────────
// Analogous to ServiceHandle. Unsubscribes on destruction.
// SimulationUI holds a collection of these — all subs removed on on_stop().

template <class E>
class ScopedSub {
public:
    ScopedSub() = default;

    ScopedSub(EventBus& bus, std::function<void(const E&)> handler)
        : m_bus(&bus)
        , m_token(bus.subscribe<E>(std::move(handler)))
    {}

    ~ScopedSub() { reset(); }

    void reset() noexcept {
        if (m_bus && m_token) {
            m_bus->unsubscribe<E>(m_token);
            m_bus   = nullptr;
            m_token = u64(0);
        }
    }

    ScopedSub(const ScopedSub&)            = delete;
    ScopedSub& operator=(const ScopedSub&) = delete;

    ScopedSub(ScopedSub&& o) noexcept
        : m_bus(o.m_bus), m_token(o.m_token)
    { o.m_bus = nullptr; o.m_token = u64(0); }

    ScopedSub& operator=(ScopedSub&& o) noexcept {
        reset();
        m_bus = o.m_bus; m_token = o.m_token;
        o.m_bus = nullptr; o.m_token = u64(0);
        return *this;
    }

    [[nodiscard]] bool active() const noexcept { return m_bus && m_token; }

private:
    EventBus* m_bus   = nullptr;
    u64       m_token = u64(0);
};

} // namespace ndde::events

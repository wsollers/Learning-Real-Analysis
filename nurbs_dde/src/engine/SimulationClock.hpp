#pragma once
// engine/SimulationClock.hpp
// Central tick source for simulations.

#include "math/Scalars.hpp"

namespace ndde {

struct TickInfo {
    u64 tick_index = 0;
    f32 dt = 0.f;
    f32 time = 0.f;
    bool paused = false;
};

class SimulationClock {
public:
    [[nodiscard]] TickInfo next(f32 dt, bool paused = false) noexcept {
        if (!paused) {
            ++m_tick_index;
            m_time += dt;
        }
        m_last = TickInfo{
            .tick_index = m_tick_index,
            .dt = paused ? 0.f : dt,
            .time = m_time,
            .paused = paused
        };
        return m_last;
    }

    void reset() noexcept {
        m_tick_index = 0;
        m_time = 0.f;
        m_last = {};
    }

    [[nodiscard]] TickInfo current() const noexcept { return m_last; }

private:
    u64 m_tick_index = 0;
    f32 m_time = 0.f;
    TickInfo m_last{};
};

} // namespace ndde


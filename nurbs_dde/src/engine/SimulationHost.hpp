#pragma once
// engine/SimulationHost.hpp
// Narrow service interface passed to simulations.

#include "engine/HotkeyService.hpp"
#include "engine/PanelService.hpp"
#include "engine/RenderService.hpp"
#include "engine/SimulationClock.hpp"

namespace ndde {

class SimulationHost {
public:
    SimulationHost(PanelService& panels,
                   HotkeyService& hotkeys,
                   RenderService& render,
                   SimulationClock& clock) noexcept
        : m_panels(panels)
        , m_hotkeys(hotkeys)
        , m_render(render)
        , m_clock(clock)
    {}

    [[nodiscard]] PanelService& panels() const noexcept { return m_panels; }
    [[nodiscard]] HotkeyService& hotkeys() const noexcept { return m_hotkeys; }
    [[nodiscard]] RenderService& render() const noexcept { return m_render; }
    [[nodiscard]] SimulationClock& clock() const noexcept { return m_clock; }

private:
    PanelService& m_panels;
    HotkeyService& m_hotkeys;
    RenderService& m_render;
    SimulationClock& m_clock;
};

class EngineServices {
public:
    [[nodiscard]] PanelService& panels() noexcept { return m_panels; }
    [[nodiscard]] HotkeyService& hotkeys() noexcept { return m_hotkeys; }
    [[nodiscard]] RenderService& render() noexcept { return m_render; }
    [[nodiscard]] SimulationClock& clock() noexcept { return m_clock; }

    [[nodiscard]] SimulationHost simulation_host() noexcept {
        return SimulationHost(m_panels, m_hotkeys, m_render, m_clock);
    }

private:
    PanelService m_panels;
    HotkeyService m_hotkeys;
    RenderService m_render;
    SimulationClock m_clock;
};

} // namespace ndde


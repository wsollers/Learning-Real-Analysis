#pragma once
// engine/SimulationHost.hpp
// Narrow service interface passed to simulations.

#include "engine/CameraInputController.hpp"
#include "engine/CameraService.hpp"
#include "engine/HotkeyService.hpp"
#include "engine/InteractionService.hpp"
#include "engine/PanelService.hpp"
#include "engine/RenderService.hpp"
#include "engine/SimulationClock.hpp"
#include "memory/MemoryService.hpp"

namespace ndde {

class SimulationHost {
public:
    SimulationHost(PanelService& panels,
                   HotkeyService& hotkeys,
                   InteractionService& interaction,
                   RenderService& render,
                   CameraService& camera,
                   SimulationClock& clock,
                   memory::MemoryService& memory) noexcept
        : m_panels(panels)
        , m_hotkeys(hotkeys)
        , m_interaction(interaction)
        , m_render(render)
        , m_camera(camera)
        , m_clock(clock)
        , m_memory(memory)
    {}

    [[nodiscard]] PanelService& panels() const noexcept { return m_panels; }
    [[nodiscard]] HotkeyService& hotkeys() const noexcept { return m_hotkeys; }
    [[nodiscard]] InteractionService& interaction() const noexcept { return m_interaction; }
    [[nodiscard]] RenderService& render() const noexcept { return m_render; }
    [[nodiscard]] CameraService& camera() const noexcept { return m_camera; }
    [[nodiscard]] SimulationClock& clock() const noexcept { return m_clock; }
    [[nodiscard]] memory::MemoryService& memory() const noexcept { return m_memory; }

private:
    PanelService& m_panels;
    HotkeyService& m_hotkeys;
    InteractionService& m_interaction;
    RenderService& m_render;
    CameraService& m_camera;
    SimulationClock& m_clock;
    memory::MemoryService& m_memory;
};

class EngineServices {
public:
    EngineServices() {
        m_panels.set_memory_service(&m_memory);
        m_hotkeys.set_memory_service(&m_memory);
        m_interaction.set_memory_service(&m_memory);
        m_render.set_memory_service(&m_memory);
        m_camera.set_render_service(&m_render);
    }

    [[nodiscard]] PanelService& panels() noexcept { return m_panels; }
    [[nodiscard]] HotkeyService& hotkeys() noexcept { return m_hotkeys; }
    [[nodiscard]] InteractionService& interaction() noexcept { return m_interaction; }
    [[nodiscard]] RenderService& render() noexcept { return m_render; }
    [[nodiscard]] CameraService& camera() noexcept { return m_camera; }
    [[nodiscard]] CameraInputController& camera_input() noexcept { return m_camera_input; }
    [[nodiscard]] SimulationClock& clock() noexcept { return m_clock; }
    [[nodiscard]] memory::MemoryService& memory() noexcept { return m_memory; }

    [[nodiscard]] SimulationHost simulation_host() noexcept {
        return SimulationHost(m_panels, m_hotkeys, m_interaction, m_render, m_camera, m_clock, m_memory);
    }

private:
    memory::MemoryService m_memory;
    PanelService m_panels;
    HotkeyService m_hotkeys;
    InteractionService m_interaction;
    RenderService m_render;
    CameraService m_camera;
    CameraInputController m_camera_input;
    SimulationClock m_clock;
};

} // namespace ndde

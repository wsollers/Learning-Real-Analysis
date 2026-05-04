#pragma once
// engine/PanelService.hpp
// Engine-owned registration surface for UI panels.

#include "engine/ServiceHandle.hpp"
#include "memory/Containers.hpp"
#include "memory/MemoryService.hpp"

#include <algorithm>
#include <functional>
#include <string>
#include <string_view>

namespace ndde {

using PanelId = u64;
using PanelHandle = ServiceHandle;

enum class PanelScope : u8 {
    Global,
    Simulation
};

struct PanelDescriptor {
    std::string title;
    std::string category = "Simulation";
    PanelScope scope = PanelScope::Simulation;
    bool initially_open = true;
    std::function<void()> draw;
};

class PanelService {
public:
    void set_memory_service(memory::MemoryService* memory) {
        std::pmr::memory_resource* resource = memory ? memory->persistent().resource()
                                                     : std::pmr::get_default_resource();
        if (resource == m_panels.get_allocator().resource())
            return;
        std::destroy_at(&m_panels);
        std::construct_at(&m_panels, resource);
    }

    [[nodiscard]] PanelHandle register_panel(PanelDescriptor descriptor) {
        const PanelId id = m_next_id++;
        m_panels.push_back(PanelEntry{
            .id = id,
            .descriptor = std::move(descriptor),
            .active = true
        });
        return PanelHandle([this, id] { unregister(id); });
    }

    void draw_registered_panels(PanelScope scope) {
        for (auto& entry : m_panels) {
            if (!entry.active || !entry.descriptor.draw) continue;
            if (entry.descriptor.scope != scope) continue;
            entry.descriptor.draw();
        }
    }

    void draw_registered_panels() {
        draw_registered_panels(PanelScope::Global);
        draw_registered_panels(PanelScope::Simulation);
    }

    [[nodiscard]] std::size_t active_count() const noexcept {
        return static_cast<std::size_t>(std::count_if(m_panels.begin(), m_panels.end(),
            [](const PanelEntry& entry) { return entry.active; }));
    }

    [[nodiscard]] std::size_t active_count(PanelScope scope) const noexcept {
        return static_cast<std::size_t>(std::count_if(m_panels.begin(), m_panels.end(),
            [scope](const PanelEntry& entry) {
                return entry.active && entry.descriptor.scope == scope;
            }));
    }

    [[nodiscard]] bool contains(std::string_view title) const {
        return std::any_of(m_panels.begin(), m_panels.end(),
            [title](const PanelEntry& entry) {
                return entry.active && entry.descriptor.title == title;
            });
    }

private:
    struct PanelEntry {
        PanelId id = 0;
        PanelDescriptor descriptor;
        bool active = false;
    };

    PanelId m_next_id = 1;
    memory::PersistentVector<PanelEntry> m_panels;

    void unregister(PanelId id) noexcept {
        for (auto& entry : m_panels) {
            if (entry.id == id) {
                entry.active = false;
                entry.descriptor.draw = {};
                return;
            }
        }
    }
};

} // namespace ndde

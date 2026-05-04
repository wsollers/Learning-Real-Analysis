#pragma once
// engine/HotkeyService.hpp
// Engine-owned hotkey registration and dispatch service.

#include "engine/ServiceHandle.hpp"
#include "math/Scalars.hpp"
#include "memory/Containers.hpp"
#include "memory/MemoryService.hpp"

#include <algorithm>
#include <functional>
#include <string>
#include <string_view>

namespace ndde {

using HotkeyId = u64;
using HotkeyHandle = ServiceHandle;

struct KeyChord {
    int key = 0;
    int mods = 0;

    [[nodiscard]] friend bool operator==(KeyChord a, KeyChord b) noexcept {
        return a.key == b.key && a.mods == b.mods;
    }
};

struct HotkeyDescriptor {
    KeyChord chord{};
    std::string label;
    std::string category = "Simulation";
    std::function<void()> callback;
};

class HotkeyService {
public:
    void set_memory_service(memory::MemoryService* memory) {
        std::pmr::memory_resource* resource = memory ? memory->persistent().resource()
                                                     : std::pmr::get_default_resource();
        if (resource == m_hotkeys.get_allocator().resource())
            return;
        std::destroy_at(&m_hotkeys);
        std::construct_at(&m_hotkeys, resource);
    }

    [[nodiscard]] HotkeyHandle register_action(HotkeyDescriptor descriptor) {
        const HotkeyId id = m_next_id++;
        m_hotkeys.push_back(HotkeyEntry{
            .id = id,
            .descriptor = std::move(descriptor),
            .active = true
        });
        return HotkeyHandle([this, id] { unregister(id); });
    }

    [[nodiscard]] bool dispatch(KeyChord chord) {
        bool handled = false;
        for (auto& entry : m_hotkeys) {
            if (!entry.active || entry.descriptor.chord != chord || !entry.descriptor.callback) continue;
            entry.descriptor.callback();
            handled = true;
        }
        return handled;
    }

    [[nodiscard]] std::size_t active_count() const noexcept {
        return static_cast<std::size_t>(std::count_if(m_hotkeys.begin(), m_hotkeys.end(),
            [](const HotkeyEntry& entry) { return entry.active; }));
    }

    [[nodiscard]] bool contains(std::string_view label) const {
        return std::any_of(m_hotkeys.begin(), m_hotkeys.end(),
            [label](const HotkeyEntry& entry) {
                return entry.active && entry.descriptor.label == label;
            });
    }

private:
    struct HotkeyEntry {
        HotkeyId id = 0;
        HotkeyDescriptor descriptor;
        bool active = false;
    };

    HotkeyId m_next_id = 1;
    memory::PersistentVector<HotkeyEntry> m_hotkeys;

    void unregister(HotkeyId id) noexcept {
        for (auto& entry : m_hotkeys) {
            if (entry.id == id) {
                entry.active = false;
                entry.descriptor.callback = {};
                return;
            }
        }
    }
};

} // namespace ndde

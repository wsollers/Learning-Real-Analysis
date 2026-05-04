#pragma once
// memory/MemoryService.hpp
// Central engine memory component.
//
// This is the public allocation surface for engine/app code.  Lower-level
// allocators such as BufferManager are implementation details of the service.
// Frame-lifetime CPU vectors are PMR-backed through this service.  Longer
// lifetime aliases still begin as std-backed containers and will migrate one
// scope at a time.

#include "math/GeometryTypes.hpp"
#include "math/Scalars.hpp"
#include "memory/ArenaSlice.hpp"
#include "memory/BufferManager.hpp"
#include "memory/Containers.hpp"

#include <memory>
#include <memory_resource>
#include <string_view>
#include <utility>

#include <volk.h>

namespace ndde::memory {

enum class MemoryLifetime : u8 {
    Frame,
    View,
    Simulation,
    Cache,
    History,
    Persistent
};

template <MemoryLifetime Lifetime>
struct LifetimeVectorPolicy;

template <>
struct LifetimeVectorPolicy<MemoryLifetime::Frame> {
    template <class T> using Vector = FrameVector<T>;
};

template <>
struct LifetimeVectorPolicy<MemoryLifetime::View> {
    template <class T> using Vector = ViewVector<T>;
};

template <>
struct LifetimeVectorPolicy<MemoryLifetime::Simulation> {
    template <class T> using Vector = SimVector<T>;
};

template <>
struct LifetimeVectorPolicy<MemoryLifetime::Cache> {
    template <class T> using Vector = CacheVector<T>;
};

template <>
struct LifetimeVectorPolicy<MemoryLifetime::History> {
    template <class T> using Vector = HistoryVector<T>;
};

template <>
struct LifetimeVectorPolicy<MemoryLifetime::Persistent> {
    template <class T> using Vector = PersistentVector<T>;
};

template <MemoryLifetime Lifetime>
class MemoryScope {
public:
    using Policy = LifetimeVectorPolicy<Lifetime>;

    explicit MemoryScope(std::string_view name,
                         std::pmr::memory_resource* resource = std::pmr::get_default_resource()) noexcept
        : m_name(name)
        , m_resource(resource)
    {}

    [[nodiscard]] std::string_view name() const noexcept { return m_name; }
    [[nodiscard]] u64 generation() const noexcept { return m_generation; }

    void reset() noexcept { ++m_generation; }
    void set_resource(std::pmr::memory_resource* resource) noexcept { m_resource = resource; }
    [[nodiscard]] std::pmr::memory_resource* resource() const noexcept { return m_resource; }

    template <class T>
    [[nodiscard]] typename Policy::template Vector<T> make_vector() const {
        if constexpr (Lifetime == MemoryLifetime::Frame) {
            return typename Policy::template Vector<T>{m_resource};
        } else {
            return {};
        }
    }

    template <class T>
    [[nodiscard]] typename Policy::template Vector<T> make_vector(std::size_t count) const {
        if constexpr (Lifetime == MemoryLifetime::Frame) {
            return typename Policy::template Vector<T>(count, m_resource);
        } else {
            return typename Policy::template Vector<T>(count);
        }
    }

    template <class T, class... Args>
    [[nodiscard]] std::unique_ptr<T> make_unique(Args&&... args) const {
        return std::make_unique<T>(std::forward<Args>(args)...);
    }

private:
    std::string_view m_name;
    std::pmr::memory_resource* m_resource = std::pmr::get_default_resource();
    u64 m_generation = 0;
};

using FrameMemoryScope = MemoryScope<MemoryLifetime::Frame>;
using ViewMemoryScope = MemoryScope<MemoryLifetime::View>;
using SimulationMemoryScope = MemoryScope<MemoryLifetime::Simulation>;
using CacheMemoryScope = MemoryScope<MemoryLifetime::Cache>;
using HistoryMemoryScope = MemoryScope<MemoryLifetime::History>;
using PersistentMemoryScope = MemoryScope<MemoryLifetime::Persistent>;

class MemoryService {
public:
    MemoryService() = default;
    ~MemoryService() = default;

    MemoryService(const MemoryService&) = delete;
    MemoryService& operator=(const MemoryService&) = delete;
    MemoryService(MemoryService&&) = delete;
    MemoryService& operator=(MemoryService&&) = delete;

    void init_frame_gpu_arena(VkDevice device, VkPhysicalDevice physical_device, u32 size_mb);
    void destroy();

    void begin_frame() noexcept;
    void reset_view() noexcept { m_view.reset(); }
    void reset_simulation() noexcept { m_simulation.reset(); }
    void reset_cache() noexcept { m_cache.reset(); }
    void reset_history() noexcept { m_history.reset(); }

    [[nodiscard]] FrameMemoryScope& frame() noexcept { return m_frame; }
    [[nodiscard]] const FrameMemoryScope& frame() const noexcept { return m_frame; }
    [[nodiscard]] ViewMemoryScope& view() noexcept { return m_view; }
    [[nodiscard]] const ViewMemoryScope& view() const noexcept { return m_view; }
    [[nodiscard]] SimulationMemoryScope& simulation() noexcept { return m_simulation; }
    [[nodiscard]] const SimulationMemoryScope& simulation() const noexcept { return m_simulation; }
    [[nodiscard]] CacheMemoryScope& cache() noexcept { return m_cache; }
    [[nodiscard]] const CacheMemoryScope& cache() const noexcept { return m_cache; }
    [[nodiscard]] HistoryMemoryScope& history() noexcept { return m_history; }
    [[nodiscard]] const HistoryMemoryScope& history() const noexcept { return m_history; }
    [[nodiscard]] PersistentMemoryScope& persistent() noexcept { return m_persistent; }
    [[nodiscard]] const PersistentMemoryScope& persistent() const noexcept { return m_persistent; }

    [[nodiscard]] ArenaSlice allocate_frame_vertices(u32 vertex_count) {
        return m_frame_gpu.acquire(vertex_count);
    }

    [[nodiscard]] u64 frame_gpu_bytes_used() const noexcept { return m_frame_gpu.bytes_used(); }
    [[nodiscard]] u64 frame_gpu_bytes_total() const noexcept { return m_frame_gpu.bytes_total(); }
    [[nodiscard]] f32 frame_gpu_utilisation() const noexcept { return m_frame_gpu.utilisation(); }

private:
    FrameMemoryScope m_frame{"Frame"};
    ViewMemoryScope m_view{"View"};
    SimulationMemoryScope m_simulation{"Simulation"};
    CacheMemoryScope m_cache{"Cache"};
    HistoryMemoryScope m_history{"History"};
    PersistentMemoryScope m_persistent{"Persistent"};
    std::pmr::monotonic_buffer_resource m_frame_cpu{1024u * 1024u * 16u};
    BufferManager m_frame_gpu;
};

} // namespace ndde::memory

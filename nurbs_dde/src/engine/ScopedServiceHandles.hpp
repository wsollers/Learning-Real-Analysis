#pragma once
// engine/ScopedServiceHandles.hpp
// Plain RAII owner for several service registration handles.

#include <utility>
#include <vector>

namespace ndde {

template <class Handle>
class ScopedServiceHandles {
public:
    ScopedServiceHandles() = default;
    ScopedServiceHandles(const ScopedServiceHandles&) = delete;
    ScopedServiceHandles& operator=(const ScopedServiceHandles&) = delete;
    ScopedServiceHandles(ScopedServiceHandles&&) noexcept = default;
    ScopedServiceHandles& operator=(ScopedServiceHandles&&) noexcept = default;

    ~ScopedServiceHandles() = default;

    void add(Handle handle) {
        if (handle)
            m_handles.push_back(std::move(handle));
    }

    void clear() noexcept {
        m_handles.clear();
    }

    [[nodiscard]] std::size_t size() const noexcept { return m_handles.size(); }
    [[nodiscard]] bool empty() const noexcept { return m_handles.empty(); }

private:
    std::vector<Handle> m_handles;
};

} // namespace ndde

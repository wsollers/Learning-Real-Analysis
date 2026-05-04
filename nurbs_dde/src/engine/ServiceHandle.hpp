#pragma once
// engine/ServiceHandle.hpp
// Move-only RAII registration handle used by engine-owned services.

#include <functional>
#include <utility>

namespace ndde {

class ServiceHandle {
public:
    ServiceHandle() = default;
    explicit ServiceHandle(std::function<void()> release) : m_release(std::move(release)) {}

    ~ServiceHandle() { reset(); }

    ServiceHandle(const ServiceHandle&) = delete;
    ServiceHandle& operator=(const ServiceHandle&) = delete;

    ServiceHandle(ServiceHandle&& other) noexcept
        : m_release(std::move(other.m_release))
    {
        other.m_release = {};
    }

    ServiceHandle& operator=(ServiceHandle&& other) noexcept {
        if (this == &other) return *this;
        reset();
        m_release = std::move(other.m_release);
        other.m_release = {};
        return *this;
    }

    void reset() noexcept {
        if (!m_release) return;
        auto release = std::move(m_release);
        m_release = {};
        release();
    }

    [[nodiscard]] explicit operator bool() const noexcept {
        return static_cast<bool>(m_release);
    }

private:
    std::function<void()> m_release;
};

} // namespace ndde


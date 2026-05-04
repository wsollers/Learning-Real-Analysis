#pragma once
// memory/Unique.hpp
// Lifetime-aware object ownership for MemoryService scopes.
//
// memory::Unique<T> is intentionally still unique_ptr-shaped, but its storage is
// obtained from a std::pmr::memory_resource instead of the global heap.  The
// deleter carries the allocation resource and the concrete destructor needed
// when a derived object is stored behind a base-class pointer.

#include <cstddef>
#include <memory>
#include <memory_resource>
#include <new>
#include <type_traits>
#include <utility>

namespace ndde::memory {

template <class T>
class UniqueDeleter {
public:
    UniqueDeleter() = default;

    template <class U>
        requires std::is_convertible_v<U*, T*>
    UniqueDeleter(const UniqueDeleter<U>& other) noexcept
        : m_resource(other.m_resource)
        , m_bytes(other.m_bytes)
        , m_alignment(other.m_alignment)
        , m_destroy([](T* ptr) noexcept {
            std::destroy_at(static_cast<U*>(ptr));
        })
    {}

    void operator()(T* ptr) const noexcept {
        if (!ptr) return;
        if (m_destroy)
            m_destroy(ptr);
        if (m_resource)
            m_resource->deallocate(ptr, m_bytes, m_alignment);
    }

    template <class U>
        requires std::is_convertible_v<U*, T*>
    [[nodiscard]] static UniqueDeleter for_type(std::pmr::memory_resource* resource) noexcept {
        UniqueDeleter deleter;
        deleter.m_resource = resource;
        deleter.m_bytes = sizeof(U);
        deleter.m_alignment = alignof(U);
        deleter.m_destroy = [](T* ptr) noexcept {
            std::destroy_at(static_cast<U*>(ptr));
        };
        return deleter;
    }

private:
    template <class>
    friend class UniqueDeleter;

    std::pmr::memory_resource* m_resource = nullptr;
    std::size_t m_bytes = 0;
    std::size_t m_alignment = alignof(T);
    void (*m_destroy)(T*) noexcept = nullptr;
};

template <class T>
using Unique = std::unique_ptr<T, UniqueDeleter<T>>;

template <class T, class... Args>
[[nodiscard]] Unique<T> make_unique(std::pmr::memory_resource* resource, Args&&... args) {
    if (!resource)
        resource = std::pmr::get_default_resource();

    void* storage = resource->allocate(sizeof(T), alignof(T));
    try {
        T* object = std::construct_at(static_cast<T*>(storage), std::forward<Args>(args)...);
        return Unique<T>(object, UniqueDeleter<T>::template for_type<T>(resource));
    } catch (...) {
        resource->deallocate(storage, sizeof(T), alignof(T));
        throw;
    }
}

} // namespace ndde::memory

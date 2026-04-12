#pragma once
// memory/ArenaSlice.hpp
// The "receipt" returned by BufferManager::acquire().
// Kept separate so the math layer never needs to include BufferManager.hpp
// (which would pull in Vulkan headers into pure math translation units).

#include <volk.h>
#include "math/Types.hpp"

namespace ndde::memory {

struct ArenaSlice {
    VkBuffer buffer       = VK_NULL_HANDLE;
    void*    data         = nullptr;
    u32      byte_offset  = 0;
    u32      vertex_count = 0;

    [[nodiscard]] Vertex* vertices() const noexcept {
        return static_cast<Vertex*>(data);
    }

    [[nodiscard]] bool valid() const noexcept {
        return buffer != VK_NULL_HANDLE && data != nullptr;
    }
};

} // namespace ndde::memory

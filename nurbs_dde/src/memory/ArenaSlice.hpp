#pragma once
// memory/ArenaSlice.hpp
// The "receipt" returned by BufferManager::acquire().
// Includes renderer/GpuTypes.hpp (not math/Scalars.hpp) because ArenaSlice
// exposes Vertex* — a GPU-contract type. Pure-math and ndde_numeric code
// must never include this header.

#include <volk.h>
#include "renderer/GpuTypes.hpp"

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

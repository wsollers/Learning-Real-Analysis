#pragma once
#include <volk.h>
#include <atomic>
#include "core/Types.hpp"

namespace ndde {

    // The "receipt" handed to the Math engine
    struct ArenaSlice {
        VkBuffer buffer;
        void* data;          // CPU-writable pointer
        uint32_t offset;     // GPU-readable offset
        uint32_t vertexCount;
    };

    struct ArenaPool {
        VkBuffer buffer = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
        void* mappedBase = nullptr;
        VkDeviceSize totalSize = 0;
        std::atomic<VkDeviceSize> currentOffset{0};
    };

    class BufferManager {
    public:
        BufferManager(VkDevice device, VkPhysicalDevice physDevice);
        ~BufferManager();

        // The thread-safe "Pop"
        ArenaSlice acquire_large_slice(uint32_t vertexCount);
        void reset_pools();

    private:
        VkDevice m_device;
        ArenaPool m_large_pool;

        void create_pool(VkDeviceSize size, ArenaPool& pool, VkPhysicalDevice physDevice);
        uint32_t find_memory_type(VkPhysicalDevice phys, uint32_t typeFilter, VkMemoryPropertyFlags properties);
    };

} // namespace ndde
#include "core/BufferManager.hpp"
#include <stdexcept>

ndde::BufferManager::BufferManager(VkDevice device, VkPhysicalDevice physDevice)
    : m_device(device)
{
    // 128MB Pool for high-density geometry
    create_pool(128 * 1024 * 1024, m_large_pool, physDevice);
}

ndde::BufferManager::~BufferManager() {
    if (m_large_pool.mappedBase) vkUnmapMemory(m_device, m_large_pool.memory);
    if (m_large_pool.buffer) vkDestroyBuffer(m_device, m_large_pool.buffer, nullptr);
    if (m_large_pool.memory) vkFreeMemory(m_device, m_large_pool.memory, nullptr);
}

ndde::ArenaSlice ndde::BufferManager::acquire_large_slice(uint32_t vertexCount) {
    VkDeviceSize size = vertexCount * sizeof(Vertex);
    // Align to 64 bytes to prevent "misaligned access" GPU hangs
    VkDeviceSize alignedSize = (size + 63) & ~63;

    // ATOMIC POP: Multiple threads can call this safely
    VkDeviceSize startOffset = m_large_pool.currentOffset.fetch_add(alignedSize);

    if (startOffset + size > m_large_pool.totalSize) {
        throw std::runtime_error("BufferManager: VRAM Arena Overflow! Increase pool size.");
    }

    ndde::ArenaSlice slice;
    slice.buffer = m_large_pool.buffer;
    // Hand out the direct pointer for Zero-Copy math
    slice.data = static_cast<char*>(m_large_pool.mappedBase) + startOffset;
    slice.offset = static_cast<uint32_t>(startOffset);
    slice.vertexCount = vertexCount;

    return slice;
}

void ndde::BufferManager::reset_pools() {
    // Reset the "conveyor belt" back to the start
    m_large_pool.currentOffset.store(0);
}

void ndde::BufferManager::create_pool(VkDeviceSize size, ArenaPool& pool, VkPhysicalDevice physDevice) {
    pool.totalSize = size;

    VkBufferCreateInfo bufferInfo{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = size,
        .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE
    };

    if (vkCreateBuffer(m_device, &bufferInfo, nullptr, &pool.buffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to create buffer!");
    }

    VkMemoryRequirements memReqs;
    vkGetBufferMemoryRequirements(m_device, pool.buffer, &memReqs);

    VkMemoryAllocateInfo allocInfo{
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memReqs.size,
        .memoryTypeIndex = find_memory_type(physDevice, memReqs.memoryTypeBits,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
    };

    if (vkAllocateMemory(m_device, &allocInfo, nullptr, &pool.memory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate buffer memory!");
    }

    vkBindBufferMemory(m_device, pool.buffer, pool.memory, 0);
    vkMapMemory(m_device, pool.memory, 0, size, 0, &pool.mappedBase);
}

uint32_t ndde::BufferManager::find_memory_type(VkPhysicalDevice phys, uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(phys, &memProperties);
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    return 0;
}
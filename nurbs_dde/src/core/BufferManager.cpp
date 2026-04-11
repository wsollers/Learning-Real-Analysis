//
// Created by wsoll on 4/11/2026.
//
#include "core/BufferManager.hpp"

#include "core/BufferManager.hpp"
#include <stdexcept>

namespace ndde {

    // Helper to find the right type of memory on your specific GPU
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties, VkPhysicalDevice physicalDevice) {
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
            if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }
        throw std::runtime_error("failed to find suitable memory type!");
    }

    void BufferManager::upload_curve_data(const std::vector<Vertex>& vertices, VkDevice device, VkPhysicalDevice physicalDevice) {
        m_vertices = vertices;
        VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

        // 1. Create the Buffer
        VkBufferCreateInfo bufferInfo{.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
        bufferInfo.size = bufferSize;
        bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(device, &bufferInfo, nullptr, &m_vertexBuffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to create vertex buffer!");
        }

        // 2. Allocate Memory
        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(device, m_vertexBuffer, &memRequirements);

        VkMemoryAllocateInfo allocInfo{.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, physicalDevice);

        if (vkAllocateMemory(device, &allocInfo, nullptr, &m_vertexBufferMemory) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate vertex buffer memory!");
        }

        // 3. Bind and Map (Copy CPU data to GPU memory)
        vkBindBufferMemory(device, m_vertexBuffer, m_vertexBufferMemory, 0);

        void* data;
        vkMapMemory(device, m_vertexBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, m_vertices.data(), (size_t)bufferSize);
        vkUnmapMemory(device, m_vertexBufferMemory);
    }


    BufferManager::~BufferManager() {
        // We will eventually put vkDestroyBuffer here
    }
}
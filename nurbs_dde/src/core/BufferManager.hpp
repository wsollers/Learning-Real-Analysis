//
// Created by wsoll on 4/11/2026.
//
#pragma once

#ifndef NURBS_DDE_BUFFERMANAGER_HPP
#define NURBS_DDE_BUFFERMANAGER_HPP

#include "core/Types.hpp"
#include <vector>
#include <volk.h>

namespace ndde {
    class BufferManager {
    public:
        // Uploads a vector of vertices to the GPU
        void upload_curve_data(const std::vector<Vertex>& vertices, VkDevice device, VkPhysicalDevice physicalDevice);

        VkBuffer get_buffer() const { return m_vertexBuffer; }
        u32 get_vertex_count() const { return static_cast<u32>(m_vertices.size()); }

        ~BufferManager();

    private:
        VkBuffer m_vertexBuffer = VK_NULL_HANDLE;
        VkDeviceMemory m_vertexBufferMemory = VK_NULL_HANDLE;
        std::vector<Vertex> m_vertices;

        void create_buffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory, VkDevice device, VkPhysicalDevice physicalDevice);
    };
}
#endif //NURBS_DDE_BUFFERMANAGER_HPP

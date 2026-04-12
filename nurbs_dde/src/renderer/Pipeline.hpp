#pragma once
// renderer/Pipeline.hpp
// Builds a graphics pipeline for line rendering.
// Topology (LINE_STRIP or LINE_LIST) is a parameter so we can share
// the same shaders for both curve and axes drawing.
// Uses VK_KHR_dynamic_rendering — no VkRenderPass required.

#include "VulkanContext.hpp"
#include <string>
#include <vector>

namespace ndde {

class Pipeline {
public:
    Pipeline() = default;
    ~Pipeline();

    Pipeline(const Pipeline&) = delete;
    Pipeline& operator=(const Pipeline&) = delete;

    void init(VkDevice device,
              VkFormat color_format,
              const std::string& vert_spv_path,
              const std::string& frag_spv_path,
              VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP);

    void destroy(VkDevice device);

    [[nodiscard]] VkPipeline       pipeline() const { return m_pipeline; }
    [[nodiscard]] VkPipelineLayout layout()   const { return m_layout; }

private:
    VkPipeline       m_pipeline = VK_NULL_HANDLE;
    VkPipelineLayout m_layout   = VK_NULL_HANDLE;
    VkDevice         m_device   = VK_NULL_HANDLE;

    static std::vector<char> read_spv(const std::string& path);
    static VkShaderModule    create_shader_module(VkDevice device,
                                                   const std::vector<char>& code);
};

} // namespace ndde::renderer

#include "Pipeline.hpp"
#include "core/Types.hpp"
#include <fstream>
#include <vector>
#include <stdexcept>

namespace ndde {

void Pipeline::init(VkDevice device,
                    VkFormat color_format,
                    const std::string& vert_spv_path,
                    const std::string& frag_spv_path,
                    VkPrimitiveTopology topology)
{
    m_device = device;

    // 1. Shader modules
    auto vert_code = read_spv(vert_spv_path);
    auto frag_code = read_spv(frag_spv_path);

    VkShaderModule vert_mod = create_shader_module(device, vert_code);
    VkShaderModule frag_mod = create_shader_module(device, frag_code);

    VkPipelineShaderStageCreateInfo stages[2] = {};
    stages[0].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[0].stage  = VK_SHADER_STAGE_VERTEX_BIT;
    stages[0].module = vert_mod;
    stages[0].pName  = "main";

    stages[1].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[1].stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
    stages[1].module = frag_mod;
    stages[1].pName  = "main";

    // 2. Vertex input
    auto bindingDescription = Vertex::getBindingDescription();
    auto attributeDescriptions = Vertex::getAttributeDescriptions();

    VkPipelineVertexInputStateCreateInfo vert_input{};
    vert_input.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vert_input.vertexBindingDescriptionCount   = 1;
    vert_input.pVertexBindingDescriptions      = &bindingDescription;
    vert_input.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vert_input.pVertexAttributeDescriptions    = attributeDescriptions.data();

    // 3. Input assembly
    VkPipelineInputAssemblyStateCreateInfo input_asm{};
    input_asm.sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_asm.topology               = topology;
    input_asm.primitiveRestartEnable = VK_FALSE;

    // 4. Viewport / Scissor (Dynamic)
    VkPipelineViewportStateCreateInfo viewport_state{};
    viewport_state.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state.viewportCount = 1;
    viewport_state.scissorCount  = 1;

    // 5. Rasteriser
    VkPipelineRasterizationStateCreateInfo raster{};
    raster.sType       = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    raster.polygonMode = VK_POLYGON_MODE_FILL;
    raster.cullMode    = VK_CULL_MODE_NONE;
    raster.frontFace   = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    raster.lineWidth   = 1.0f;

    // 6. MSAA
    VkPipelineMultisampleStateCreateInfo msaa{};
    msaa.sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    msaa.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    // 7. Colour blend
    VkPipelineColorBlendAttachmentState blend_att{};
    blend_att.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                               VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    blend_att.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo blend{};
    blend.sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    blend.attachmentCount = 1;
    blend.pAttachments    = &blend_att;

    // 8. Dynamic State
    VkDynamicState dyn_states[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo dynamic{};
    dynamic.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic.dynamicStateCount = 2;
    dynamic.pDynamicStates    = dyn_states;

    // 9. Layout & Push Constants
    VkPushConstantRange push_range{};
    push_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    push_range.offset     = 0;
    push_range.size       = sizeof(PushConstants); 

    VkPipelineLayoutCreateInfo layout_info{.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    layout_info.pushConstantRangeCount = 1;
    layout_info.pPushConstantRanges    = &push_range;

    if (vkCreatePipelineLayout(device, &layout_info, nullptr, &m_layout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create pipeline layout");
    }

    // 10. Dynamic Rendering Metadata
    VkPipelineRenderingCreateInfo dyn_render{};
    dyn_render.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    dyn_render.colorAttachmentCount    = 1;
    dyn_render.pColorAttachmentFormats = &color_format;

    // 11. Final Assembly
    VkGraphicsPipelineCreateInfo pipe_info{.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
    pipe_info.pNext               = &dyn_render;
    pipe_info.stageCount          = 2;
    pipe_info.pStages             = stages;
    pipe_info.pVertexInputState   = &vert_input;
    pipe_info.pInputAssemblyState = &input_asm;
    pipe_info.pViewportState      = &viewport_state;
    pipe_info.pRasterizationState = &raster;
    pipe_info.pMultisampleState   = &msaa;
    pipe_info.pColorBlendState    = &blend;
    pipe_info.pDynamicState       = &dynamic;
    pipe_info.layout              = m_layout;
    pipe_info.renderPass          = VK_NULL_HANDLE;

    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipe_info, nullptr, &m_pipeline) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create graphics pipeline");
    }

    vkDestroyShaderModule(device, vert_mod, nullptr);
    vkDestroyShaderModule(device, frag_mod, nullptr);
} // End of init()

void Pipeline::destroy(VkDevice device) {
    if (m_pipeline != VK_NULL_HANDLE) vkDestroyPipeline(device, m_pipeline, nullptr);
    if (m_layout != VK_NULL_HANDLE) vkDestroyPipelineLayout(device, m_layout, nullptr);
}

std::vector<char> Pipeline::read_spv(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);
    if (!file.is_open()) throw std::runtime_error("failed to open file: " + filename);
    size_t fileSize = (size_t)file.tellg();
    std::vector<char> buffer(fileSize);
    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();
    return buffer;
}

VkShaderModule Pipeline::create_shader_module(VkDevice device, const std::vector<char>& code) {
    VkShaderModuleCreateInfo createInfo{.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        throw std::runtime_error("failed to create shader module!");
    }
    return shaderModule;
}

    Pipeline::~Pipeline() {
        // You can leave this empty or call destroy(m_device) if you want
        // to be extra safe, provided m_device is valid.
    }
} // namespace ndde
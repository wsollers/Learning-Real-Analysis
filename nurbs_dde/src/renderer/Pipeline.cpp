// renderer/Pipeline.cpp
// Graphics pipeline for line-strip curve rendering.
// Uses VK_KHR_dynamic_rendering — no VkRenderPass required.

#include "Pipeline.hpp"
#include <fstream>
#include <stdexcept>
#include <vector>

#include "volk.h"

namespace ndde::renderer {

// ── Destructor ─────────────────────────────────────────────────────────────────

Pipeline::~Pipeline() {
    if (m_device != VK_NULL_HANDLE) destroy(m_device);
}

// ── Public init ────────────────────────────────────────────────────────────────

void Pipeline::init(VkDevice device,
                    VkFormat color_format,
                    const std::string& vert_spv_path,
                    const std::string& frag_spv_path,
                    VkPrimitiveTopology topology)
{
    m_device = device;

    // ── Shader modules ─────────────────────────────────────────────────────────
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

    // ── Vertex input ───────────────────────────────────────────────────────────
    // Vertices are vec4 (x,y,z,w) — one binding, one attribute.
    VkVertexInputBindingDescription binding{};
    binding.binding   = 0;
    binding.stride    = sizeof(float) * 4;
    binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription attrib{};
    attrib.binding  = 0;
    attrib.location = 0;
    attrib.format   = VK_FORMAT_R32G32B32A32_SFLOAT;
    attrib.offset   = 0;

    VkPipelineVertexInputStateCreateInfo vert_input{};
    vert_input.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vert_input.vertexBindingDescriptionCount   = 1;
    vert_input.pVertexBindingDescriptions      = &binding;
    vert_input.vertexAttributeDescriptionCount = 1;
    vert_input.pVertexAttributeDescriptions    = &attrib;

    // ── Input assembly — topology is a parameter (LINE_STRIP or LINE_LIST) ────
    VkPipelineInputAssemblyStateCreateInfo input_asm{};
    input_asm.sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_asm.topology               = topology;
    input_asm.primitiveRestartEnable = VK_FALSE;

    // ── Viewport / scissor (dynamic) ───────────────────────────────────────────
    VkPipelineViewportStateCreateInfo viewport_state{};
    viewport_state.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state.viewportCount = 1;
    viewport_state.scissorCount  = 1;

    // ── Rasteriser ─────────────────────────────────────────────────────────────
    VkPipelineRasterizationStateCreateInfo raster{};
    raster.sType       = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    raster.polygonMode = VK_POLYGON_MODE_FILL;
    raster.cullMode    = VK_CULL_MODE_NONE;
    raster.frontFace   = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    raster.lineWidth   = 1.0f;

    // ── MSAA (off) ─────────────────────────────────────────────────────────────
    VkPipelineMultisampleStateCreateInfo msaa{};
    msaa.sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    msaa.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    // ── Colour blend (opaque) ──────────────────────────────────────────────────
    VkPipelineColorBlendAttachmentState blend_att{};
    blend_att.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    blend_att.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo blend{};
    blend.sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    blend.attachmentCount = 1;
    blend.pAttachments    = &blend_att;

    // ── Dynamic state — viewport + scissor ────────────────────────────────────
    VkDynamicState dyn_states[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };
    VkPipelineDynamicStateCreateInfo dynamic{};
    dynamic.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic.dynamicStateCount = 2;
    dynamic.pDynamicStates    = dyn_states;

    // ── Push constants: MVP matrix (4×4 float = 64 bytes) ─────────────────────
    VkPushConstantRange push_range{};
    push_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    push_range.offset     = 0;
    push_range.size       = sizeof(float) * 16 + sizeof(int) + sizeof(float) * 3; // mat4 + mode + colour

    VkPipelineLayoutCreateInfo layout_info{};
    layout_info.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layout_info.pushConstantRangeCount = 1;
    layout_info.pPushConstantRanges    = &push_range;

    if (vkCreatePipelineLayout(device, &layout_info, nullptr, &m_layout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create pipeline layout");
    }

    // ── Dynamic rendering attachment ───────────────────────────────────────────
    VkPipelineRenderingCreateInfo dyn_render{};
    dyn_render.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    dyn_render.colorAttachmentCount    = 1;
    dyn_render.pColorAttachmentFormats = &color_format;

    // ── Assemble ───────────────────────────────────────────────────────────────
    VkGraphicsPipelineCreateInfo pipe_info{};
    pipe_info.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipe_info.pNext               = &dyn_render;   // chain dynamic rendering
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
    pipe_info.renderPass          = VK_NULL_HANDLE; // dynamic rendering — no pass

    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1,
                                   &pipe_info, nullptr, &m_pipeline) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create graphics pipeline");
    }

    // Shader modules are no longer needed after pipeline creation
    vkDestroyShaderModule(device, vert_mod, nullptr);
    vkDestroyShaderModule(device, frag_mod, nullptr);
}

// ── destroy ────────────────────────────────────────────────────────────────────

void Pipeline::destroy(VkDevice device) {
    if (m_pipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(device, m_pipeline, nullptr);
        m_pipeline = VK_NULL_HANDLE;
    }
    if (m_layout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(device, m_layout, nullptr);
        m_layout = VK_NULL_HANDLE;
    }
    m_device = VK_NULL_HANDLE;
}

// ── Private helpers ────────────────────────────────────────────────────────────

std::vector<char> Pipeline::read_spv(const std::string& path) {
    std::ifstream file(path, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open SPIR-V: " + path);
    }
    std::size_t size = static_cast<std::size_t>(file.tellg());
    std::vector<char> buf(size);
    file.seekg(0);
    file.read(buf.data(), static_cast<std::streamsize>(size));
    return buf;
}

VkShaderModule Pipeline::create_shader_module(VkDevice device,
                                               const std::vector<char>& code)
{
    VkShaderModuleCreateInfo info{};
    info.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    info.codeSize = code.size();
    // SPIR-V requires uint32 alignment — reinterpret_cast is well-defined here
    // because std::vector guarantees alignment for scalar types.
    info.pCode    = reinterpret_cast<const std::uint32_t*>(code.data());

    VkShaderModule mod;
    if (vkCreateShaderModule(device, &info, nullptr, &mod) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create shader module");
    }
    return mod;
}

} // namespace ndde::renderer

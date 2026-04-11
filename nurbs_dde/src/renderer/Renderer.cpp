// renderer/Renderer.cpp
#include "Renderer.hpp"
#include <stdexcept>
#include <cstring>
#include <algorithm>
#include <iostream>

namespace ndde::renderer {

Renderer::~Renderer() { destroy(); }

// ── init ───────────────────────────────────────────────────────────────────────

void Renderer::init(GLFWwindow* window,
                    const VulkanContext& ctx,
                    Swapchain& swapchain,
                    const std::string& vert_spv,
                    const std::string& frag_spv,
                    const std::string& assets_dir)
{
    m_device = ctx.device();

    m_curve_pipeline.init(ctx.device(), swapchain.format(),
                          vert_spv, frag_spv,
                          VK_PRIMITIVE_TOPOLOGY_LINE_STRIP);

    m_hyperbola_pipeline.init(ctx.device(), swapchain.format(),
                              vert_spv, frag_spv,
                              VK_PRIMITIVE_TOPOLOGY_LINE_STRIP);

    m_axes_pipeline.init(ctx.device(), swapchain.format(),
                         vert_spv, frag_spv,
                         VK_PRIMITIVE_TOPOLOGY_LINE_LIST);

    m_ball_pipeline.init(ctx.device(), swapchain.format(),
                         vert_spv, frag_spv,
                         VK_PRIMITIVE_TOPOLOGY_LINE_STRIP);

    m_imgui.init(window, ctx, swapchain, assets_dir);
    create_frames(ctx.device(), ctx.queue_families().graphics);
}

// ── destroy ────────────────────────────────────────────────────────────────────

void Renderer::destroy() {
    if (m_device == VK_NULL_HANDLE) return;
    vkDeviceWaitIdle(m_device);
    m_imgui.destroy();
    for (auto& c : m_curves) destroy_buffer(m_device, c.buf);
    m_curves.clear();
    destroy_buffer(m_device, m_axes_buf);
    destroy_buffer(m_device, m_epsilon_buf);
    destroy_buffer(m_device, m_interval_buf);
    destroy_buffer(m_device, m_centre_buf);
    destroy_buffer(m_device, m_secant_buf);
    destroy_frames(m_device);
    m_curve_pipeline.destroy(m_device);
    m_hyperbola_pipeline.destroy(m_device);
    m_axes_pipeline.destroy(m_device);
    m_ball_pipeline.destroy(m_device);
    m_device = VK_NULL_HANDLE;
}

// ── on_resize ─────────────────────────────────────────────────────────────────

void Renderer::on_resize(const VulkanContext& ctx,
                          Swapchain& swapchain,
                          int /*width*/, int /*height*/)
{
    vkDeviceWaitIdle(ctx.device());
    swapchain.recreate(ctx, swapchain.extent().width, swapchain.extent().height);
    m_imgui.on_resize(swapchain);
}

// ── upload helpers ─────────────────────────────────────────────────────────────

void Renderer::upload_to_buffer(const VulkanContext& ctx,
                                  GpuBuffer& buf,
                                  std::span<const math::Vec4> verts,
                                  std::uint32_t& count)
{
    vkDeviceWaitIdle(ctx.device());
    destroy_buffer(ctx.device(), buf);
    if (verts.empty()) { count = 0; return; }

    const VkDeviceSize bytes = verts.size_bytes();
    buf = create_buffer(ctx, bytes,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    void* mapped = nullptr;
    vkMapMemory(ctx.device(), buf.memory, 0, bytes, 0, &mapped);
    std::memcpy(mapped, verts.data(), static_cast<std::size_t>(bytes));
    vkUnmapMemory(ctx.device(), buf.memory);
    count = static_cast<std::uint32_t>(verts.size());
}

void Renderer::upload_curve(const VulkanContext& ctx,
                             std::size_t idx,
                             std::span<const math::Vec4> verts,
                             const float colour[3],
                             bool is_hyperbola)
{
    if (idx >= m_curves.size()) m_curves.resize(idx + 1);
    CurveBuffer& cb = m_curves[idx];
    upload_to_buffer(ctx, cb.buf, verts, cb.total_count);
    cb.visible_count = 0;
    cb.colour[0] = colour[0];
    cb.colour[1] = colour[1];
    cb.colour[2] = colour[2];
    cb.is_hyperbola = is_hyperbola;
}

void Renderer::set_curve_fraction(std::size_t idx, float fraction)
{
    if (idx >= m_curves.size()) return;
    CurveBuffer& cb = m_curves[idx];
    fraction = std::clamp(fraction, 0.f, 1.f);
    cb.visible_count = static_cast<std::uint32_t>(
        fraction * static_cast<float>(cb.total_count));
}

void Renderer::remove_curve(const VulkanContext& ctx, std::size_t idx)
{
    if (idx >= m_curves.size()) return;
    vkDeviceWaitIdle(ctx.device());
    destroy_buffer(ctx.device(), m_curves[idx].buf);
    m_curves[idx] = {};
}

void Renderer::upload_axes(const VulkanContext& ctx,
                            std::span<const math::Vec4> verts)
{
    upload_to_buffer(ctx, m_axes_buf, verts, m_axes_count);
}

void Renderer::upload_epsilon_ball(const VulkanContext& ctx,
                                    std::span<const math::Vec4> verts)
{
    upload_to_buffer(ctx, m_epsilon_buf, verts, m_epsilon_count);
}

void Renderer::upload_interval_lines(const VulkanContext& ctx,
                                      std::span<const math::Vec4> verts)
{
    upload_to_buffer(ctx, m_interval_buf, verts, m_interval_count);
}

void Renderer::upload_centre_lines(const VulkanContext& ctx,
                                    std::span<const math::Vec4> verts)
{
    upload_to_buffer(ctx, m_centre_buf, verts, m_centre_count);
}

void Renderer::upload_secant_line(const VulkanContext& ctx,
                                   std::span<const math::Vec4> verts,
                                   const float colour[3])
{
    upload_to_buffer(ctx, m_secant_buf, verts, m_secant_count);
    m_secant_colour[0] = colour[0];
    m_secant_colour[1] = colour[1];
    m_secant_colour[2] = colour[2];
}

// ── begin_frame ────────────────────────────────────────────────────────────────

bool Renderer::begin_frame(const VulkanContext& ctx, Swapchain& swapchain)
{
    FrameData& frame = m_frames[m_current_frame];
    vkWaitForFences(ctx.device(), 1, &frame.in_flight, VK_TRUE, UINT64_MAX);

    VkResult acq = vkAcquireNextImageKHR(ctx.device(), swapchain.swapchain(),
                                          UINT64_MAX, frame.img_avail,
                                          VK_NULL_HANDLE, &m_active_img);
    if (acq == VK_ERROR_OUT_OF_DATE_KHR) return false;
    if (acq != VK_SUCCESS && acq != VK_SUBOPTIMAL_KHR)
        throw std::runtime_error("vkAcquireNextImageKHR failed");

    vkResetFences(ctx.device(), 1, &frame.in_flight);
    vkResetCommandPool(ctx.device(), frame.cmd_pool, 0);

    VkCommandBufferBeginInfo begin{};
    begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(frame.cmd_buf, &begin);

    m_active_cmd   = frame.cmd_buf;
    m_active_image = swapchain.images()[m_active_img];

    transition_image(m_active_cmd, m_active_image,
                     VK_IMAGE_LAYOUT_UNDEFINED,
                     VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

    VkClearValue clear{};
    clear.color = {{0.05f, 0.05f, 0.08f, 1.f}};

    VkRenderingAttachmentInfo color_att{};
    color_att.sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    color_att.imageView   = swapchain.image_views()[m_active_img];
    color_att.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    color_att.loadOp      = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_att.storeOp     = VK_ATTACHMENT_STORE_OP_STORE;
    color_att.clearValue  = clear;

    VkExtent2D ext = swapchain.extent();
    VkRenderingInfo render_info{};
    render_info.sType                = VK_STRUCTURE_TYPE_RENDERING_INFO;
    render_info.renderArea           = {{0, 0}, ext};
    render_info.layerCount           = 1;
    render_info.colorAttachmentCount = 1;
    render_info.pColorAttachments    = &color_att;

    vkCmdBeginRendering(m_active_cmd, &render_info);

    VkViewport vp{};
    vp.x        = 0.f;
    vp.width    = static_cast<float>(ext.width);
    vp.y        = static_cast<float>(ext.height);
    vp.height   = -static_cast<float>(ext.height);
    vp.minDepth = 0.f; vp.maxDepth = 1.f;
    vkCmdSetViewport(m_active_cmd, 0, 1, &vp);

    VkRect2D scissor{{0, 0}, ext};
    vkCmdSetScissor(m_active_cmd, 0, 1, &scissor);

    m_imgui.new_frame();
    return true;
}

// ── draw helpers ───────────────────────────────────────────────────────────────

struct PushConstants {
    math::Mat4 mvp;
    int        mode;
    float      colour[3];
};

void Renderer::draw_buffer(VkCommandBuffer cmd,
                             VkPipeline pipeline,
                             VkPipelineLayout layout,
                             const math::Mat4& mvp,
                             int mode,
                             const float colour[3],
                             VkBuffer buf,
                             std::uint32_t count)
{
    if (count < 2 || buf == VK_NULL_HANDLE) return;

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

    PushConstants pc{};
    pc.mvp       = mvp;
    pc.mode      = mode;
    pc.colour[0] = colour ? colour[0] : 1.f;
    pc.colour[1] = colour ? colour[1] : 1.f;
    pc.colour[2] = colour ? colour[2] : 1.f;
    vkCmdPushConstants(cmd, layout, VK_SHADER_STAGE_VERTEX_BIT,
                       0, sizeof(pc), &pc);

    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(cmd, 0, 1, &buf, &offset);
    vkCmdDraw(cmd, count, 1, 0, 0);
}

// ── draw_scene ─────────────────────────────────────────────────────────────────

void Renderer::draw_scene(const math::Mat4& mvp)
{
    // ── Axes ──────────────────────────────────────────────────────────────────
    if (m_axes_count > 0 && m_axes_buf.buffer != VK_NULL_HANDLE) {
        const float grey[3] = {0.75f, 0.75f, 0.72f};
        draw_buffer(m_active_cmd,
                    m_axes_pipeline.pipeline(), m_axes_pipeline.layout(),
                    mvp, 1, grey, m_axes_buf.buffer, m_axes_count);
    }

    // ── Curves ────────────────────────────────────────────────────────────────
    for (auto& cb : m_curves) {
        if (cb.visible_count < 2 || cb.buf.buffer == VK_NULL_HANDLE) continue;

        VkPipeline       pipe = cb.is_hyperbola ? m_hyperbola_pipeline.pipeline()
                                                 : m_curve_pipeline.pipeline();
        VkPipelineLayout lay  = cb.is_hyperbola ? m_hyperbola_pipeline.layout()
                                                 : m_curve_pipeline.layout();

        if (cb.is_hyperbola) {
            const std::uint32_t half = (cb.total_count > 1)
                                       ? (cb.total_count - 1) / 2 : 0;
            // First branch
            if (cb.visible_count > 0) {
                std::uint32_t first_count = std::min(cb.visible_count, half);
                if (first_count >= 2)
                    draw_buffer(m_active_cmd, pipe, lay, mvp, 0, cb.colour,
                                cb.buf.buffer, first_count);
            }
            // Second branch (after sentinel)
            const std::uint32_t second_start = half + 1;
            if (cb.visible_count > second_start) {
                std::uint32_t second_count = cb.visible_count - second_start;
                if (second_count >= 2) {
                    vkCmdBindPipeline(m_active_cmd,
                                      VK_PIPELINE_BIND_POINT_GRAPHICS, pipe);
                    PushConstants pc2{};
                    pc2.mvp      = mvp; pc2.mode = 0;
                    pc2.colour[0] = cb.colour[0];
                    pc2.colour[1] = cb.colour[1];
                    pc2.colour[2] = cb.colour[2];
                    vkCmdPushConstants(m_active_cmd, lay,
                                       VK_SHADER_STAGE_VERTEX_BIT,
                                       0, sizeof(pc2), &pc2);
                    VkDeviceSize offset = second_start * sizeof(math::Vec4);
                    vkCmdBindVertexBuffers(m_active_cmd, 0, 1,
                                          &cb.buf.buffer, &offset);
                    vkCmdDraw(m_active_cmd, second_count, 1, 0, 0);
                }
            }
        } else {
            draw_buffer(m_active_cmd, pipe, lay, mvp, 0, cb.colour,
                        cb.buf.buffer, cb.visible_count);
        }
    }

    // ── Interval boundary lines (yellow) ──────────────────────────────────────
    if (m_interval_count > 0 && m_interval_buf.buffer != VK_NULL_HANDLE) {
        const float yellow[3] = {1.f, 0.85f, 0.1f};
        draw_buffer(m_active_cmd,
                    m_axes_pipeline.pipeline(), m_axes_pipeline.layout(),
                    mvp, 2, yellow, m_interval_buf.buffer, m_interval_count);
    }

    // ── Centre lines (dim gold) ───────────────────────────────────────────────
    if (m_centre_count > 0 && m_centre_buf.buffer != VK_NULL_HANDLE) {
        const float gold[3] = {0.6f, 0.45f, 0.05f};
        draw_buffer(m_active_cmd,
                    m_axes_pipeline.pipeline(), m_axes_pipeline.layout(),
                    mvp, 2, gold, m_centre_buf.buffer, m_centre_count);
    }

    // ── Secant line ───────────────────────────────────────────────────────────
    if (m_secant_count > 0 && m_secant_buf.buffer != VK_NULL_HANDLE) {
        draw_buffer(m_active_cmd,
                    m_axes_pipeline.pipeline(), m_axes_pipeline.layout(),
                    mvp, 0, m_secant_colour,
                    m_secant_buf.buffer, m_secant_count);
    }

    // ── Epsilon ball ──────────────────────────────────────────────────────────
    if (m_epsilon_count > 0 && m_epsilon_buf.buffer != VK_NULL_HANDLE) {
        const float white[3] = {1.f, 1.f, 1.f};
        draw_buffer(m_active_cmd,
                    m_ball_pipeline.pipeline(), m_ball_pipeline.layout(),
                    mvp, 2, white, m_epsilon_buf.buffer, m_epsilon_count);
    }
}

// ── end_frame ─────────────────────────────────────────────────────────────────

bool Renderer::end_frame(const VulkanContext& ctx, Swapchain& swapchain)
{
    m_imgui.render(m_active_cmd);
    vkCmdEndRendering(m_active_cmd);

    transition_image(m_active_cmd, m_active_image,
                     VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                     VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

    vkEndCommandBuffer(m_active_cmd);

    FrameData& frame = m_frames[m_current_frame];
    VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo submit{};
    submit.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.waitSemaphoreCount   = 1;
    submit.pWaitSemaphores      = &frame.img_avail;
    submit.pWaitDstStageMask    = &wait_stage;
    submit.commandBufferCount   = 1;
    submit.pCommandBuffers      = &m_active_cmd;
    submit.signalSemaphoreCount = 1;
    submit.pSignalSemaphores    = &frame.render_done;
    vkQueueSubmit(ctx.graphics_queue(), 1, &submit, frame.in_flight);

    VkSwapchainKHR sc = swapchain.swapchain();
    VkPresentInfoKHR present{};
    present.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present.waitSemaphoreCount = 1;
    present.pWaitSemaphores    = &frame.render_done;
    present.swapchainCount     = 1;
    present.pSwapchains        = &sc;
    present.pImageIndices      = &m_active_img;

    VkResult pres = vkQueuePresentKHR(ctx.present_queue(), &present);

    m_active_cmd    = VK_NULL_HANDLE;
    m_active_image  = VK_NULL_HANDLE;
    m_current_frame = (m_current_frame + 1) % FRAMES_IN_FLIGHT;

    if (pres == VK_ERROR_OUT_OF_DATE_KHR || pres == VK_SUBOPTIMAL_KHR) return false;
    if (pres != VK_SUCCESS) throw std::runtime_error("vkQueuePresentKHR failed");
    return true;
}

// ── Frame sync ─────────────────────────────────────────────────────────────────

void Renderer::create_frames(VkDevice device, std::uint32_t queue_family) {
    for (auto& f : m_frames) {
        VkCommandPoolCreateInfo pool_info{};
        pool_info.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        pool_info.queueFamilyIndex = queue_family;
        pool_info.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        vkCreateCommandPool(device, &pool_info, nullptr, &f.cmd_pool);

        VkCommandBufferAllocateInfo alloc{};
        alloc.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        alloc.commandPool        = f.cmd_pool;
        alloc.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        alloc.commandBufferCount = 1;
        vkAllocateCommandBuffers(device, &alloc, &f.cmd_buf);

        VkSemaphoreCreateInfo sem{};
        sem.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        vkCreateSemaphore(device, &sem, nullptr, &f.img_avail);
        vkCreateSemaphore(device, &sem, nullptr, &f.render_done);

        VkFenceCreateInfo fence{};
        fence.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fence.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        vkCreateFence(device, &fence, nullptr, &f.in_flight);
    }
}

void Renderer::destroy_frames(VkDevice device) {
    for (auto& f : m_frames) {
        vkDestroyFence(device, f.in_flight, nullptr);
        vkDestroySemaphore(device, f.render_done, nullptr);
        vkDestroySemaphore(device, f.img_avail, nullptr);
        vkDestroyCommandPool(device, f.cmd_pool, nullptr);
        f = {};
    }
}

// ── Buffer helpers ─────────────────────────────────────────────────────────────

GpuBuffer Renderer::create_buffer(const VulkanContext& ctx,
                                   VkDeviceSize size,
                                   VkBufferUsageFlags usage,
                                   VkMemoryPropertyFlags props)
{
    GpuBuffer buf; buf.size = size;
    VkBufferCreateInfo info{};
    info.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    info.size        = size;
    info.usage       = usage;
    info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    if (vkCreateBuffer(ctx.device(), &info, nullptr, &buf.buffer) != VK_SUCCESS)
        throw std::runtime_error("Failed to create buffer");

    VkMemoryRequirements req{};
    vkGetBufferMemoryRequirements(ctx.device(), buf.buffer, &req);

    VkMemoryAllocateInfo alloc{};
    alloc.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc.allocationSize  = req.size;
    alloc.memoryTypeIndex = find_memory_type(ctx.physical_device(),
                                              req.memoryTypeBits, props);
    if (vkAllocateMemory(ctx.device(), &alloc, nullptr, &buf.memory) != VK_SUCCESS)
        throw std::runtime_error("Failed to allocate buffer memory");
    vkBindBufferMemory(ctx.device(), buf.buffer, buf.memory, 0);
    return buf;
}

void Renderer::destroy_buffer(VkDevice device, GpuBuffer& buf) {
    if (buf.buffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(device, buf.buffer, nullptr);
        buf.buffer = VK_NULL_HANDLE;
    }
    if (buf.memory != VK_NULL_HANDLE) {
        vkFreeMemory(device, buf.memory, nullptr);
        buf.memory = VK_NULL_HANDLE;
    }
    buf.size = 0;
}

std::uint32_t Renderer::find_memory_type(VkPhysicalDevice phys,
                                           std::uint32_t type_filter,
                                           VkMemoryPropertyFlags props)
{
    VkPhysicalDeviceMemoryProperties mp{};
    vkGetPhysicalDeviceMemoryProperties(phys, &mp);
    for (std::uint32_t i = 0; i < mp.memoryTypeCount; ++i)
        if ((type_filter & (1u << i)) &&
            (mp.memoryTypes[i].propertyFlags & props) == props) return i;
    throw std::runtime_error("No suitable memory type");
}

void Renderer::transition_image(VkCommandBuffer cmd, VkImage image,
                                  VkImageLayout from, VkImageLayout to)
{
    VkImageMemoryBarrier b{};
    b.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    b.oldLayout           = from;
    b.newLayout           = to;
    b.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    b.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    b.image               = image;
    b.subresourceRange    = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

    VkPipelineStageFlags src{}, dst{};
    if (from == VK_IMAGE_LAYOUT_UNDEFINED &&
        to   == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
        b.srcAccessMask = 0;
        b.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        src = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dst = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    } else if (from == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL &&
               to   == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR) {
        b.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        b.dstAccessMask = 0;
        src = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dst = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    } else {
        b.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
        b.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
        src = dst = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    }
    vkCmdPipelineBarrier(cmd, src, dst, 0, 0, nullptr, 0, nullptr, 1, &b);
}

} // namespace ndde::renderer
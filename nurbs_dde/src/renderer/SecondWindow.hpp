#pragma once
// renderer/SecondWindow.hpp
// A second OS window with its own Vulkan surface + swapchain,
// sharing the VkDevice/queues/pipelines of the primary window.

#include "renderer/GpuTypes.hpp"
#include "renderer/Pipeline.hpp"
#include <volk.h>
#include <VkBootstrap.h>
#include <string>
#include <vector>

struct GLFWwindow;
namespace ndde::platform { class VulkanContext; }

namespace ndde::renderer {

struct DrawCall;

class SecondWindow {
public:
    SecondWindow()  = default;
    ~SecondWindow();

    SecondWindow(const SecondWindow&)            = delete;
    SecondWindow& operator=(const SecondWindow&) = delete;

    // x,y: OS screen position for top-left corner.
    void init(const platform::VulkanContext& ctx,
              int x, int y, u32 width, u32 height,
              const std::string& title,
              const std::string& shader_dir);

    void destroy();

    [[nodiscard]] bool begin_frame();
    void draw(const DrawCall& dc);
    [[nodiscard]] bool end_frame();

    void on_resize();

    [[nodiscard]] bool should_close() const noexcept;
    [[nodiscard]] bool valid()        const noexcept { return m_initialised; }
    [[nodiscard]] u32  width()        const noexcept { return m_sc_extent.width;  }
    [[nodiscard]] u32  height()       const noexcept { return m_sc_extent.height; }
    [[nodiscard]] GLFWwindow* window()const noexcept { return m_window; }

private:
    GLFWwindow*      m_window          = nullptr;
    VkInstance       m_instance        = VK_NULL_HANDLE;  // borrowed
    VkPhysicalDevice m_physical_device = VK_NULL_HANDLE;  // borrowed
    VkDevice         m_device          = VK_NULL_HANDLE;  // borrowed
    VkQueue          m_gfx_queue       = VK_NULL_HANDLE;  // borrowed
    VkQueue          m_present_queue   = VK_NULL_HANDLE;  // borrowed
    u32              m_gfx_family      = 0;
    VkSurfaceKHR     m_surface         = VK_NULL_HANDLE;  // owned

    // Raw swapchain state (bypass Swapchain class — it uses VulkanContext::surface())
    VkSwapchainKHR           m_sc_raw    = VK_NULL_HANDLE;
    VkFormat                 m_sc_format = VK_FORMAT_UNDEFINED;
    VkExtent2D               m_sc_extent = {};
    std::vector<VkImage>     m_sc_images;
    std::vector<VkImageView> m_sc_views;

    VkCommandPool   m_cmd_pool        = VK_NULL_HANDLE;
    VkCommandBuffer m_cmd             = VK_NULL_HANDLE;
    VkFence         m_render_fence    = VK_NULL_HANDLE;
    // One semaphore per swapchain image — prevents reuse-before-consumed races
    // for both acquire and render-finished signals.
    std::vector<VkSemaphore> m_image_available;
    std::vector<VkSemaphore> m_render_finished;
    u32             m_image_index     = 0;
    bool            m_frame_open      = false;
    bool            m_initialised     = false;

    Pipeline m_pipeline_line_list;
    Pipeline m_pipeline_line_strip;
    Pipeline m_pipeline_triangle_list;

    void build_swapchain(u32 w, u32 h);
    void destroy_swapchain();
    void create_cmd_objects();
    void create_sync_objects();
    void init_pipelines(const std::string& shader_dir);
    void transition_image(VkImage image, VkImageLayout from, VkImageLayout to);
    [[nodiscard]] Pipeline& pipeline_for(Topology topo);

    static void resize_callback(GLFWwindow* win, int w, int h);
};

} // namespace ndde::renderer

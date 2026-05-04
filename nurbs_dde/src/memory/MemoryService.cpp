#include "memory/MemoryService.hpp"

namespace ndde::memory {

void MemoryService::init_frame_gpu_arena(VkDevice device, VkPhysicalDevice physical_device, u32 size_mb) {
    m_frame_gpu.init(device, physical_device, size_mb);
}

void MemoryService::destroy() {
    m_frame_gpu.destroy();
}

void MemoryService::begin_frame() noexcept {
    m_frame.reset();
    m_frame_cpu.release();
    m_frame.set_resource(&m_frame_cpu);
    m_frame_gpu.reset();
}

} // namespace ndde::memory

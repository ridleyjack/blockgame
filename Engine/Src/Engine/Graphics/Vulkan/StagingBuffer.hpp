#pragma once

#include <vulkan/vulkan.h>

namespace engine::graphics::vulkan {
class Context;

// Todo: Ring buffer.

class StagingBuffer {
public:
  static constexpr VkDeviceSize DefaultBufferMemorySize = 256ull * 1024ull * 1024ull; // 256 MiB

  explicit StagingBuffer(Context& context);
  ~StagingBuffer();

  void* Mapping() const noexcept;
  VkDeviceMemory Memory() const noexcept;
  VkBuffer Handle() const noexcept;
  VkDeviceSize Allocate(VkDeviceSize size, VkDeviceSize alignment);

private:
  Context& context_;

  VkBuffer buffer_{VK_NULL_HANDLE};
  VkDeviceMemory memory_{VK_NULL_HANDLE};
  VkDeviceSize capacity_{};
  VkDeviceSize offset_{};

  void* mapped_{};

  static constexpr VkDeviceSize align_(VkDeviceSize value, VkDeviceSize alignment);
};

} // namespace engine::graphics::vulkan
#pragma once

#include <vulkan/vulkan.h>

namespace engine::graphics::vulkan {
class Context;

// MeshBuffer creates a single buffer and memory allocation to story mesh vertices and indices in. It currently
// allocates a buffer of size DefaultBufferMemorySize. Meshes are added in a linear fashion until capacity is exceeded
// and an exception is thrown.
class MeshBuffer {
public:
  static constexpr VkDeviceSize DefaultBufferMemorySize = 256ull * 1024ull * 1024ull; // 256 MiB

  explicit MeshBuffer(Context& context);
  ~MeshBuffer();

  VkBuffer Handle() const noexcept;
  VkDeviceSize AllocateDeviceLocal(VkDeviceSize size);

private:
  Context& context_;

  VkBuffer buffer_{VK_NULL_HANDLE};
  VkDeviceMemory memory_{VK_NULL_HANDLE};
  VkDeviceSize capacity_{};
  VkDeviceSize offset_{};

  static constexpr VkDeviceSize align_(VkDeviceSize value, VkDeviceSize alignment);
};

} // namespace engine::graphics::vulkan
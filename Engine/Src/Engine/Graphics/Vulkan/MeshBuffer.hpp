#pragma once

#include "Engine/Memory/SparseBuffer.hpp"

#include <vulkan/vulkan.h>

namespace engine::graphics::vulkan {
class Context;

// MeshBuffer creates a single buffer and memory allocation to story mesh vertices and indices in. It currently
// allocates a buffer of size DefaultBufferMemorySize.
class MeshBuffer {
public:
  static constexpr VkDeviceSize DefaultBufferMemorySize = 256ull * 1024ull * 1024ull; // 256 MiB

  static MeshBuffer Create(Context& context);
  ~MeshBuffer();

  MeshBuffer(const MeshBuffer&) = delete;
  MeshBuffer& operator=(const MeshBuffer&) = delete;

  VkBuffer Handle() const noexcept;

  VkDeviceSize Allocate(VkDeviceSize size);
  void Free(VkDeviceSize offset);

private:
  Context& context_;

  VkBuffer buffer_{VK_NULL_HANDLE};
  VkDeviceMemory memory_{VK_NULL_HANDLE};

  memory::SparseBuffer sparseBuffer_;

  explicit MeshBuffer(Context& context, VkBuffer buffer, VkDeviceMemory memory, VkDeviceSize capacity);
};

} // namespace engine::graphics::vulkan
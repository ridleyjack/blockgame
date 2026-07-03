#pragma once

#include "Config.hpp"
#include "Engine/Containers/HandlePool.hpp"
#include "Engine/Memory/SparseBuffer.hpp"

#include <span>
#include <vector>
#include <vulkan/vulkan.h>

namespace engine::graphics::vulkan {
class Context;

class ShaderDataAllocator {
public:
  static constexpr VkDeviceSize DefaultBufferMemorySize = 256ull * 1024ull * 1024ull; // 256 MiB

  explicit ShaderDataAllocator(Context& context);
  ~ShaderDataAllocator();

  std::uint32_t AllocateShaderData(std::size_t size);
  void FreeShaderDataDeferred(std::uint32_t id, std::uint32_t retireFrame);
  void ProcessDeferredDeletions(std::uint32_t currentFrame);
  void WriteShaderData(std::uint32_t id, std::uint32_t frame, std::span<const std::byte> data);
  VkDeviceSize GetShaderDataAddress(std::uint32_t id, std::uint32_t frame) const noexcept;

private:
  struct ShaderData {
    std::array<VkDeviceSize, config::MaxFramesInFlight> Offsets;
    VkDeviceSize Size;
  };

  struct PendingDelete {
    std::uint32_t ShaderDataID{};
    std::uint32_t RetireFrame{};
  };

  Context& context_;

  VkBuffer buffer_{VK_NULL_HANDLE};
  VkDeviceMemory memory_{VK_NULL_HANDLE};
  VkDeviceAddress deviceAddress_{};
  VkDeviceSize capacity_{};
  std::byte* memoryMapping_{nullptr};

  memory::SparseBuffer sparseBuffer_{DefaultBufferMemorySize};

  containers::HandlePool<ShaderData> shaderDataPool_{};
  std::vector<PendingDelete> pendingDeletes_{};

  void freeShaderData_(std::uint32_t id);
};

} // namespace engine::graphics::vulkan

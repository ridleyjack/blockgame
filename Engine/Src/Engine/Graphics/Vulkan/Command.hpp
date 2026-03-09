#pragma once

#include <vulkan/vulkan.h>

#include <vector>

namespace engine::graphics::vulkan {

class Context;

class Command {
public:
  explicit Command(Context& context);
  ~Command();

  Command(const Command&) = delete;
  Command& operator=(const Command&) = delete;

  VkCommandBuffer PerFrameBuffer(uint32_t index) const noexcept;

  VkCommandBuffer BeginSingleTimeCommands() const noexcept;
  void EndSingleTimeCommands(VkCommandBuffer commandBuffer) const noexcept;

  VkCommandBuffer BeginTransient() const noexcept;
  void FreeTransient(VkCommandBuffer cmd) const noexcept;

private:
  Context& context_;

  VkCommandPool framePool_{VK_NULL_HANDLE};
  VkCommandPool transientPool_{VK_NULL_HANDLE};

  std::vector<VkCommandBuffer> perFrameBuffers_{};

  void createFramePool_();
  void createFrameBuffers_();

  void createTransientPool_();
};

} // namespace engine::graphics::vulkan
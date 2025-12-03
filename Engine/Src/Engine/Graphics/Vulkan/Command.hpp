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

  VkCommandBuffer& Buffer(uint32_t index) noexcept;

  VkCommandBuffer BeginSingleTimeCommands_();
  void EndSingleTimeCommands_(VkCommandBuffer commandBuffer);

private:
  Context& context_;
  VkCommandPool commandPool_{VK_NULL_HANDLE};
  std::vector<VkCommandBuffer> commandBuffers_{};

  void createCommandPool_();
  void createCommandBuffers_();
};

} // namespace engine::graphics::vulkan
#include "Command.hpp"

#include "Context.hpp"
#include "Engine/Graphics/Vulkan/Device.hpp"

#include <stdexcept>

namespace engine::graphics::vulkan {

// ==============================
// Public Methods
// ==============================

Command::Command(Context& context) : context_{context} {
  createCommandPool_();
  createCommandBuffers_();
}

Command::~Command() {
  auto vkDevice = context_.GetDevice().Logical();
  vkDestroyCommandPool(vkDevice, commandPool_, nullptr);
}

VkCommandBuffer& Command::Buffer(uint32_t index) noexcept {
  return commandBuffers_[index];
}

VkCommandBuffer Command::BeginSingleTimeCommands_() {
  const auto vkDevice = context_.GetDevice().Logical();

  // TODO: Command pool for short lived operations allows optimization.
  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandPool = commandPool_;
  allocInfo.commandBufferCount = 1;

  VkCommandBuffer commandBuffer;
  vkAllocateCommandBuffers(vkDevice, &allocInfo, &commandBuffer);

  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  vkBeginCommandBuffer(commandBuffer, &beginInfo);
  return commandBuffer;
}

void Command::EndSingleTimeCommands_(VkCommandBuffer commandBuffer) {
  auto& device = context_.GetDevice();

  vkEndCommandBuffer(commandBuffer);

  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffer;

  vkQueueSubmit(device.GraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
  vkQueueWaitIdle(device.GraphicsQueue());

  vkFreeCommandBuffers(device.Logical(), commandPool_, 1, &commandBuffer);
}

// ==============================
// Private Methods
// ==============================

void Command::createCommandPool_() {
  const auto& device = context_.GetDevice();
  QueueFamilyIndices queueFamilyIndices = device.FindQueueFamilies();

  VkCommandPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  poolInfo.queueFamilyIndex = queueFamilyIndices.GraphicsFamily.value();

  if (vkCreateCommandPool(device.Logical(), &poolInfo, nullptr, &commandPool_) != VK_SUCCESS) {
    throw std::runtime_error("failed to create command pool!");
  }
}

void Command::createCommandBuffers_() {
  const auto& device = context_.GetDevice();

  commandBuffers_.resize(config::MaxFramesInFlight);

  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool = commandPool_;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers_.size());

  if (vkAllocateCommandBuffers(device.Logical(), &allocInfo, commandBuffers_.data()) != VK_SUCCESS) {
    throw std::runtime_error("failed to allocate command buffers!");
  }
}
} // namespace engine::graphics::vulkan
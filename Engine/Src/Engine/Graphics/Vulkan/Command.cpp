#include "Command.hpp"

#include "Context.hpp"
#include "Config.hpp"
#include "Engine/Graphics/Vulkan/Device.hpp"

#include <stdexcept>

namespace engine::graphics::vulkan {

Command::Command(Context& context) : context_{context} {
  createFramePool_();
  createFrameBuffers_();
  createTransientPool_();
}

Command::~Command() {
  const auto vkDevice = context_.GetDevice().Logical();
  vkDestroyCommandPool(vkDevice, framePool_, nullptr);
  vkDestroyCommandPool(vkDevice, transientPool_, nullptr);
}

VkCommandBuffer Command::PerFrameBuffer(const uint32_t index) const noexcept {
  return perFrameBuffers_[index];
}

VkCommandBuffer Command::BeginSingleTimeCommands() const noexcept {
  const auto vkDevice = context_.GetDevice().Logical();

  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandPool = transientPool_;
  allocInfo.commandBufferCount = 1;

  VkCommandBuffer commandBuffer;
  vkAllocateCommandBuffers(vkDevice, &allocInfo, &commandBuffer);

  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  vkBeginCommandBuffer(commandBuffer, &beginInfo);
  return commandBuffer;
}

void Command::EndSingleTimeCommands(VkCommandBuffer commandBuffer) const noexcept {
  const auto& device = context_.GetDevice();

  vkEndCommandBuffer(commandBuffer);

  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffer;

  vkQueueSubmit(device.GraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
  vkQueueWaitIdle(device.GraphicsQueue());

  vkFreeCommandBuffers(device.Logical(), transientPool_, 1, &commandBuffer);
}

VkCommandBuffer Command::BeginTransient() const noexcept {
  // BeginSingleTimeCommands will likely be removed in the future.
  return BeginSingleTimeCommands();
}

void Command::FreeTransient(VkCommandBuffer cmd) const noexcept {
  const auto& vkDevice = context_.GetDevice().Logical();
  vkFreeCommandBuffers(vkDevice, transientPool_, 1, &cmd);
}

void Command::createFramePool_() {
  const auto& device = context_.GetDevice();
  QueueFamilyIndices queueFamilyIndices = device.FindQueueFamilies();

  VkCommandPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  poolInfo.queueFamilyIndex = queueFamilyIndices.GraphicsFamily.value();

  if (vkCreateCommandPool(device.Logical(), &poolInfo, nullptr, &framePool_) != VK_SUCCESS) {
    throw std::runtime_error("failed to create command pool!");
  }
}

void Command::createFrameBuffers_() {
  const auto& device = context_.GetDevice();

  perFrameBuffers_.resize(config::MaxFramesInFlight);

  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool = framePool_;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = static_cast<uint32_t>(perFrameBuffers_.size());

  if (vkAllocateCommandBuffers(device.Logical(), &allocInfo, perFrameBuffers_.data()) != VK_SUCCESS) {
    throw std::runtime_error("failed to allocate command buffers!");
  }
}

void Command::createTransientPool_() {
  const auto& device = context_.GetDevice();
  QueueFamilyIndices queueFamilyIndices = device.FindQueueFamilies();

  VkCommandPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
  poolInfo.queueFamilyIndex = queueFamilyIndices.GraphicsFamily.value();

  if (vkCreateCommandPool(device.Logical(), &poolInfo, nullptr, &transientPool_) != VK_SUCCESS) {
    throw std::runtime_error("failed to create transient pool!");
  }
}
} // namespace engine::graphics::vulkan
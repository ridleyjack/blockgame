#pragma once

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include <vector>

namespace engine::graphics::vulkan {

class Context;
class Device;
class UniformBuffer;

class DescriptorAllocator {
public:
  static UniformBuffer CreateUniformBuffer(const Device& device);

  explicit DescriptorAllocator(Context& context);
  ~DescriptorAllocator();

  void CreateDescriptorSet(VkDescriptorSetLayout descriptorSetLayout,
                           const UniformBuffer& uniformGPU,
                           VkImageView imageView,
                           VkSampler sampler);

  VkDescriptorSet DescriptorSet(uint32_t frame) const noexcept;

private:
  Context& context_;

  VkDescriptorPool descriptorPool_{VK_NULL_HANDLE};
  std::vector<VkDescriptorSet> descriptorSets_{};
};

} // namespace engine::graphics::vulkan

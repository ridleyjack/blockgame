#pragma once

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include <cstdint>
#include <vector>

namespace engine::graphics::vulkan {

class Context;
class Device;
class UniformBuffer;

struct DescriptorSetBinding {
  std::vector<VkDescriptorSet> descriptors_{};
};

class DescriptorAllocator {
public:
  static UniformBuffer CreateUniformBuffer(const Device& device);

  explicit DescriptorAllocator(Context& context);
  ~DescriptorAllocator();

  std::uint32_t CreateDescriptorSet(VkDescriptorSetLayout descriptorSetLayout,
                                    const UniformBuffer& uniformGPU,
                                    VkImageView imageView,
                                    VkSampler sampler);

  VkDescriptorSet DescriptorSet(std::uint32_t setID, std::uint32_t frame) const noexcept;

private:
  Context& context_;

  VkDescriptorPool descriptorPool_{VK_NULL_HANDLE};
  std::vector<DescriptorSetBinding> descriptorSets_{};
};

} // namespace engine::graphics::vulkan

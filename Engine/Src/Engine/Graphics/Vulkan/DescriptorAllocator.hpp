#pragma once

#include <vulkan/vulkan.h>
#include "Config.hpp"

#include <array>
#include <vector>

namespace engine::graphics::vulkan {
struct TextureGPU;

class Context;
class TextureAllocator;
class Device;
class UniformBuffer;

class DescriptorAllocator {
public:
  static UniformBuffer CreateUniformBuffer(const Device& device);

  DescriptorAllocator(Context& context, TextureAllocator& textureAllocator);
  ~DescriptorAllocator();

  std::uint32_t CreateDescriptorSet(VkDescriptorSetLayout descriptorSetLayout,
                                    const UniformBuffer& uniformGPU,
                                    std::uint32_t textureID);

  VkDescriptorSet DescriptorSet(std::uint32_t setID, std::uint32_t frame) noexcept;

  // DescriptorSetLayout returns the default layout that is used by all rendering components at this stage of
  // development.
  VkDescriptorSetLayout DescriptorSetLayout() const noexcept;

private:
  struct DescriptorEntry {
    std::array<VkDescriptorSet, config::MaxFramesInFlight> Sets{};
    std::uint32_t TextureID{};
    bool TextureReady{};
  };

  Context& context_;
  TextureAllocator& textureAllocator_;

  VkDescriptorSetLayout descriptorSetLayout_{VK_NULL_HANDLE};
  VkDescriptorPool descriptorPool_{VK_NULL_HANDLE};

  std::vector<DescriptorEntry> descriptorSets_{};

  void createDescriptorSetLayout_();
  void writeTexture(DescriptorEntry& entry, const TextureGPU& texture) const noexcept;
};

} // namespace engine::graphics::vulkan

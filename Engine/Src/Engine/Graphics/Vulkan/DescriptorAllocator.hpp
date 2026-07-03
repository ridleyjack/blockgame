#pragma once

#include <vulkan/vulkan.h>
#include "Config.hpp"

#include <array>
#include <vector>
#include <span>

namespace engine::graphics::vulkan {
struct TextureGPU;

class Context;
class TextureAllocator;
class Device;

class DescriptorAllocator {
public:
  static constexpr std::uint32_t MaxTextures = 256;

  DescriptorAllocator(Context& context, TextureAllocator& textureAllocator);
  ~DescriptorAllocator();

  std::uint32_t CreateTextureDescriptorSet(std::uint32_t textureID);
  VkDescriptorSet TextureDescriptorSet(std::uint32_t setID) noexcept;

  std::span<const VkDescriptorSetLayout> TextureSetLayout() const noexcept;

private:
  struct DescriptorEntry {
    std::uint32_t TextureID{};
    VkDescriptorSet DescriptorSet{VK_NULL_HANDLE};
    bool TextureReady{};
  };

  Context& context_;
  TextureAllocator& textureAllocator_;

  VkDescriptorSetLayout textureSetLayout_{VK_NULL_HANDLE};

  VkDescriptorPool descriptorPool_{VK_NULL_HANDLE};

  std::vector<DescriptorEntry> textureSets_{};

  void createDescriptorSetLayouts_();
  void writeTexture(DescriptorEntry& entry, const TextureGPU& texture) const noexcept;
};

} // namespace engine::graphics::vulkan

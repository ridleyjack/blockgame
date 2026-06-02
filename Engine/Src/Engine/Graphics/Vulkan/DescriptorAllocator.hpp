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
struct UniformBuffer;

class DescriptorAllocator {
public:
  static UniformBuffer CreateUniformBuffer(const Device& device);

  static constexpr std::uint32_t MaxTextures = 256;

  DescriptorAllocator(Context& context, TextureAllocator& textureAllocator);
  ~DescriptorAllocator();

  void CreateGlobalDescriptorSets(const UniformBuffer& uniformGPU);
  std::uint32_t CreateTextureDescriptorSet(std::uint32_t textureID);

  VkDescriptorSet GlobalDescriptorSet(std::uint32_t frame) const noexcept;
  VkDescriptorSet TextureDescriptorSet(std::uint32_t setID) noexcept;

  // DescriptorSetLayout returns the layout used for the per frame global UBO and the
  std::array<VkDescriptorSetLayout, 2> DescriptorSetLayouts() const noexcept;

private:
  struct DescriptorEntry {
    std::uint32_t TextureID{};
    VkDescriptorSet DescriptorSet{VK_NULL_HANDLE};
    bool TextureReady{};
  };

  Context& context_;
  TextureAllocator& textureAllocator_;

  VkDescriptorSetLayout globalSetLayout_{VK_NULL_HANDLE};
  VkDescriptorSetLayout textureSetLayout_{VK_NULL_HANDLE};

  VkDescriptorPool descriptorPool_{VK_NULL_HANDLE};

  std::array<VkDescriptorSet, config::MaxFramesInFlight> globalSets_{};
  std::vector<DescriptorEntry> textureSets_{};

  void createDescriptorSetLayouts_();
  void writeTexture(DescriptorEntry& entry, const TextureGPU& texture) const noexcept;
};

} // namespace engine::graphics::vulkan

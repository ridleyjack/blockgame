#include "DescriptorAllocator.hpp"

#include "CheckVk.hpp"
#include "Device.hpp"
#include "Context.hpp"
#include "Config.hpp"
#include "TextureAllocator.hpp"

#include <assert.h>

namespace engine::graphics::vulkan {

DescriptorAllocator::DescriptorAllocator(Context& context, TextureAllocator& textureAllocator)
    : context_(context), textureAllocator_(textureAllocator) {
  const auto& device = context_.GetDevice();

  // Init Descriptor Pool.
  std::array<VkDescriptorPoolSize, 1> poolSizes{};
  poolSizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  poolSizes[0].descriptorCount = MaxTextures;

  VkDescriptorPoolCreateInfo poolInfo{
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
      .maxSets = MaxTextures,
      .poolSizeCount = poolSizes.size(),
      .pPoolSizes = poolSizes.data(),
  };

  CheckVk(vkCreateDescriptorPool(device.Logical(), &poolInfo, nullptr, &descriptorPool_), "vkCreateDescriptorPool");

  createDescriptorSetLayouts_();
}

DescriptorAllocator::~DescriptorAllocator() {
  const auto& device = context_.GetDevice();
  vkDestroyDescriptorPool(device.Logical(), descriptorPool_, nullptr);
  vkDestroyDescriptorSetLayout(device.Logical(), textureSetLayout_, nullptr);
}

std::uint32_t DescriptorAllocator::CreateTextureDescriptorSet(std::uint32_t textureID) {
  const auto& vkDevice = context_.GetDevice().Logical();
  DescriptorEntry entry{.TextureID = textureID};

  VkDescriptorSetAllocateInfo allocInfo{
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
      .descriptorPool = descriptorPool_,
      .descriptorSetCount = 1,
      .pSetLayouts = &textureSetLayout_,
  };
  CheckVk(vkAllocateDescriptorSets(vkDevice, &allocInfo, &entry.DescriptorSet), "vkAllocateDescriptorSets");

  textureSets_.emplace_back(entry);
  return textureSets_.size() - 1;
}

VkDescriptorSet DescriptorAllocator::TextureDescriptorSet(const std::uint32_t textureSetID) noexcept {
  assert(textureSetID < textureSets_.size());

  auto& entry = textureSets_[textureSetID];
  if (!entry.TextureReady) {
    const TextureGPU& texture = textureAllocator_.Get(entry.TextureID);
    if (texture.State != TextureState::Ready)
      return VK_NULL_HANDLE;
    writeTexture(entry, texture);
  }
  return entry.DescriptorSet;
}

std::span<const VkDescriptorSetLayout> DescriptorAllocator::TextureSetLayout() const noexcept {
  return {&textureSetLayout_, 1};
}

void DescriptorAllocator::createDescriptorSetLayouts_() {
  const auto& device = context_.GetDevice().Logical();

  // Texture Layout
  constexpr VkDescriptorSetLayoutBinding samplerLayoutBinding{
      .binding = 0,
      .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
      .descriptorCount = 1,
      .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
  };

  constexpr std::array<VkDescriptorSetLayoutBinding, 1> textureBinding{samplerLayoutBinding};
  const VkDescriptorSetLayoutCreateInfo textureLayoutInfo{
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
      .bindingCount = textureBinding.size(),
      .pBindings = textureBinding.data(),
  };

  CheckVk(vkCreateDescriptorSetLayout(device, &textureLayoutInfo, nullptr, &textureSetLayout_),
          "vkCreateDescriptorSetLayout");
}

void DescriptorAllocator::writeTexture(DescriptorEntry& entry, const TextureGPU& texture) const noexcept {
  auto vkDevice = context_.GetDevice().Logical();

  // Update Descriptor Sets.
  for (std::size_t i = 0; i < config::MaxFramesInFlight; i++) {
    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = texture.ImageView;
    imageInfo.sampler = texture.Sampler;

    std::array<VkWriteDescriptorSet, 1> descriptorWrites{};
    descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[0].dstSet = entry.DescriptorSet;
    descriptorWrites[0].dstBinding = 0;
    descriptorWrites[0].dstArrayElement = 0;
    descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrites[0].descriptorCount = 1;
    descriptorWrites[0].pImageInfo = &imageInfo;

    vkUpdateDescriptorSets(vkDevice, descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
  }
  entry.TextureReady = true;
}

} // namespace engine::graphics::vulkan

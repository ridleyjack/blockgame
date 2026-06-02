#include "DescriptorAllocator.hpp"

#include "Device.hpp"
#include "Context.hpp"
#include "Config.hpp"
#include "TextureAllocator.hpp"
#include "UniformBuffer.hpp"

namespace engine::graphics::vulkan {

UniformBuffer DescriptorAllocator::CreateUniformBuffer(const Device& device) {
  // Return hardcoded uniform for now.

  UniformBuffer uniform{};
  for (size_t i = 0; i < config::MaxFramesInFlight; i++) {
    constexpr VkDeviceSize bufferSize = sizeof(GlobalUBO);

    uniform.Buffers[i] =
        device.CreateBuffer(bufferSize,
                            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    vkMapMemory(device.Logical(), uniform.Buffers[i].Memory, 0, bufferSize, 0, &uniform.MappedMemory[i]);
  }
  return uniform;
}

DescriptorAllocator::DescriptorAllocator(Context& context, TextureAllocator& textureAllocator)
    : context_(context), textureAllocator_(textureAllocator) {
  const auto& device = context_.GetDevice();

  // Init Descriptor Pool.
  std::array<VkDescriptorPoolSize, 2> poolSizes{};
  poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  poolSizes[0].descriptorCount = config::MaxFramesInFlight;
  poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  poolSizes[1].descriptorCount = MaxTextures;

  VkDescriptorPoolCreateInfo poolInfo{
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
      .maxSets = config::MaxFramesInFlight + MaxTextures,
      .poolSizeCount = poolSizes.size(),
      .pPoolSizes = poolSizes.data(),
  };

  if (vkCreateDescriptorPool(device.Logical(), &poolInfo, nullptr, &descriptorPool_) != VK_SUCCESS) {
    throw std::runtime_error("failed to create descriptor pool!");
  }

  createDescriptorSetLayouts_();
}

DescriptorAllocator::~DescriptorAllocator() {
  const auto& device = context_.GetDevice();
  vkDestroyDescriptorPool(device.Logical(), descriptorPool_, nullptr);
  vkDestroyDescriptorSetLayout(device.Logical(), globalSetLayout_, nullptr);
  vkDestroyDescriptorSetLayout(device.Logical(), textureSetLayout_, nullptr);
}

void DescriptorAllocator::CreateGlobalDescriptorSets(const UniformBuffer& uniformGPU) {
  const auto& vkDevice = context_.GetDevice().Logical();

  std::array<VkDescriptorSetLayout, config::MaxFramesInFlight> layouts{};
  layouts.fill(globalSetLayout_);

  VkDescriptorSetAllocateInfo allocInfo{
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
      .descriptorPool = descriptorPool_,
      .descriptorSetCount = static_cast<uint32_t>(config::MaxFramesInFlight),
      .pSetLayouts = layouts.data(),
  };
  if (vkAllocateDescriptorSets(vkDevice, &allocInfo, globalSets_.data()) != VK_SUCCESS) {
    throw std::runtime_error("failed to allocate descriptor sets!");
  }

  // Update Descriptor Sets.
  for (std::size_t i = 0; i < config::MaxFramesInFlight; i++) {
    VkDescriptorBufferInfo bufferInfo{
        .buffer = uniformGPU.Buffers[i].Buffer,
        .offset = 0,
        .range = sizeof(GlobalUBO),
    };

    std::array<VkWriteDescriptorSet, 1> descriptorWrites{
        VkWriteDescriptorSet{
                             .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                             .dstSet = globalSets_[i],
                             .dstBinding = 0,
                             .dstArrayElement = 0,
                             .descriptorCount = 1,
                             .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                             .pBufferInfo = &bufferInfo,
                             }
    };
    vkUpdateDescriptorSets(vkDevice, descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
  }
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
  if (vkAllocateDescriptorSets(vkDevice, &allocInfo, &entry.DescriptorSet) != VK_SUCCESS) {
    throw std::runtime_error("failed to allocate descriptor sets!");
  }

  textureSets_.emplace_back(entry);
  return textureSets_.size() - 1;
}

// GlobalDescriptorSet returns the requested VKDescriptorSet. VK_NULL_HANDLE is returned if uploading the dependencies
// is not finished.
VkDescriptorSet DescriptorAllocator::GlobalDescriptorSet(const std::uint32_t frame) const noexcept {
  assert(globalSets_[frame] != VK_NULL_HANDLE);
  return globalSets_[frame];
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

std::array<VkDescriptorSetLayout, 2> DescriptorAllocator::DescriptorSetLayouts() const noexcept {
  return std::array<VkDescriptorSetLayout, 2>{globalSetLayout_, textureSetLayout_};
}

void DescriptorAllocator::createDescriptorSetLayouts_() {
  const auto& device = context_.GetDevice().Logical();

  // UBO Layout
  constexpr VkDescriptorSetLayoutBinding uboLayoutBinding{
      .binding = 0,
      .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
      .descriptorCount = 1,
      .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
  };

  constexpr std::array<VkDescriptorSetLayoutBinding, 1> uboBinding{uboLayoutBinding};
  const VkDescriptorSetLayoutCreateInfo uboLayoutInfo{
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
      .bindingCount = uboBinding.size(),
      .pBindings = uboBinding.data(),
  };

  if (vkCreateDescriptorSetLayout(device, &uboLayoutInfo, nullptr, &globalSetLayout_) != VK_SUCCESS) {
    throw std::runtime_error("failed to create UBO descriptor set layout!");
  }

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

  if (vkCreateDescriptorSetLayout(device, &textureLayoutInfo, nullptr, &textureSetLayout_) != VK_SUCCESS) {
    throw std::runtime_error("failed to create texture descriptor set layout!");
  }
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
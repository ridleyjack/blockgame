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
    constexpr VkDeviceSize bufferSize = sizeof(UniformBufferObject);

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
  poolSizes[1].descriptorCount = config::MaxFramesInFlight;

  VkDescriptorPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.poolSizeCount = poolSizes.size();
  poolInfo.pPoolSizes = poolSizes.data();
  poolInfo.maxSets = config::MaxFramesInFlight;
  if (vkCreateDescriptorPool(device.Logical(), &poolInfo, nullptr, &descriptorPool_) != VK_SUCCESS) {
    throw std::runtime_error("failed to create descriptor pool!");
  }

  createDescriptorSetLayout_();
}

DescriptorAllocator::~DescriptorAllocator() {
  const auto& device = context_.GetDevice();
  vkDestroyDescriptorPool(device.Logical(), descriptorPool_, nullptr);
  vkDestroyDescriptorSetLayout(device.Logical(), descriptorSetLayout_, nullptr);
}

std::uint32_t DescriptorAllocator::CreateDescriptorSet(VkDescriptorSetLayout descriptorSetLayout,
                                                       const UniformBuffer& uniformGPU,
                                                       std::uint32_t textureID) {

  const auto& vkDevice = context_.GetDevice().Logical();

  std::array<VkDescriptorSetLayout, config::MaxFramesInFlight> layouts{};
  layouts.fill(descriptorSetLayout);

  VkDescriptorSetAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.descriptorPool = descriptorPool_;
  allocInfo.descriptorSetCount = static_cast<uint32_t>(config::MaxFramesInFlight);
  allocInfo.pSetLayouts = layouts.data();

  DescriptorEntry entry{.TextureID = textureID};
  if (vkAllocateDescriptorSets(vkDevice, &allocInfo, entry.Sets.data()) != VK_SUCCESS) {
    throw std::runtime_error("failed to allocate descriptor sets!");
  }

  // Update Descriptor Sets.
  for (std::size_t i = 0; i < config::MaxFramesInFlight; i++) {
    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = uniformGPU.Buffers[i].Buffer;
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(UniformBufferObject);

    std::array<VkWriteDescriptorSet, 1> descriptorWrites{};

    descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[0].dstSet = entry.Sets[i];
    descriptorWrites[0].dstBinding = 0;
    descriptorWrites[0].dstArrayElement = 0;
    descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrites[0].descriptorCount = 1;
    descriptorWrites[0].pBufferInfo = &bufferInfo;

    vkUpdateDescriptorSets(vkDevice, descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
  }

  descriptorSets_.push_back(entry);
  return descriptorSets_.size() - 1;
}

// DescriptorSet returns the requested VKDescriptorSet. VK_NULL_HANDLE is returned if uploading the dependencies is not
// finished.
VkDescriptorSet DescriptorAllocator::DescriptorSet(const std::uint32_t setID, const std::uint32_t frame) noexcept {
  assert(setID < descriptorSets_.size());

  auto& entry = descriptorSets_[setID];
  if (!entry.TextureReady) {
    const TextureGPU& texture = textureAllocator_.Get(entry.TextureID);
    if (texture.State != TextureState::Ready)
      return VK_NULL_HANDLE;
    writeTexture(entry, texture);
  }

  return descriptorSets_[setID].Sets[frame];
}
VkDescriptorSetLayout DescriptorAllocator::DescriptorSetLayout() const noexcept {
  return descriptorSetLayout_;
}

void DescriptorAllocator::createDescriptorSetLayout_() {
  const auto& device = context_.GetDevice().Logical();

  VkDescriptorSetLayoutBinding uboLayoutBinding{};
  uboLayoutBinding.binding = 0;
  uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  uboLayoutBinding.descriptorCount = 1;
  uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

  VkDescriptorSetLayoutBinding samplerLayoutBinding{};
  samplerLayoutBinding.binding = 1;
  samplerLayoutBinding.descriptorCount = 1;
  samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

  const std::array<VkDescriptorSetLayoutBinding, 2> bindings{uboLayoutBinding, samplerLayoutBinding};

  VkDescriptorSetLayoutCreateInfo layoutInfo{};
  layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layoutInfo.bindingCount = bindings.size();
  layoutInfo.pBindings = bindings.data();

  if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout_) != VK_SUCCESS) {
    throw std::runtime_error("failed to create descriptor set layout!");
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
    descriptorWrites[0].dstSet = entry.Sets[i];
    descriptorWrites[0].dstBinding = 1;
    descriptorWrites[0].dstArrayElement = 0;
    descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrites[0].descriptorCount = 1;
    descriptorWrites[0].pImageInfo = &imageInfo;

    vkUpdateDescriptorSets(vkDevice, descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
  }
  entry.TextureReady = true;
}

} // namespace engine::graphics::vulkan
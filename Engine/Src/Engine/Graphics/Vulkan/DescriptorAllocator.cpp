#include "DescriptorAllocator.hpp"

#include "Device.hpp"
#include "Context.hpp"
#include "Config.hpp"
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

DescriptorAllocator::DescriptorAllocator(Context& context) : context_(context) {
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
}

DescriptorAllocator::~DescriptorAllocator() {
  const auto& device = context_.GetDevice();
  vkDestroyDescriptorPool(device.Logical(), descriptorPool_, nullptr);
}

void DescriptorAllocator::CreateDescriptorSet(VkDescriptorSetLayout descriptorSetLayout,
                                              const UniformBuffer& uniformGPU,
                                              VkImageView imageView,
                                              VkSampler sampler) {

  const auto& vkDevice = context_.GetDevice().Logical();

  std::array<VkDescriptorSetLayout, config::MaxFramesInFlight> layouts{};
  layouts.fill(descriptorSetLayout);

  VkDescriptorSetAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.descriptorPool = descriptorPool_;
  allocInfo.descriptorSetCount = static_cast<uint32_t>(config::MaxFramesInFlight);
  allocInfo.pSetLayouts = layouts.data();

  descriptorSets_.resize(config::MaxFramesInFlight);
  if (vkAllocateDescriptorSets(vkDevice, &allocInfo, descriptorSets_.data()) != VK_SUCCESS) {
    throw std::runtime_error("failed to allocate descriptor sets!");
  }

  // Update Descriptor Sets.
  for (size_t i = 0; i < config::MaxFramesInFlight; i++) {
    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = uniformGPU.Buffers[i].Buffer;
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(UniformBufferObject);

    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = imageView;
    imageInfo.sampler = sampler;

    std::array<VkWriteDescriptorSet, 2> descriptorWrites{};

    descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[0].dstSet = descriptorSets_[i];
    descriptorWrites[0].dstBinding = 0;
    descriptorWrites[0].dstArrayElement = 0;
    descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrites[0].descriptorCount = 1;
    descriptorWrites[0].pBufferInfo = &bufferInfo;

    descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[1].dstSet = descriptorSets_[i];
    descriptorWrites[1].dstBinding = 1;
    descriptorWrites[1].dstArrayElement = 0;
    descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrites[1].descriptorCount = 1;
    descriptorWrites[1].pImageInfo = &imageInfo;
    vkUpdateDescriptorSets(vkDevice, descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
  }
}
VkDescriptorSet DescriptorAllocator::DescriptorSet(const uint32_t frame) const noexcept {
  return descriptorSets_[frame];
}
} // namespace engine::graphics::vulkan
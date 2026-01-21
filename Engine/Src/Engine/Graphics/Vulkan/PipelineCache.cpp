#include "PipelineCache.hpp"

#include "Context.hpp"
#include "Device.hpp"
#include "Pipeline.hpp"

#include "Engine/Graphics/Handles.hpp"

namespace engine::graphics::vulkan {

PipelineCache::PipelineCache(Context& context) : context_(context) {
  createDescriptorSetLayout_();
}

PipelineCache::~PipelineCache() {
  const auto vkDevice = context_.GetDevice().Logical();
  vkDestroyDescriptorSetLayout(vkDevice, descriptorSetLayout_, nullptr);
}

VkDescriptorSetLayout PipelineCache::DescriptorSetLayout() const noexcept {
  return descriptorSetLayout_;
}

PipelineHandle PipelineCache::CreatePipeline(const PipelineCreateInfo& info, RenderPass& renderPass) {
  return PipelineHandle{pipelines_.Create(context_, info, renderPass, descriptorSetLayout_)};
}

Pipeline& PipelineCache::GetPipeline(const uint32_t pipelineID) noexcept {
  return pipelines_.Get(pipelineID);
}

void PipelineCache::DestroyPipeline(const uint32_t pipelineID) noexcept {
  pipelines_.Delete(pipelineID);
}

void PipelineCache::createDescriptorSetLayout_() {
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

} // namespace engine::graphics::vulkan
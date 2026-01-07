#pragma once

#include "Pipeline.hpp"

#include "Engine/Containers/HandlePool.hpp"

namespace engine::graphics {
struct PipelineCreateInfo;
} // namespace engine::graphics

namespace engine::graphics::vulkan {

class Context;
class RenderPass;

class PipelineCache {
public:
  explicit PipelineCache(Context& context);
  ~PipelineCache();

  PipelineCache(const PipelineCache&) = delete;
  PipelineCache& operator=(const PipelineCache&) = delete;

  VkDescriptorSetLayout DescriptorSetLayout() const noexcept;

  PipelineHandle CreatePipeline(const PipelineCreateInfo& info, RenderPass& renderPass);
  Pipeline& GetPipeline(uint32_t pipelineID) noexcept;
  void DestroyPipeline(uint32_t pipelineID) noexcept;

  void createDescriptorSetLayout_();

private:
  Context& context_;
  containers::HandlePool<Pipeline> pipelines_;

  VkDescriptorSetLayout descriptorSetLayout_{VK_NULL_HANDLE};
};

} // namespace engine::graphics::vulkan
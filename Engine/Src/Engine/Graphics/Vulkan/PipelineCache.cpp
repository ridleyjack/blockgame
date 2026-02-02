#include "PipelineCache.hpp"

#include "Context.hpp"
#include "Pipeline.hpp"

namespace engine::graphics::vulkan {

PipelineCache::PipelineCache(Context& context) : context_(context) {}

std::expected<std::uint32_t, PipelineError>
PipelineCache::CreatePipeline(const PipelineCreateInfo& info,
                              const RenderPass& renderPass,
                              VkDescriptorSetLayout descriptorSetLayout) noexcept {

  auto pipeline = Pipeline::Create(context_, renderPass, info, descriptorSetLayout);
  if (!pipeline)
    return std::unexpected(pipeline.error());

  return pipelines_.Create(std::move(*pipeline));
}

Pipeline& PipelineCache::GetPipeline(const std::uint32_t pipelineID) noexcept {
  return pipelines_.Get(pipelineID);
}

void PipelineCache::DestroyPipeline(const std::uint32_t pipelineID) noexcept {
  pipelines_.Delete(pipelineID);
}
} // namespace engine::graphics::vulkan
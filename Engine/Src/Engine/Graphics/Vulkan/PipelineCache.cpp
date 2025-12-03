#include "PipelineCache.hpp"

#include "Pipeline.hpp"

#include "Engine/Graphics/Handles.hpp"

namespace engine::graphics::vulkan {

PipelineCache::PipelineCache(Context& context) : context_(context) {}

PipelineHandle PipelineCache::CreatePipeline(const PipelineCreateInfo& info, RenderPass& renderPass) {
  return PipelineHandle{pipelines_.Create(context_, info, renderPass)};
}

Pipeline& PipelineCache::GetPipeline(const PipelineHandle handle) noexcept {
  return pipelines_.Get(handle.PipelineID);
}
void PipelineCache::DestroyPipeline(const PipelineHandle handle) noexcept {
  pipelines_.Delete(handle.PipelineID);
}

} // namespace engine::graphics::vulkan
#include "PipelineCache.hpp"

#include "Context.hpp"
#include "Pipeline.hpp"

#include <algorithm>
#include <cassert>

namespace engine::graphics::vulkan {

PipelineCache::PipelineCache(Context& context) : context_(context) {}

std::expected<std::uint32_t, PipelineError>
PipelineCache::CreatePipeline(const PipelineCreateInfo& info,
                              std::span<const VkDescriptorSetLayout> descriptorSetLayouts) noexcept {

  auto pipeline = Pipeline::Create(context_, info, descriptorSetLayouts);
  if (!pipeline)
    return std::unexpected(pipeline.error());

  return pipelines_.Create(std::move(*pipeline));
}

Pipeline& PipelineCache::GetPipeline(const std::uint32_t pipelineID) noexcept {
  return pipelines_.Get(pipelineID);
}

void PipelineCache::DestroyPipelineDeferred(const std::uint32_t pipelineID, const std::uint32_t retireFrame) noexcept {
  assert(pipelines_.Contains(pipelineID));

  const auto alreadyPending = std::ranges::any_of(pendingDeletes_, [pipelineID](const PendingDelete& pending) {
    return pending.PipelineID == pipelineID;
  });
  if (alreadyPending)
    return;

  pendingDeletes_.push_back({.PipelineID = pipelineID, .RetireFrame = retireFrame});
}

void PipelineCache::ProcessDeferredDeletions(const std::uint32_t currentFrame) noexcept {
  std::erase_if(pendingDeletes_, [&](const PendingDelete& pending) {
    if (pending.RetireFrame != currentFrame)
      return false;

    if (pipelines_.Contains(pending.PipelineID))
      pipelines_.Delete(pending.PipelineID);

    return true;
  });
}

} // namespace engine::graphics::vulkan

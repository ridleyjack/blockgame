#include "VulkanPipelineCache.hpp"

#include "VulkanContext.hpp"
#include "VulkanPipeline.hpp"

namespace Engine {

// ==============================
// Public Methods
// ==============================

PipelineCache::PipelineCache(Context& context) : context_(context) {}

PipelineCache::~PipelineCache() {
  pipelines_.clear();
}

size_t PipelineCache::CreatePipeline(const CreatePipelineRequest& request) {
  auto pipeline = std::make_unique<Pipeline>(context_);
  pipeline->Create(request);
  pipelines_.push_back(std::move(pipeline));
  return pipelines_.size() - 1;
}

Pipeline& PipelineCache::GetPipeline(const size_t pipelineID) {
  return *pipelines_.at(pipelineID);
}

// ==============================
// Private Methods
// ==============================

} // namespace Engine
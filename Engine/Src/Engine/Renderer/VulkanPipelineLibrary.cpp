#include "VulkanPipelineLibrary.hpp"

#include "VulkanContext.hpp"
#include "VulkanPipeline.hpp"

namespace Engine {

// ==============================
// Public Methods
// ==============================

PipelineLibrary::PipelineLibrary(Context& context) : context_(context) {}

PipelineLibrary::~PipelineLibrary() {
  pipelines_.clear();
}

size_t PipelineLibrary::CreatePipeline(const CreatePipelineRequest& request) {
  auto pipeline = std::make_unique<Pipeline>(context_);
  pipeline->Create(request);
  pipelines_.push_back(std::move(pipeline));
  return pipelines_.size() - 1;
}

Pipeline& PipelineLibrary::GetPipeline(const size_t pipelineID) {
  return *pipelines_.at(pipelineID);
}

// ==============================
// Private Methods
// ==============================

} // namespace Engine
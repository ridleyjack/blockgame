#pragma once

#include "Pipeline.hpp"

#include "Engine/Containers/HandlePool.hpp"

#include <expected>
#include <span>
#include <vector>

namespace engine::graphics {
struct PipelineCreateInfo;
} // namespace engine::graphics

namespace engine::graphics::vulkan {

class Context;

class PipelineCache {
public:
  explicit PipelineCache(Context& context);

  PipelineCache(const PipelineCache&) = delete;
  PipelineCache& operator=(const PipelineCache&) = delete;

  std::expected<std::uint32_t, PipelineError>
  CreatePipeline(const PipelineCreateInfo& info, std::span<const VkDescriptorSetLayout> descriptorSetLayouts) noexcept;
  Pipeline& GetPipeline(std::uint32_t pipelineID) noexcept;
  void DestroyPipelineDeferred(std::uint32_t pipelineID, std::uint32_t retireFrame) noexcept;
  void ProcessDeferredDeletions(std::uint32_t currentFrame) noexcept;

private:
  struct PendingDelete {
    std::uint32_t PipelineID{};
    std::uint32_t RetireFrame{};
  };

  Context& context_;
  containers::HandlePool<Pipeline> pipelines_;
  std::vector<PendingDelete> pendingDeletes_{};
};

} // namespace engine::graphics::vulkan

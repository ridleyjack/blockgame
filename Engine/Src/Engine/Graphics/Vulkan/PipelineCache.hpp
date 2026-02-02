#pragma once

#include "Pipeline.hpp"

#include "Engine/Containers/HandlePool.hpp"

#include <expected>

namespace engine::graphics {
struct PipelineCreateInfo;
} // namespace engine::graphics

namespace engine::graphics::vulkan {

class Context;
class RenderPass;

class PipelineCache {
public:
  explicit PipelineCache(Context& context);

  PipelineCache(const PipelineCache&) = delete;
  PipelineCache& operator=(const PipelineCache&) = delete;

  std::expected<std::uint32_t, PipelineError> CreatePipeline(const PipelineCreateInfo& info,
                                                             const RenderPass& renderPass,
                                                             VkDescriptorSetLayout descriptorSetLayout) noexcept;
  Pipeline& GetPipeline(std::uint32_t pipelineID) noexcept;
  void DestroyPipeline(std::uint32_t pipelineID) noexcept;

private:
  Context& context_;
  containers::HandlePool<Pipeline> pipelines_;
};

} // namespace engine::graphics::vulkan
#pragma once

#include "Pipeline.hpp"

#include "Engine/Containers/HandlePool.hpp"

namespace engine::graphics {
struct PipelineCreateInfo;
struct PipelineHandle;
} // namespace engine::graphics

namespace engine::graphics::vulkan {

class Context;
class RenderPass;

class PipelineCache {
public:
  explicit PipelineCache(Context& context);
  ~PipelineCache() = default;

  PipelineCache(const PipelineCache&) = delete;
  PipelineCache& operator=(const PipelineCache&) = delete;

  PipelineHandle CreatePipeline(const PipelineCreateInfo& info, RenderPass& renderPass);
  Pipeline& GetPipeline(PipelineHandle handle) noexcept;
  void DestroyPipeline(PipelineHandle handle) noexcept;

private:
  Context& context_;
  core::containers::HandlePool<Pipeline> pipelines_;
};

} // namespace engine::graphics::vulkan
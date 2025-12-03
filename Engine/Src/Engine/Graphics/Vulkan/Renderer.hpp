#pragma once

#include "Context.hpp"
#include "FramebufferSet.hpp"
#include "PipelineCache.hpp"
#include "RenderPass.hpp"

#include "Engine/Graphics/RenderObject.hpp"
#include "Engine/Core/Containers/HandlePool.hpp"

struct GLFWwindow;

namespace engine::graphics {
struct MeshHandle;
struct MeshCreateInfo;

struct PipelineHandle;
struct PipelineCreateInfo;
} // namespace engine::graphics

namespace engine::graphics::vulkan {

class Context;
class PipelineCache;

class Renderer {
public:
  explicit Renderer(GLFWwindow* window);
  ~Renderer() = default;

  Renderer(const Renderer&) = delete;
  Renderer& operator=(const Renderer&) = delete;

  void DrawFrame();

  bool ShouldClose() const noexcept;
  void WaitUntilIdle() const;

  PipelineHandle CreatePipeline(const PipelineCreateInfo& info);
  void DeletePipeline(PipelineHandle Handle);

  MeshHandle CreateMesh(const MeshCreateInfo& info);
  void DeleteMesh(MeshHandle Handle);

private:
  uint32_t currentFrame_{};

  Context context_;
  PipelineCache pipelineCache_;
  core::containers::HandlePool<FramebufferSet> framebuffers_;
  core::containers::HandlePool<RenderPass> renderPasses_;

  RenderObject renderObject_;
};

} // namespace engine::graphics::vulkan
#pragma once

#include "Context.hpp"
#include "FramebufferSet.hpp"
#include "PipelineCache.hpp"
#include "RenderPass.hpp"
#include "MeshGPU.hpp"
#include "UniformBuffer.hpp"

#include <expected>

struct GLFWwindow;

namespace engine::graphics {
struct MeshHandle;
struct Mesh;
struct MeshCreateInfo;

struct PipelineHandle;
struct PipelineCreateInfo;
} // namespace engine::graphics

namespace engine::graphics::vulkan {

class Context;

class PipelineCache;
struct MeshGPU;

enum class RenderError {
  FrameAcquireFailed,
  FrameOutOfDate,
  RecordCommandFailed,
  SubmitCommandFailed,
  PresentFailed
};

struct FrameContext {
  uint32_t ImageIndex{};
};

class Renderer {
public:
  explicit Renderer(GLFWwindow* window);
  ~Renderer();

  Renderer(const Renderer&) = delete;
  Renderer& operator=(const Renderer&) = delete;

  std::expected<FrameContext, RenderError> BeginFrame() noexcept;
  std::expected<void, RenderError> EndFrame(const FrameContext& ctx);
  void Submit(const MeshHandle& handle);

  bool ShouldClose() const noexcept;
  void WaitUntilIdle() const;

  PipelineHandle CreatePipeline(const PipelineCreateInfo& info);
  void DeletePipeline(PipelineHandle handle);

  MeshHandle CreateMesh(const Mesh& mesh);
  void DeleteMesh(MeshHandle handle);

private:
  Context context_;

  PipelineCache pipelineCache_;
  core::containers::HandlePool<FramebufferSet> framebuffers_;
  core::containers::HandlePool<RenderPass> renderPasses_;
  core::containers::HandlePool<MeshGPU> meshes_;
  core::containers::HandlePool<UniformBuffer> uniforms_;

  VkDescriptorPool descriptorPool_{VK_NULL_HANDLE};
  std::vector<VkDescriptorSet> descriptorSets_{};

  uint32_t currentFrame_{};

  AllocatedBuffer createVertexBuffer_(const Mesh& mesh);
  AllocatedBuffer createIndexBuffer_(const Mesh& mesh);
};

} // namespace engine::graphics::vulkan
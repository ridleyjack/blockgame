#pragma once

#include "Context.hpp"
#include "DescriptorAllocator.hpp"
#include "MeshAllocator.hpp"
#include "PipelineCache.hpp"
#include "RenderPassCache.hpp"
#include "UniformBuffer.hpp"
#include "TextureAllocator.h"
#include "Engine/Graphics/Handles.hpp"

#include <expected>

namespace engine::graphics::vulkan {}
struct GLFWwindow;

namespace engine::graphics {
struct MeshHandle;
struct Mesh;
struct MeshCreateInfo;

struct PipelineHandle;
struct PipelineCreateInfo;

struct TextureHandle;

struct RenderPassHandle;
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
  uint32_t CurrentFrame{};
  uint32_t ImageIndex{};
  uint32_t RenderPassID{};
  uint32_t PipelineID{};
};

class Renderer {
public:
  explicit Renderer(GLFWwindow* window);
  ~Renderer();

  Renderer(const Renderer&) = delete;
  Renderer& operator=(const Renderer&) = delete;

  std::expected<void, RenderError> BeginFrame(const RenderPassHandle& renderPassHandle,
                                              const PipelineHandle& pipelineHandle) noexcept;
  std::expected<void, RenderError> EndFrame();
  void Submit(const MeshHandle& handle);

  bool ShouldClose() const noexcept;
  void WaitUntilIdle() const;

  RenderPassHandle CreateRenderPass();

  PipelineHandle CreatePipeline(const PipelineCreateInfo& info);
  void DeletePipeline(PipelineHandle handle);

  MeshHandle CreateMesh(const Mesh& mesh);
  void DeleteMesh(MeshHandle handle);

  TextureHandle CreateTexture(std::span<const std::byte> data, int width, int height);

private:
  Context context_;

  RenderPassCache renderPassCache_;
  PipelineCache pipelineCache_;

  DescriptorAllocator descriptorAllocator_;
  TextureAllocator textureAllocator_;
  MeshAllocator meshAllocator_;

  UniformBuffer cameraGPU_{};

  FrameContext frameContext_{};
};

} // namespace engine::graphics::vulkan
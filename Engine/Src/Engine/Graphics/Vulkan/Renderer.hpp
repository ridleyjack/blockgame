#pragma once

#include "Context.hpp"
#include "DescriptorAllocator.hpp"
#include "MeshAllocator.hpp"
#include "RenderPassCache.hpp"
#include "UniformBuffer.hpp"
#include "TextureAllocator.h"
#include "PipelineCache.hpp"

#include <expected>

struct GLFWwindow;

namespace engine::graphics {
class CameraMatrices;

struct MeshHandle;
struct Mesh;
struct MeshCreateInfo;

struct PipelineHandle;
struct PipelineCreateInfo;

struct TextureHandle;

struct RenderPassHandle;
} // namespace engine::graphics

namespace engine::graphics::vulkan {
class PipelineCache;
struct MeshGPU;

enum class RenderError : uint8_t {
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

  UniformBuffer CameraGPU{};
};

class Renderer {
public:
  explicit Renderer(GLFWwindow* window);
  ~Renderer();

  Renderer(const Renderer&) = delete;
  Renderer& operator=(const Renderer&) = delete;

  std::expected<void, RenderError> BeginFrame(const RenderPassHandle& renderPassHandle,
                                              const PipelineHandle& pipelineHandle,
                                              const CameraMatrices& camera) noexcept;
  std::expected<void, RenderError> EndFrame();
  void Submit(const MeshHandle& handle, const MaterialHandle& material);

  bool ShouldClose() const noexcept;
  void WaitUntilIdle() const;

  RenderPassHandle CreateRenderPass();

  PipelineHandle CreatePipeline(const PipelineCreateInfo& info);
  void DeletePipeline(const PipelineHandle& handle);

  MeshHandle CreateMesh(const Mesh& mesh);
  void DeleteMesh(const MeshHandle& handle);

  TextureHandle CreateTexture(std::span<const std::byte> data, int width, int height);

  MaterialHandle CreateMaterial(const TextureHandle& texture);

  glm::mat4 MakeProjection() const noexcept;

private:
  Context context_;

  RenderPassCache renderPassCache_;
  PipelineCache pipelineCache_;

  DescriptorAllocator descriptorAllocator_;
  TextureAllocator textureAllocator_;
  MeshAllocator meshAllocator_;

  FrameContext frameContext_{};
};

} // namespace engine::graphics::vulkan
#pragma once

#include "Context.hpp"
#include "DescriptorAllocator.hpp"
#include "MeshAllocator.hpp"
#include "MeshBuffer.hpp"
#include "UniformBuffer.hpp"
#include "TextureAllocator.hpp"
#include "PipelineCache.hpp"
#include "StagingBuffer.hpp"
#include "Uploader.hpp"

#include "Engine/Graphics/ObjectPushConstants.hpp"

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
struct MaterialHandle;

namespace resources {
class TextureArrayBuilder;
struct TextureArrayInfo;
} // namespace resources

namespace vulkan {
class PipelineCache;

enum class RenderError : uint8_t {
  FrameAcquireFailed,
  FrameOutOfDate,
};

struct FrameContext {
  std::uint32_t CurrentFrame{};
  std::uint32_t ImageIndex{};

  UniformBuffer CameraGPU{};
  bool FrameActive{};
};

class Renderer {
public:
  struct ProjectionSettings {
    float FOV{45.0f};
    float NearPlane{0.1f};
    float FarPlane{512.0f};
  };

  explicit Renderer(GLFWwindow* window);
  ~Renderer();

  Renderer(const Renderer&) = delete;
  Renderer& operator=(const Renderer&) = delete;

  std::expected<void, RenderError> BeginFrame(const CameraMatrices& camera);
  void EndFrame();
  void
  Submit(PipelineHandle pipelineHandle, MeshHandle handle, MaterialHandle material, const ObjectPushConstants& model);

  bool ShouldClose() const noexcept;
  void WaitUntilIdle() const;

  PipelineHandle CreatePipeline(const PipelineCreateInfo& info);
  void DeletePipeline(PipelineHandle handle);

  MeshHandle CreateMesh(const Mesh& mesh);
  void DeleteMesh(MeshHandle handle);
  bool IsMeshReady(MeshHandle mesh) noexcept;

  TextureHandle CreateTexture(const std::span<const std::byte>& data, int width, int height);
  resources::TextureArrayBuilder BeginTextureArray(const resources::TextureArrayInfo& info);

  MaterialHandle CreateMaterial(TextureHandle texture);

  glm::mat4 MakeProjection(const ProjectionSettings& settings) const noexcept;

  void SetFramebufferResized(bool hasResized) noexcept;

private:
  Context context_;

  PipelineCache pipelineCache_;

  StagingBuffer stagingBuffer_;
  Uploader uploader_;
  TextureAllocator textureAllocator_;
  MeshBuffer meshBuffer_;
  MeshAllocator meshAllocator_;
  DescriptorAllocator descriptorAllocator_;

  FrameContext frameContext_{};
};
} // namespace vulkan
} // namespace engine::graphics

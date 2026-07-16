#pragma once

#include "Context.hpp"
#include "DescriptorAllocator.hpp"
#include "MeshAllocator.hpp"
#include "MeshBuffer.hpp"
#include "TextureAllocator.hpp"
#include "PipelineCache.hpp"
#include "ShaderDataAllocator.hpp"
#include "StagingBuffer.hpp"
#include "Uploader.hpp"

#include "Engine/Graphics/Color.hpp"
#include "Engine/Graphics/Handles.hpp"

#include <expected>

struct GLFWwindow;

namespace engine::graphics {
class CameraMatrices;

struct Mesh;
struct MeshCreateInfo;

struct SubmitInfo;

struct PipelineCreateInfo;

namespace resources {
class TextureArrayBuilder;
struct TextureArrayInfo;
} // namespace resources

namespace vulkan {
class PipelineCache;

enum class RenderError : uint8_t {
  FrameOutOfDate,
};

struct CameraShaderData {
  alignas(16) glm::mat4 View{1.0f};
  alignas(16) glm::mat4 Projection{1.0f};
};

struct FrameContext {
  std::uint32_t CurrentFrame{};
  std::uint32_t ImageIndex{};

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

  std::expected<void, RenderError> BeginFrame(const CameraMatrices& camera, Color clearColor);
  void EndFrame();
  void Submit(const SubmitInfo& info);

  bool ShouldClose() const noexcept;
  void WaitUntilIdle() const;

  PipelineHandle CreatePipeline(const PipelineCreateInfo& info);
  void DeletePipeline(PipelineHandle handle);

  MeshHandle CreateMesh(const Mesh& mesh);
  void DeleteMesh(MeshHandle handle);
  bool IsMeshReady(MeshHandle mesh) noexcept;

  TextureHandle CreateTexture(const std::span<const std::byte>& data, int width, int height);
  resources::TextureArrayBuilder BeginTextureArray(const resources::TextureArrayInfo& info);

  template <ShaderDataStruct T> ShaderDataHandle<T> CreateShaderData() {
    const std::uint32_t id = shaderDataAllocator_.AllocateShaderData(sizeof(T));
    return ShaderDataHandle<T>{.ShaderDataID = id};
  }

  template <ShaderDataStruct T> void SetShaderData(ShaderDataHandle<T> handle, const T& data) {
    shaderDataAllocator_.WriteShaderData(handle.ShaderDataID,
                                         frameContext_.CurrentFrame,
                                         std::as_bytes(std::span{&data, 1}));
  }

  template <ShaderDataStruct T> void DeleteShaderData(ShaderDataHandle<T> handle) {
    shaderDataAllocator_.FreeShaderDataDeferred(handle.ShaderDataID, retireFrameForDeletion_());
  }

  MaterialHandle CreateMaterial(TextureHandle texture);

  glm::mat4 MakeProjection(const ProjectionSettings& settings) const noexcept;

  void RebuildFrameBuffer() noexcept;

private:
  Context context_;

  PipelineCache pipelineCache_;

  StagingBuffer stagingBuffer_;
  Uploader uploader_;
  TextureAllocator textureAllocator_;
  MeshBuffer meshBuffer_;
  MeshAllocator meshAllocator_;
  DescriptorAllocator descriptorAllocator_;
  ShaderDataAllocator shaderDataAllocator_;

  FrameContext frameContext_{};

  std::uint32_t cameraShaderDataID_{};

  std::uint32_t retireFrameForDeletion_() const noexcept;

  bool rebuildFrameBuffer_ = false;
};
} // namespace vulkan
} // namespace engine::graphics

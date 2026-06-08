#pragma once

#include "Device.hpp"
#include "Engine/Memory/RingBuffer.hpp"

#include <vulkan/vulkan.h>

#include <cstddef>
#include <cstdint>
#include <expected>
#include <optional>
#include <span>
#include <variant>
#include <vector>

namespace engine::graphics {

namespace resources {
struct TextureArrayInfo;
}

namespace vulkan {

namespace image {
enum class Error : uint8_t;
struct ImageViewSpec;
} // namespace image

class Context;
class Uploader;
class StagingBuffer;

struct TextureError {
  enum class ErrorCode : std::uint8_t {
    FailedSamplerCreation,
    ImageUtilFailure,
    StagingBufferFailure
  };
  ErrorCode Code{};

  std::variant<std::monostate, image::Error, memory::RingBuffer::AllocateError> Cause{};
};

enum class TextureState : std::uint8_t {
  Uploading,
  Ready,
};

struct TextureGPU {
  VkImage Image{VK_NULL_HANDLE};
  VkDeviceMemory ImageMemory{VK_NULL_HANDLE};

  VkImageView ImageView{VK_NULL_HANDLE};
  VkSampler Sampler{VK_NULL_HANDLE};

  TextureState State{TextureState::Uploading};

  std::uint32_t Width{};
  std::uint32_t Height{};
  std::uint32_t MipLevels{};
};

class TextureAllocator {
public:
  TextureAllocator(Context& context, Uploader& uploader, StagingBuffer& staging);
  ~TextureAllocator();

  std::uint32_t Create(const std::span<const std::byte>& imageData, std::uint32_t width, std::uint32_t height);

  const TextureGPU& Get(std::uint32_t textureID) const noexcept;

  // BeginArray creates a Texture with multiple layers. A texture can be uploaded to each layer. FinishArray must be
  // called before another array can be started.
  void BeginArray(const resources::TextureArrayInfo& info);
  void UploadLayer(const std::span<const std::byte>& imageData);
  std::uint32_t FinishArray();

private:
  Context& context_;
  Uploader& uploader_;
  StagingBuffer& stagingBuffer_;

  std::vector<TextureGPU> textures_;

  struct ArrayBuildState {
    TextureGPU Texture{};

    VkDeviceSize LayerSizeBytes{};
    std::uint32_t NumLayers{};
    std::uint32_t NextLayer{};
  };
  std::optional<ArrayBuildState> arrayState_{};

  std::expected<TextureGPU, TextureError>
  createImage_(VkCommandBuffer cmd, std::uint32_t width, std::uint32_t height, std::uint32_t numLayers) const;

  std::expected<void, TextureError> uploadLayer_(const TextureGPU& texture,
                                                 VkCommandBuffer cmd,
                                                 std::uint64_t batchID,
                                                 std::uint32_t layerIndex,
                                                 const std::span<const std::byte>& bytes) const;

  std::expected<std::uint32_t, TextureError>
  finishTexture_(VkCommandBuffer cmd, TextureGPU& texture, std::uint32_t numLayers);

  void copyBufferToImage_(VkCommandBuffer cmd,
                          VkDeviceSize stagingOffset,
                          VkImage image,
                          std::uint32_t width,
                          std::uint32_t height,
                          std::uint32_t layerIndex) const;

  std::expected<VkImageView, TextureError> createTextureImageView_(VkImage image,
                                                                   const image::ImageViewSpec& spec) const;
  std::expected<VkSampler, TextureError> createSampler_() const;

  void generateMipmaps_(VkCommandBuffer cmd,
                        VkImage image,
                        std::uint32_t width,
                        std::uint32_t height,
                        std::uint32_t layerIndex,
                        std::uint32_t mipLevels) const noexcept;

  void cleanupGPUTexture_(TextureGPU& texture) const noexcept;
};
} // namespace vulkan

} // namespace engine::graphics

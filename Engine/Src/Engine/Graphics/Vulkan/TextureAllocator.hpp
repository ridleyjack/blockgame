#pragma once

#include "Device.hpp"
#include "TextureAllocator.hpp"

#include <vulkan/vulkan.h>

#include <cstddef>
#include <cstdint>
#include <expected>
#include <span>
#include <string_view>
#include <variant>
#include <vector>

namespace engine::graphics {

struct TextureArrayInfo;

namespace vulkan {

namespace imageutil {
enum class Error;
}

class Context;
class Uploader;
class StagingBuffer;

struct TextureError {
  enum class ErrorCode : std::uint8_t {
    FailedSamplerCreation,
    ImageUtilFailure
  };
  ErrorCode Code;

  std::variant<std::monostate, imageutil::Error> Cause{};
};

inline std::string_view ToString(const TextureError& e) noexcept {
  using enum TextureError::ErrorCode;

  switch (e.Code) {
  case FailedSamplerCreation:
    return "FailedSamplerCreation";
  case ImageUtilFailure:
    return "ImageUtilFailure";
  }
  return "Unknown TextureError";
}

enum class TextureState : std::uint8_t {
  Uploading,
  Ready,
  Failed
};

struct TextureGPU {
  VkImage Image{VK_NULL_HANDLE};
  VkDeviceMemory ImageMemory{VK_NULL_HANDLE};

  VkImageView ImageView{VK_NULL_HANDLE};
  VkSampler Sampler{VK_NULL_HANDLE};

  TextureState State{TextureState::Uploading};
  std::optional<TextureError> Error{};
};

class TextureAllocator {
public:
  TextureAllocator(Context& context, Uploader& uploader, StagingBuffer& staging);
  ~TextureAllocator();

  std::uint32_t Create(const std::span<const std::byte>& imageData, std::uint32_t width, std::uint32_t height);
  const TextureGPU& Get(std::uint32_t textureID) const noexcept;

  // BeginArray creates a Texture with multiple layers. A texture can be uploaded to each layer. FinishArray must be
  // called before another array can be started.
  void BeginArray(const TextureArrayInfo& info) noexcept;
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
    std::uint32_t Width{}, Height{};
    std::uint32_t NumLayers{};
    std::uint32_t NextLayer{};
  };
  std::optional<ArrayBuildState> arrayState_{};

  std::expected<TextureGPU, TextureError>
  createImage_(VkCommandBuffer cmd, std::uint32_t width, std::uint32_t height, std::uint32_t numLayers) const noexcept;

  void uploadLayer_(const TextureGPU& texture,
                    VkCommandBuffer cmd,
                    std::uint64_t batchID,
                    std::uint32_t width,
                    std::uint32_t height,
                    std::uint32_t layerIndex,
                    const std::span<const std::byte>& bytes) const noexcept;

  std::expected<void, TextureError>
  finishTexture_(VkCommandBuffer cmd, TextureGPU& texture, std::uint32_t numLayers) noexcept;

  void copyBufferToImage_(VkCommandBuffer cmd,
                          VkDeviceSize stagingOffset,
                          VkImage image,
                          uint32_t width,
                          uint32_t height,
                          uint32_t layerIndex) const noexcept;

  std::expected<VkImageView, TextureError>
  createTextureImageView_(VkImage image, VkFormat format, uint32_t arrayLayers) const noexcept;
  std::expected<VkSampler, TextureError> createSampler_() const noexcept;

  void cleanupGPUTexture_(TextureGPU& texture) const noexcept;
};
} // namespace vulkan

} // namespace engine::graphics
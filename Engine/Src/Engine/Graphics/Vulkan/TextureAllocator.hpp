#pragma once

#include "Device.hpp"

#include <vulkan/vulkan.h>

#include <cstddef>
#include <cstdint>
#include <expected>
#include <span>
#include <string_view>
#include <variant>
#include <vector>

namespace engine::graphics::vulkan {

namespace imageutil {
enum class Error;
}

class Context;

struct TextureGPU {
  VkImage Image{VK_NULL_HANDLE};
  VkDeviceMemory ImageMemory{VK_NULL_HANDLE};

  VkImageView ImageView{VK_NULL_HANDLE};
  VkSampler Sampler{VK_NULL_HANDLE};
};

struct TextureStaging {
  AllocatedBuffer Buffer{};
  void* Data{};
};

struct TextureError {
  enum class ErrorCode : std::uint8_t {
    FailedSamplerCreation,
    ImageUtilFailure
  };
  ErrorCode Code;

  std::variant<std::monostate, imageutil::Error> Cause{};
};

constexpr std::string_view ToString(const TextureError& e) noexcept {
  using enum TextureError::ErrorCode;

  switch (e.Code) {
  case FailedSamplerCreation:
    return "FailedSamplerCreation";
  case ImageUtilFailure:
    return "ImageUtilFailure";
  }
  return "Unknown TextureError";
}

class TextureAllocator {
public:
  explicit TextureAllocator(Context& context);
  ~TextureAllocator();

  std::expected<uint32_t, TextureError>
  Create(const std::span<const std::byte>& imageData, uint32_t width, uint32_t height);
  TextureGPU& Get(uint32_t textureID) noexcept;

  std::expected<void, TextureError>
  BeginArray(std::uint32_t width, std::uint32_t height, VkDeviceSize imageSize, std::uint32_t numLayers) noexcept;
  void UploadLayer(const std::span<const std::byte>& imageData);
  std::expected<std::uint32_t, TextureError> FinishArray();

private:
  Context& context_;

  std::vector<TextureGPU> textures_;

  struct ArrayBuildState {
    TextureGPU Texture{};
    TextureStaging Staging{};

    VkDeviceSize LayerSizeBytes{};
    std::uint32_t Width{}, Height{};
    std::uint32_t NumLayers{};
    std::uint32_t NextLayer{};
  };

  std::optional<ArrayBuildState> arrayState_{};

  std::expected<TextureGPU, TextureError>
  createImage_(std::uint32_t width, std::uint32_t height, std::uint32_t numLayers) const noexcept;

  TextureStaging createStaging_(VkDeviceSize layerSizeBytes) const noexcept;

  void uploadLayer_(const TextureGPU& texture,
                    const TextureStaging& staging,
                    std::uint32_t width,
                    std::uint32_t height,
                    VkDeviceSize layerSizeBytes,
                    std::uint32_t layerIndex,
                    const std::span<const std::byte>& bytes) const noexcept;

  // finishTexture_ guarantees that TextureStaging is freed.
  std::expected<uint32_t, TextureError>
  finishTexture_(TextureGPU& texture, const TextureStaging& staging, std::uint32_t numLayers) noexcept;

  void copyBufferToImage_(VkBuffer buffer,
                          VkImage image,
                          uint32_t width,
                          uint32_t height,
                          uint32_t layerIndex) const noexcept;

  std::expected<VkImageView, TextureError>
  createTextureImageView_(VkImage image, VkFormat format, uint32_t arrayLayers) const noexcept;
  std::expected<VkSampler, TextureError> createSampler_() const noexcept;

  void cleanupGPUTexture_(TextureGPU& texture) const noexcept;
};

} // namespace engine::graphics::vulkan
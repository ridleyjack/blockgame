#pragma once

#include <vulkan/vulkan.h>

#include <cstddef>
#include <cstdint>
#include <expected>
#include <span>
#include <variant>
#include <vector>

namespace engine::graphics::vulkan {

namespace imageutil {
enum class Error;
}

class Context;

struct TextureGPU {
  VkImage Image;
  VkDeviceMemory ImageMemory;

  VkImageView ImageView;
  VkSampler Sampler;
};

struct TextureError {
  enum class ErrorCode : std::uint8_t {
    FailedSamplerCreation,
    ImageUtilFailure
  };
  ErrorCode Code;

  std::variant<std::monostate, imageutil::Error> Cause{};
};

class TextureAllocator {
public:
  explicit TextureAllocator(Context& context);
  ~TextureAllocator();

  std::expected<uint32_t, TextureError> Create(std::span<const std::byte> pixels, uint32_t width, uint32_t height);
  TextureGPU& Get(uint32_t textureID) noexcept;

private:
  Context& context_;

  std::vector<TextureGPU> textures_;

  void copyBufferToImage_(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) const noexcept;

  std::expected<VkImageView, TextureError> createTextureImageView_(VkImage image, VkFormat format) const;
  std::expected<VkSampler, TextureError> createSampler_() const;
};

} // namespace engine::graphics::vulkan
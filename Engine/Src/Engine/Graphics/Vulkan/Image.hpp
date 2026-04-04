#pragma once

#include <cstdint>
#include <expected>
#include <string_view>
#include <vulkan/vulkan.h>

namespace engine::graphics::vulkan {
class Device;
class Command;

namespace image {
enum class Error : std::uint8_t {
  UnsupportedTransition,
  ImageCreationFailed,
  ImageMemoryAllocationFailed,
  ImageViewCreationFailed
};

constexpr std::string_view ToString(const Error e) noexcept {
  using enum Error;

  switch (e) {
  case UnsupportedTransition:
    return "UnsupportedTransition";
  case ImageCreationFailed:
    return "ImageCreationFailed";
  case ImageMemoryAllocationFailed:
    return "ImageMemoryAllocationFailed";
  case ImageViewCreationFailed:
    return "ImageViewCreationFailed";
  }

  return "UnknownImageUtilError";
}

struct ImageSpec {
  std::uint32_t Width{};
  std::uint32_t Height{};
  std::uint32_t MipLevels{1};
  std::uint32_t ArrayLayers{1};

  VkFormat Format{};
  VkImageTiling Tiling{};
  VkImageUsageFlags Usage{};
  VkMemoryPropertyFlags Properties{};
};

struct ImageViewSpec {
  std::uint32_t ArrayLayers{1};
  std::uint32_t MipLevels{1};

  VkFormat Format{};
  VkImageAspectFlags AspectFlags{};
  VkImageViewType ViewType{VK_IMAGE_VIEW_TYPE_2D};
};

struct Image {
  VkImage Handle{VK_NULL_HANDLE};
  VkDeviceMemory Memory{VK_NULL_HANDLE};
};

std::expected<Image, Error> CreateImage(const Device& device, const ImageSpec& spec) noexcept;

std::expected<VkImageView, Error>
CreateImageView(const Device& device, VkImage image, const ImageViewSpec& spec) noexcept;

std::expected<void, Error> TransitionImageLayout(VkCommandBuffer cmd,
                                                 VkImage image,
                                                 const ImageSpec& spec,
                                                 VkImageLayout oldLayout,
                                                 VkImageLayout newLayout) noexcept;

// CalculateMipLevels returns the number of times something of dimensions width x height is divisible by 2.
constexpr std::uint32_t CalculateMipLevels(const std::uint32_t width, const std::uint32_t height) noexcept {
  return std::bit_width(std::max(width, height));
}

} // namespace image
} // namespace engine::graphics::vulkan

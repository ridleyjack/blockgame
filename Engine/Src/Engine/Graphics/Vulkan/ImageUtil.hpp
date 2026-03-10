#pragma once

#include <expected>
#include <string_view>
#include <vulkan/vulkan.h>

namespace engine::graphics::vulkan {
class Device;
class Command;

namespace imageutil {
enum class Error {
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

struct CreateImageInfo {
  uint32_t Width{};
  uint32_t Height{};
  uint32_t ArrayLayers{1};
  VkFormat Format{};
  VkImageTiling Tiling{};
  VkImageUsageFlags Usage{};
  VkMemoryPropertyFlags Properties{};
};

struct CreateImageViewInfo {
  uint32_t ArrayLayers{1};
  VkFormat Format{};
  VkImageAspectFlags AspectFlags{};
  VkImage Image{};
};

struct Image {
  VkImage Handle{VK_NULL_HANDLE};
  VkDeviceMemory Memory{VK_NULL_HANDLE};
};

std::expected<Image, Error> CreateImage(const Device& device, const CreateImageInfo& info);

std::expected<VkImageView, Error> CreateImageView(const Device& device, const CreateImageViewInfo& info);

std::expected<void, Error> TransitionImageLayout(VkCommandBuffer cmd,
                                                 VkImage image,
                                                 VkFormat format,
                                                 VkImageLayout oldLayout,
                                                 VkImageLayout newLayout,
                                                 uint32_t numLayers);

} // namespace imageutil
} // namespace engine::graphics::vulkan

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

std::expected<void, Error> CreateImage(const Device& device,
                                       uint32_t width,
                                       uint32_t height,
                                       VkFormat format,
                                       VkImageTiling tiling,
                                       VkImageUsageFlags usage,
                                       VkMemoryPropertyFlags properties,
                                       VkImage& image,
                                       VkDeviceMemory& imageMemory);

std::expected<VkImageView, Error>
CreateImageView(const Device& device, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);

std::expected<void, Error> TransitionImageLayout(const Command& command,
                                                 VkImage image,
                                                 VkFormat format,
                                                 VkImageLayout oldLayout,
                                                 VkImageLayout newLayout);

} // namespace imageutil
} // namespace engine::graphics::vulkan

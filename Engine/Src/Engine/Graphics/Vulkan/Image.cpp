#include "Image.hpp"

#include "Command.hpp"
#include "Device.hpp"

#include <cassert>

namespace engine::graphics::vulkan::image {

std::expected<Image, Error> CreateImage(const Device& device, const ImageSpec& spec) noexcept {
  auto vkDevice = device.Logical();

  Image result{};

  // Create VKImage
  VkImageCreateInfo imageInfo{
      .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
      .imageType = VK_IMAGE_TYPE_2D,
      .format = spec.Format,
      .extent = {.width = spec.Width, .height = spec.Height, .depth = 1},
      .mipLevels = spec.MipLevels,
      .arrayLayers = spec.ArrayLayers,
      .samples = spec.Samples,
      .tiling = spec.Tiling,
      .usage = spec.Usage,
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
  };

  if (vkCreateImage(vkDevice, &imageInfo, nullptr, &result.Handle) != VK_SUCCESS) {
    return std::unexpected(Error::ImageCreationFailed);
  }

  // Create VKImage Memory
  VkMemoryRequirements memRequirements;
  vkGetImageMemoryRequirements(vkDevice, result.Handle, &memRequirements);

  VkMemoryAllocateInfo allocInfo{
      .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      .allocationSize = memRequirements.size,
      .memoryTypeIndex = device.FindMemoryType(memRequirements.memoryTypeBits, spec.Properties),
  };

  if (vkAllocateMemory(vkDevice, &allocInfo, nullptr, &result.Memory) != VK_SUCCESS) {
    vkDestroyImage(vkDevice, result.Handle, nullptr);
    return std::unexpected(Error::ImageMemoryAllocationFailed);
  }

  vkBindImageMemory(vkDevice, result.Handle, result.Memory, 0);

  return result;
}

std::expected<VkImageView, Error>
CreateImageView(const Device& device, VkImage image, const ImageViewSpec& spec) noexcept {
  VkImageViewCreateInfo viewInfo{
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .image = image,
      .viewType = spec.ViewType,
      .format = spec.Format,
      .subresourceRange = {
                           .aspectMask = spec.AspectFlags,
                           .baseMipLevel = 0,
                           .levelCount = spec.MipLevels,
                           .baseArrayLayer = 0,
                           .layerCount = spec.ArrayLayers,
                           }
  };

  VkImageView imageView;
  if (vkCreateImageView(device.Logical(), &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
    return std::unexpected(Error::ImageViewCreationFailed);
  }
  assert(viewInfo.subresourceRange.levelCount != 0);
  return imageView;
}

std::expected<void, Error> TransitionImageLayout(VkCommandBuffer cmd,
                                                 VkImage image,
                                                 const ImageSpec& spec,
                                                 const VkImageLayout oldLayout,
                                                 const VkImageLayout newLayout) noexcept {
  VkImageMemoryBarrier barrier{};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.oldLayout = oldLayout;
  barrier.newLayout = newLayout;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

  barrier.image = image;
  barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.baseMipLevel = 0;
  barrier.subresourceRange.levelCount = spec.MipLevels;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = spec.ArrayLayers;

  VkPipelineStageFlags sourceStage;
  VkPipelineStageFlags destinationStage;

  // Barriers.
  if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
  } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
             newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {

    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  } else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
  } else {
    return std::unexpected(Error::UnsupportedTransition);
  }

  vkCmdPipelineBarrier(cmd, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
  return {};
}
} // namespace engine::graphics::vulkan::image

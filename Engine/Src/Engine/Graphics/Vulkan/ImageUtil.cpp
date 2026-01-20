#include "ImageUtil.hpp"

#include "Command.hpp"
#include "Device.hpp"

#include <stdexcept>

namespace engine::graphics::vulkan::imageutil {

std::expected<Image, Error> CreateImage(const Device& device, const CreateImageInfo& info) {
  auto vkDevice = device.Logical();

  Image result{};

  // Create VKImage
  VkImageCreateInfo imageInfo{};
  imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageInfo.imageType = VK_IMAGE_TYPE_2D;
  imageInfo.extent.width = info.Width;
  imageInfo.extent.height = info.Height;
  imageInfo.extent.depth = 1;
  imageInfo.mipLevels = 1;
  imageInfo.arrayLayers = info.ArrayLayers;
  imageInfo.format = info.Format;
  imageInfo.tiling = info.Tiling;
  imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageInfo.usage = info.Usage;
  imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;

  if (vkCreateImage(vkDevice, &imageInfo, nullptr, &result.Handle) != VK_SUCCESS) {
    return std::unexpected(Error::ImageCreationFailed);
  }

  // Create VKImage Memory
  VkMemoryRequirements memRequirements;
  vkGetImageMemoryRequirements(vkDevice, result.Handle, &memRequirements);

  VkMemoryAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize = memRequirements.size;
  allocInfo.memoryTypeIndex = device.FindMemoryType(memRequirements.memoryTypeBits, info.Properties);

  if (vkAllocateMemory(vkDevice, &allocInfo, nullptr, &result.Memory) != VK_SUCCESS) {
    vkDestroyImage(vkDevice, result.Handle, nullptr);
    return std::unexpected(Error::ImageMemoryAllocationFailed);
  }

  vkBindImageMemory(vkDevice, result.Handle, result.Memory, 0);

  return result;
}

std::expected<VkImageView, Error> CreateImageView(const Device& device, const CreateImageViewInfo& info) {
  VkImageViewCreateInfo viewInfo{};
  viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  viewInfo.image = info.Image;
  viewInfo.viewType = info.ArrayLayers == 1 ? VK_IMAGE_VIEW_TYPE_2D : VK_IMAGE_VIEW_TYPE_2D_ARRAY;
  viewInfo.format = info.Format;
  viewInfo.subresourceRange.aspectMask = info.AspectFlags;
  viewInfo.subresourceRange.baseMipLevel = 0;
  viewInfo.subresourceRange.levelCount = 1;
  viewInfo.subresourceRange.baseArrayLayer = 0;
  viewInfo.subresourceRange.layerCount = info.ArrayLayers;

  VkImageView imageView;
  if (vkCreateImageView(device.Logical(), &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
    return std::unexpected(Error::ImageViewCreationFailed);
  }

  return imageView;
}

std::expected<void, Error> TransitionImageLayout(const Command& command,
                                                 VkImage image,
                                                 VkFormat format,
                                                 VkImageLayout oldLayout,
                                                 VkImageLayout newLayout,
                                                 uint32_t numLayers) {

  VkCommandBuffer commandBuffer = command.BeginSingleTimeCommands();

  VkImageMemoryBarrier barrier{};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.oldLayout = oldLayout;
  barrier.newLayout = newLayout;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

  barrier.image = image;
  barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.baseMipLevel = 0;
  barrier.subresourceRange.levelCount = 1;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = numLayers;

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

  vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
  command.EndSingleTimeCommands(commandBuffer);
  return {};
}
} // namespace engine::graphics::vulkan::imageutil

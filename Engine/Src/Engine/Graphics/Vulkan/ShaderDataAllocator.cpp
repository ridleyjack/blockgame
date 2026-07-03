#include "ShaderDataAllocator.hpp"

#include "CheckVk.hpp"
#include "Context.hpp"
#include "Device.hpp"

#include <algorithm>
#include <cstring>

namespace engine::graphics::vulkan {
ShaderDataAllocator::ShaderDataAllocator(Context& context) : context_(context) {
  auto& device = context.GetDevice();
  auto vkDevice = device.Logical();

  constexpr VkBufferCreateInfo bufferInfo{
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .size = DefaultBufferMemorySize,
      .usage = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
  };
  CheckVk(vkCreateBuffer(vkDevice, &bufferInfo, nullptr, &buffer_), "vkCreateBuffer");

  VkMemoryRequirements req;
  vkGetBufferMemoryRequirements(vkDevice, buffer_, &req);
  capacity_ = req.size;
  const std::uint32_t memoryType =
      device.FindMemoryType(req.memoryTypeBits,
                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

  VkMemoryAllocateFlagsInfo flags{
      .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO,
      .flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT,
  };
  VkMemoryAllocateInfo allocInfo{
      .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      .pNext = &flags,
      .allocationSize = req.size,
      .memoryTypeIndex = memoryType,
  };
  const VkResult allocateResult = vkAllocateMemory(vkDevice, &allocInfo, nullptr, &memory_);
  if (allocateResult != VK_SUCCESS) {
    vkDestroyBuffer(vkDevice, buffer_, nullptr);
    CheckVk(allocateResult, "vkAllocateMemory");
  }
  const VkResult bindResult = vkBindBufferMemory(vkDevice, buffer_, memory_, 0);
  if (bindResult != VK_SUCCESS) {
    vkDestroyBuffer(vkDevice, buffer_, nullptr);
    vkFreeMemory(vkDevice, memory_, nullptr);
    CheckVk(bindResult, "vkBindBufferMemory");
  }

  void* mapped = nullptr;
  const VkResult mapResult = vkMapMemory(vkDevice, memory_, 0, req.size, 0, &mapped);
  if (mapResult != VK_SUCCESS) {
    vkDestroyBuffer(vkDevice, buffer_, nullptr);
    vkFreeMemory(vkDevice, memory_, nullptr);
    CheckVk(mapResult, "vkMapMemory");
  }
  memoryMapping_ = static_cast<std::byte*>(mapped);

  VkBufferDeviceAddressInfo addressInfo{.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO, .buffer = buffer_};
  deviceAddress_ = vkGetBufferDeviceAddress(vkDevice, &addressInfo);
}

ShaderDataAllocator::~ShaderDataAllocator() {
  auto vkDevice = context_.GetDevice().Logical();
  vkFreeMemory(vkDevice, memory_, nullptr);
  vkDestroyBuffer(vkDevice, buffer_, nullptr);
}

std::uint32_t ShaderDataAllocator::AllocateShaderData(const std::size_t size) {

  ShaderData shaderData{.Size = size};
  for (size_t i = 0; i < shaderData.Offsets.size(); i++) {
    constexpr VkDeviceSize shaderDataAlignment = 16;
    auto result = sparseBuffer_.Allocate(size, shaderDataAlignment);
    if (!result)
      Fatal("Allocating shader data failed");

    shaderData.Offsets[i] = *result;
  }

  return shaderDataPool_.Create(shaderData);
}

void ShaderDataAllocator::FreeShaderDataDeferred(const std::uint32_t id, const std::uint32_t retireFrame) {
  if (!shaderDataPool_.Contains(id))
    return;

  const auto alreadyPending = std::ranges::any_of(pendingDeletes_, [id](const PendingDelete& pending) {
    return pending.ShaderDataID == id;
  });
  if (alreadyPending)
    return;

  pendingDeletes_.push_back({.ShaderDataID = id, .RetireFrame = retireFrame});
}

void ShaderDataAllocator::ProcessDeferredDeletions(const std::uint32_t currentFrame) {
  std::erase_if(pendingDeletes_, [&](const PendingDelete& pending) {
    if (pending.RetireFrame != currentFrame)
      return false;

    freeShaderData_(pending.ShaderDataID);
    return true;
  });
}

void ShaderDataAllocator::WriteShaderData(const std::uint32_t id,
                                          const std::uint32_t frame,
                                          const std::span<const std::byte> data) {
  if (!shaderDataPool_.Contains(id))
    return;

  auto& shaderData = shaderDataPool_.Get(id);
  assert(data.size() == shaderData.Size);
  std::memcpy(memoryMapping_ + shaderData.Offsets[frame], data.data(), shaderData.Size);
}

VkDeviceAddress ShaderDataAllocator::GetShaderDataAddress(const std::uint32_t id,
                                                          const std::uint32_t frame) const noexcept {
  assert(shaderDataPool_.Contains(id));
  return deviceAddress_ + shaderDataPool_.Get(id).Offsets[frame];
}

void ShaderDataAllocator::freeShaderData_(const std::uint32_t id) {
  if (!shaderDataPool_.Contains(id))
    return;

  for (auto& shaderData = shaderDataPool_.Get(id); VkDeviceSize offset : shaderData.Offsets) {
    sparseBuffer_.Free(offset);
  }
  shaderDataPool_.Delete(id);
}

} // namespace engine::graphics::vulkan

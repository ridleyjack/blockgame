#include "TextureArrayBuilder.hpp"

#include "Engine/Graphics/Handles.hpp"
#include "Engine/Graphics/Vulkan/TextureAllocator.hpp"

namespace engine::graphics::resources {
TextureArrayBuilder::TextureArrayBuilder(vulkan::TextureAllocator& allocator) : allocator_(allocator) {}

void TextureArrayBuilder::Upload(const std::span<const std::byte> pixels) const noexcept {
  allocator_.UploadLayer(pixels);
}

std::expected<TextureHandle, vulkan::TextureError> TextureArrayBuilder::Finalize() const noexcept {
  auto result = allocator_.FinishArray();
  if (!result)
    return std::unexpected(result.error());
  return TextureHandle{.TextureID = *result};
}
} // namespace engine::graphics::resources

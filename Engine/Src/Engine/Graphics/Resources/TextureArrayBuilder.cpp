#include "TextureArrayBuilder.hpp"

#include "Engine/Graphics/Handles.hpp"
#include "Engine/Graphics/Vulkan/TextureAllocator.hpp"

namespace engine::graphics::resources {
TextureArrayBuilder::TextureArrayBuilder(vulkan::TextureAllocator& allocator) : allocator_(allocator) {}

void TextureArrayBuilder::Upload(const std::span<const std::byte> pixels) const {
  allocator_.UploadLayer(pixels);
}

TextureHandle TextureArrayBuilder::Finalize() const {
  const std::uint32_t result = allocator_.FinishArray();
  return TextureHandle{.TextureID = result};
}
} // namespace engine::graphics::resources

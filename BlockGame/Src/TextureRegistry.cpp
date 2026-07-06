#include "TextureRegistry.hpp"

#include "Engine/Graphics/Resources/TextureLoader.hpp"
#include "Engine/Graphics/Vulkan/Renderer.hpp"

namespace gfx = engine::graphics;
namespace vlk = gfx::vulkan;

TextureRegistry::TextureRegistry(vlk::Renderer& renderer) {
  std::array<std::string_view, blockTextures_.size()> paths{};
  for (std::size_t i = 0; i < blockTextures_.size(); i++) {
    // Ensure the texture path matches the intended BlockTexture enum.
    assert(static_cast<std::uint32_t>(blockTextures_[i].TextureID) == i);
    paths[i] = blockTextures_[i].Path;
  }

  const gfx::resources::TextureLoader loader(renderer);
  auto texture = loader.LoadArray(paths);

  materialHandle_ = renderer.CreateMaterial(texture);
}

const gfx::MaterialHandle TextureRegistry::GetBlockMaterial() const noexcept {
  return materialHandle_;
}
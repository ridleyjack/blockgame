#include "TextureRegistry.hpp"

#include "Engine/Graphics/PipelineCreateInfo.hpp"
#include "Engine/Graphics/Resources/TextureLoader.hpp"
#include "Engine/Graphics/Vulkan/Renderer.hpp"

#include <stdexcept>

namespace gfx = engine::graphics;
namespace vlk = gfx::vulkan;

TextureRegistry::TextureRegistry(vlk::Renderer& renderer) {
  const gfx::PipelineHandle pipeline =
      renderer.CreatePipeline(gfx::PipelineCreateInfo{.Kind = gfx::PipelineKind::SolidTexture,
                                                      .VertexShaderFile = "Shaders/terrain.vert.spv",
                                                      .FragmentShaderFile = "Shaders/terrain.frag.spv"});

  std::array<std::string_view, blockTextures_.size()> paths{};
  for (std::size_t i = 0; i < blockTextures_.size(); i++) {
    // Ensure the texture path matches the intended BlockTexture enum.
    assert(static_cast<std::uint32_t>(blockTextures_[i].TextureID) == i);
    paths[i] = blockTextures_[i].Path;
  }

  const gfx::resources::TextureLoader loader(renderer);
  auto texture = loader.LoadArray(paths);

  const auto material = renderer.CreateMaterial(texture);
  renderItem_ = {.Pipeline = pipeline, .Material = material};
}

const RenderItem& TextureRegistry::GetRenderItem() const noexcept {
  return renderItem_;
}

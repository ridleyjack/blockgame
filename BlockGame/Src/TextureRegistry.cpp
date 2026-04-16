#include "TextureRegistry.hpp"

#include "Engine/Graphics/PipelineCreateInfo.hpp"
#include "Engine/Graphics/Resources/TextureLoader.hpp"
#include "Engine/Graphics/Vulkan/Renderer.hpp"

#include <stdexcept>

namespace gfx = engine::graphics;
namespace vlk = gfx::vulkan;

TextureRegistry::TextureRegistry(vlk::Renderer& renderer) {

  const gfx::RenderPassHandle renderPass = renderer.CreateRenderPass();
  const gfx::PipelineHandle pipeline =
      renderer.CreatePipeline(gfx::PipelineCreateInfo{.RenderPass = renderPass,
                                                      .VertexShaderFile = "Shaders/vert.spv",
                                                      .FragmentShaderFile = "Shaders/frag.spv"});

  const gfx::resources::TextureLoader loader(renderer);
  std::array paths{std::string_view{"Textures/Tiles/dirt.png"},
                   std::string_view{"Textures/Tiles/dirt_grass.png"},
                   std::string_view{"Textures/Tiles/grass_top.png"},
                   std::string_view{"Textures/Tiles/dirt_sand.png"},
                   std::string_view{"Textures/Tiles/dirt_snow.png"},
                   std::string_view{"Textures/Tiles/sand.png"},
                   std::string_view{"Textures/Tiles/snow.png"},
                   std::string_view{"Textures/Tiles/ice.png"},
                   std::string_view{"Textures/Tiles/stone.png"},
                   std::string_view{"Textures/Tiles/stone_dirt.png"},
                   std::string_view{"Textures/Tiles/stone_grass.png"},
                   std::string_view{"Textures/Tiles/stone_snow.png"}};

  auto texture = loader.LoadArray(paths);
  if (!texture)
    throw std::runtime_error("Failed to load texture " + texture.error().Detail);

  const auto material = renderer.CreateMaterial(*texture);
  renderItem_ = {.RenderPass = renderPass, .Pipeline = pipeline, .Material = material};
}

const RenderItem& TextureRegistry::GetRenderItem() const noexcept {
  return renderItem_;
}

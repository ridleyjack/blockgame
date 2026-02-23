#include "GameLayer.hpp"

#include "Map.hpp"
#include "Engine/Application.hpp"
#include "Engine/Graphics/Vulkan/Renderer.hpp"
#include "Engine/Events/Events.hpp"
#include "Engine/Assets/ImageLoader.hpp"
#include "Engine/Graphics/CameraMatrices.hpp"
#include "Engine/Graphics/PipelineCreateInfo.hpp"
#include "Engine/Graphics/TextureArrayInfo.hpp"
#include "Engine/Graphics/TextureArrayBuilder.hpp"

#include <print>

namespace gfx = engine::graphics;
namespace vlk = gfx::vulkan;
namespace assets = engine::assets;

GameLayer::GameLayer(engine::Application& application) : application_(application), map_(10, 10, 10), mapMeshes_(map_) {

  auto& renderer = application_.GetRenderer();

  const gfx::RenderPassHandle renderPass = renderer.CreateRenderPass();
  const gfx::PipelineHandle pipeline =
      renderer.CreatePipeline(gfx::PipelineCreateInfo{.RenderPass = renderPass,
                                                      .VertexShaderFile = "Shaders/vert.spv",
                                                      .FragmentShaderFile = "Shaders/frag.spv"});

  constexpr gfx::TextureArrayInfo info{
      .Height = 128,
      .Width = 128,
      .NumLayers = 4,
      .LayerSizeBytes = static_cast<std::size_t>(128) * 128 * 4,
  };

  auto arrayBuilderResult = renderer.BeginTextureArray(info);
  if (!arrayBuilderResult) {
    const auto msg = std::format("Failed to build texture array: {}", vlk::ToString(arrayBuilderResult.error()));
    throw std::runtime_error(msg);
  }
  const gfx::TextureArrayBuilder& builder = *arrayBuilderResult;

  auto loader = assets::ImageLoader{};
  auto upload = [&](std::string_view path) {
    if (auto r = loader.Load(path); !r) {
      const auto msg = std::format("Failed to load texture {}: {}", path, r.error());
      throw std::runtime_error(msg);
    }
    builder.Upload(loader.Data());
  };

  std::println("Loading Textures..");
  upload("Textures/Tiles/dirt.png");
  upload("Textures/Tiles/stone.png");
  upload("Textures/Tiles/sand.png");
  upload("Textures/Tiles/snow.png");

  engine::graphics::TextureHandle texture{};
  if (const auto result = builder.Finalize(); !result) {
    const auto msg = std::format("Failed to finalize texture array: {}", vlk::ToString(arrayBuilderResult.error()));
    throw std::runtime_error(msg);
  } else {
    texture = *result;
  }
  const gfx::MaterialHandle material = renderer.CreateMaterial(texture);
  std::println("Done!");

  mapMeshes_.BuildAll();
}

void GameLayer::OnUpdate(const float deltaTime) {
  const float speed = 20.5f * deltaTime;
  if (input_.Movement.Forward)
    camera_.Move(camera_.Forward() * speed);
  if (input_.Movement.Backward)
    camera_.Move(camera_.Forward() * -speed);
  if (input_.Movement.Left)
    camera_.Move(camera_.Right() * -speed);
  if (input_.Movement.Right)
    camera_.Move(camera_.Right() * speed);

  if (input_.Exit)
    application_.Stop();

  auto& renderer = application_.GetRenderer();
  mapMeshes_.Update(renderer);
};

void GameLayer::OnRender() {
  auto& renderer = application_.GetRenderer();

  engine::graphics::CameraMatrices cameraMatrices{};
  cameraMatrices.View = camera_.View();
  cameraMatrices.Projection = renderer.MakeProjection();

  if (const auto r = renderer.BeginFrame({0}, {0}, cameraMatrices); !r) {
    if (r.error() != vlk::RenderError::FrameOutOfDate) {
      std::println("Failed to begin rendering frame", 1);
      return;
    }
  }

  for (int z = 0; z < map_.Depth(); z++)
    for (int y = 0; y < map_.Height(); y++)
      for (int x = 0; x < map_.Width(); x++) {
        const ChunkMesh& mesh{mapMeshes_.Mesh({x, y, z})};
        if (mesh.HasVertices())
          renderer.Submit(mesh.Mesh, {0});
      }

  if (const auto rv = renderer.EndFrame(); !rv) {
    std::println("Failed to render frame");
    return;
  }
};

void GameLayer::OnKeyReleased(const engine::events::KeyReleasedEvent& event) {
  handleKeyInput(event.Keycode, false);
};

void GameLayer::OnKeyPressed(const engine::events::KeyPressedEvent& event) {
  handleKeyInput(event.Keycode, true);
}

void GameLayer::OnMouseMoved(const engine::events::MouseMovedEvent& event) {
  const auto xPos = static_cast<float>(event.X);
  const auto yPos = static_cast<float>(event.Y);

  if (firstMouse_) {
    lastX_ = xPos;
    lastY_ = yPos;
    firstMouse_ = false;
  }

  const float deltaX = xPos - lastX_;
  const float deltaY = lastY_ - yPos;
  lastX_ = xPos;
  lastY_ = yPos;

  constexpr float sensitivity = 0.1f;
  camera_.Rotate(deltaX * sensitivity, deltaY * sensitivity);
};

void GameLayer::handleKeyInput(const int keycode, const bool state) {
  switch (keycode) {
  case GLFW_KEY_W:
    input_.Movement.Forward = state;
    break;
  case GLFW_KEY_S:
    input_.Movement.Backward = state;
    break;
  case GLFW_KEY_A:
    input_.Movement.Left = state;
    break;
  case GLFW_KEY_D:
    input_.Movement.Right = state;
    break;
  case GLFW_KEY_ESCAPE:
    input_.Exit = true;
    break;
  default:
    // Do nothing.
  }
}

#include "GameLayer.hpp"

#include "Engine/Application.hpp"
#include "Engine/Graphics/Mesh.hpp"
#include "Engine/Graphics/Vulkan/Renderer.hpp"
#include "Engine/Events/Events.hpp"
#include "Engine/Assets/ImageLoader.hpp"
#include "Engine/Graphics/CameraMatrices.h"
#include "Engine/Graphics/PipelineCreateInfo.hpp"
#include "Engine/Graphics/TextureArrayInfo.hpp"
#include "Engine/Graphics/TextureArrayBuilder.hpp"

#include <print>

namespace gfx = engine::graphics;
namespace vlk = gfx::vulkan;
namespace assets = engine::assets;

const std::vector<engine::graphics::Vertex> vertices = {
    // +Z (Front) - Red
    {{-0.5f, -0.5f, 0.5f},  {1, 0, 0}, {0, 0}},
    {{0.5f, -0.5f, 0.5f},   {1, 0, 0}, {1, 0}},
    {{0.5f, 0.5f, 0.5f},    {1, 0, 0}, {1, 1}},
    {{-0.5f, 0.5f, 0.5f},   {1, 0, 0}, {0, 1}},

    // -Z (Back) - Green
    {{0.5f, -0.5f, -0.5f},  {0, 1, 0}, {0, 0}},
    {{-0.5f, -0.5f, -0.5f}, {0, 1, 0}, {1, 0}},
    {{-0.5f, 0.5f, -0.5f},  {0, 1, 0}, {1, 1}},
    {{0.5f, 0.5f, -0.5f},   {0, 1, 0}, {0, 1}},

    // +X (Right) - Blue
    {{0.5f, -0.5f, 0.5f},   {0, 0, 1}, {0, 0}},
    {{0.5f, -0.5f, -0.5f},  {0, 0, 1}, {1, 0}},
    {{0.5f, 0.5f, -0.5f},   {0, 0, 1}, {1, 1}},
    {{0.5f, 0.5f, 0.5f},    {0, 0, 1}, {0, 1}},

    // -X (Left) - Yellow
    {{-0.5f, -0.5f, -0.5f}, {1, 1, 0}, {0, 0}},
    {{-0.5f, -0.5f, 0.5f},  {1, 1, 0}, {1, 0}},
    {{-0.5f, 0.5f, 0.5f},   {1, 1, 0}, {1, 1}},
    {{-0.5f, 0.5f, -0.5f},  {1, 1, 0}, {0, 1}},

    // +Y (Top) - Magenta
    {{-0.5f, 0.5f, 0.5f},   {1, 0, 1}, {0, 0}},
    {{0.5f, 0.5f, 0.5f},    {1, 0, 1}, {1, 0}},
    {{0.5f, 0.5f, -0.5f},   {1, 0, 1}, {1, 1}},
    {{-0.5f, 0.5f, -0.5f},  {1, 0, 1}, {0, 1}},

    // -Y (Bottom) - Cyan
    {{-0.5f, -0.5f, -0.5f}, {0, 1, 1}, {0, 0}},
    {{0.5f, -0.5f, -0.5f},  {0, 1, 1}, {1, 0}},
    {{0.5f, -0.5f, 0.5f},   {0, 1, 1}, {1, 1}},
    {{-0.5f, -0.5f, 0.5f},  {0, 1, 1}, {0, 1}},
};

// clang-format off
const std::vector<std::uint32_t> indices = {
  0, 1, 2, 2, 3, 0,       // Front
  4, 5, 6, 6, 7, 4,       // Back
  8, 9,10,10,11, 8,       // Right
 12,13,14,14,15,12,       // Left
 16,17,18,18,19,16,       // Top
 20,21,22,22,23,20        // Bottom
};
// clang-format on

GameLayer::GameLayer(engine::Application& application) : application_(application) {

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
  gfx::TextureArrayBuilder& builder = *arrayBuilderResult;

  auto loader = assets::ImageLoader{};
  auto upload = [&](std::string_view path) {
    if (auto r = loader.Load(path); !r) {
      const auto msg = std::format("Failed to load texture {}: {}", path, r.error());
      throw std::runtime_error(msg);
    }
    builder.Upload(loader.Data());
  };

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

  myMesh_.Upload(application.GetRenderer());
}

void GameLayer::OnUpdate(const float deltaTime) {
  const float speed = 2.5f * deltaTime;
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

  renderer.Submit(myMesh_.Mesh(), {0});

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

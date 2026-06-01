#include "GameLayer.hpp"

#include "Engine/Application.hpp"
#include "Engine/Graphics/Vulkan/Renderer.hpp"
#include "Engine/Events/Events.hpp"
#include "Engine/Assets/ImageLoader.hpp"
#include "Engine/Graphics/CameraMatrices.hpp"

#include <GLFW/glfw3.h>

#include <print>
#include <cmath>

namespace gfx = engine::graphics;
namespace vlk = gfx::vulkan;
namespace assets = engine::assets;

GameLayer::GameLayer(engine::Application& application)
    : application_(application),
      textures_(application.GetRenderer()),
      camera_({16 * 1.5f, 16 * 3.0f, 16 * 1.5f}),
      world_(application.GetRenderer()) {
  world_.Update({1, 1, 1});
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

  const int playerX = static_cast<int>(std::floor(camera_.Position().x / Chunk::ChunkWidth));
  const int playerZ = static_cast<int>(std::floor(camera_.Position().z / Chunk::ChunkDepth));

  world_.Update({playerX, 1, playerZ});
}

void GameLayer::OnRender() {
  auto& renderer = application_.GetRenderer();
  const auto& renderItem = textures_.GetRenderItem();

  float drawDistance = static_cast<float>(ChunkStreamer::LoadRadius * (Chunk::ChunkWidth + 1));
  engine::graphics::CameraMatrices cameraMatrices{.Projection =
                                                      renderer.MakeProjection(vlk::Renderer::ProjectionSettings{
                                                          .FarPlane = drawDistance,
                                                      }),
                                                  .View = camera_.View()};

  if (const auto r = renderer.BeginFrame(cameraMatrices); !r) {
    if (r.error() != vlk::RenderError::FrameOutOfDate) {
      std::println("Failed to begin rendering frame", 1);
      return;
    }
  }

  for (const auto& meshCoords : world_.LoadedChunks()) {
    const ChunkMesh& mesh{world_.Mesh(meshCoords)};
    if (mesh.HasVertices())
      renderer.Submit(renderItem.Pipeline, mesh.Mesh, renderItem.Material);
  }

  if (const auto rv = renderer.EndFrame(); !rv) {
    std::println("Failed to render frame");
    return;
  }
}

void GameLayer::OnKeyReleased(const engine::events::KeyReleasedEvent& event) {
  handleKeyInput(event.Keycode, false);
}

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
}

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
  case GLFW_KEY_P: {
    if (!state)
      return;
    auto pos = camera_.Position();
    auto dir = camera_.Forward();
    auto result = world_.RaycastBlock(pos, dir, 10);
    if (!result) {
      std::println("Miss");
    } else {
      auto pos2 = result->Position;
      std::println("Hit:{} at (x:{},y:{},z:{})", static_cast<int>(result->BlockType), pos2.X, pos2.Y, pos2.Z);
    }
    break;
  }
  case GLFW_KEY_ESCAPE:
    input_.Exit = true;
    break;
  default:
    break;
  }
}

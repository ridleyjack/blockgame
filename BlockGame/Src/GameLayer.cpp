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
      world_(application.GetRenderer()),
      blockHighlighter_(application.GetRenderer()) {
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

  constexpr float maxDrawDistanceInBlocks = 10;
  const auto result = world_.RaycastBlock(camera_.Position(), camera_.Forward(), maxDrawDistanceInBlocks);
  if (result) {
    blockHighlighter_.SetPosition(result->Position);
    hoveredBlock_ = result->Position;
  } else
    hoveredBlock_ = std::nullopt;
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
    std::println("Failed to begin rendering frame");
    return;
  }

  for (const auto& meshCoords : world_.LoadedChunks()) {
    const auto meshHandle = world_.Mesh(meshCoords);
    if (meshHandle)
      renderer.Submit(renderItem.Pipeline, *meshHandle, renderItem.Material, renderItem.PushConstants);
  }

  if (hoveredBlock_) {
    auto& highlightItem = blockHighlighter_.GetRenderItem();
    renderer.Submit(highlightItem.Pipeline,
                    blockHighlighter_.GetMesh(),
                    highlightItem.Material,
                    highlightItem.PushConstants);
  }

  renderer.EndFrame();
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

void GameLayer::OnMouseButtonPressed(const engine::events::MouseButtonPressedEvent& event) {
  if (event.Button == GLFW_MOUSE_BUTTON_LEFT && hoveredBlock_) {
    world_.SetBlock(*hoveredBlock_, BlockType::Air);
  }
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
  case GLFW_KEY_ESCAPE:
    input_.Exit = true;
    break;
  default:
    break;
  }
}

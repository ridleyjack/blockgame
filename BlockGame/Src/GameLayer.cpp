#include "GameLayer.hpp"

#include "Engine/Application.hpp"
#include "Engine/Graphics/Mesh.hpp"
#include "Engine/Graphics/Vulkan/Renderer.hpp"
#include "Engine/Events/Events.hpp"
#include "Engine/Assets/ImageLoader.hpp"
#include "Engine/Graphics/Camera.h"

#include <print>

namespace gfx = engine::graphics::vulkan;
namespace assets = engine::assets;

const std::vector<engine::graphics::Vertex> vertices = {
    {{-0.5f, -0.5f, 0.0f},  {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
    {{0.5f, -0.5f, 0.0f},   {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
    {{0.5f, 0.5f, 0.0f},    {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
    {{-0.5f, 0.5f, 0.0f},   {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},

    {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
    {{0.5f, -0.5f, -0.5f},  {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
    {{0.5f, 0.5f, -0.5f},   {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
    {{-0.5f, 0.5f, -0.5f},  {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},

    // Triangle to the right of the quad
    {{0.75f, -0.5f, 0.0f},  {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}}, // left  -> GREEN
    {{1.25f, -0.5f, 0.0f},  {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}}, // right -> RED
    {{1.0f, 0.5f, 0.0f},    {0.0f, 0.0f, 1.0f}, {0.5f, 1.0f}}, // top   -> BLUE
};

// clang-format off
const std::vector<uint16_t> indices = {
  0, 1, 2, 2, 3, 0,
  4, 5, 6, 6, 7, 4,

  8, 9, 10   // new triangle
};
// clang-format on

GameLayer::GameLayer(engine::Application& application) : application_(application) {
  myMesh_ = application.GetRenderer().CreateMesh({.Vertices = vertices, .Indices = indices});
  auto loader = assets::ImageLoader{};
  if (auto result = loader.Load("Textures/texture.jpg"); !result) {
    std::println("Failed to load texture: {}", result.error());
  }
  // application.GetRenderer().CreateTexture(loader.Data(), loader.Width(), loader.Height());
}

void GameLayer::OnUpdate(float deltaTime) {
  const float speed = 2.5f * deltaTime;
  if (movement_.Forward)
    camera_.Move(camera_.Forward() * speed);
  if (movement_.Backward)
    camera_.Move(camera_.Forward() * -speed);
  if (movement_.Left)
    camera_.Move(camera_.Right() * -speed);
  if (movement_.Right)
    camera_.Move(camera_.Right() * speed);
};

void GameLayer::OnRender() {
  auto& renderer = application_.GetRenderer();

  if (const auto r = renderer.BeginFrame({0}, {0}, camera_); !r) {
    if (r.error() != gfx::RenderError::FrameOutOfDate) {
      std::println("Failed to begin rendering frame", 1);
      return;
    }
  }

  renderer.Submit(myMesh_);

  if (const auto rv = renderer.EndFrame(); !rv) {
    std::print("Failed to render frame");
    return;
  }
};

void GameLayer::OnKeyReleased(const engine::events::KeyReleasedEvent& event) {
  handleKeyInput(event.Keycode, false);
};

void GameLayer::OnKeyPressed(const engine::events::KeyPressedEvent& event) {
  handleKeyInput(event.Keycode, true);
};

void GameLayer::handleKeyInput(const int keycode, const bool state) {
  switch (keycode) {
  case GLFW_KEY_W:
    movement_.Forward = state;
    break;
  case GLFW_KEY_S:
    movement_.Backward = state;
    break;
  case GLFW_KEY_A:
    movement_.Left = state;
    break;
  case GLFW_KEY_D:
    movement_.Right = state;
    break;
  default:
    // Do nothing.
  }
}

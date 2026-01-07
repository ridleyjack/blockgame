#include "GameLayer.hpp"

#include "Engine/Application.hpp"
#include "Engine/Graphics/Mesh.hpp"
#include "Engine/Graphics/Vulkan/Renderer.hpp"
#include "Engine/Events/Events.hpp"
#include "Engine/Assets/ImageLoader.hpp"

#include <print>

namespace gfx = engine::graphics::vulkan;
namespace assets = engine::assets;

const std::vector<engine::graphics::Vertex> vertices = {
    {{-0.5f, -0.5f, 0.0f},  {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
    {{0.5f, -0.5f, 0.0f},   {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
    {{0.5f, 0.5f, 0.0f},    {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
    {{-0.5f, 0.5f, 0.0f},   {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},

    {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
    {{0.5f, -0.5f, -0.5f},  {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
    {{0.5f, 0.5f, -0.5f},   {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
    {{-0.5f, 0.5f, -0.5f},  {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}}
};

// clang-format off
const std::vector<uint16_t> indices = {
  0, 1, 2, 2, 3, 0,
  4, 5, 6, 6, 7, 4
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

void GameLayer::OnUpdate(float deltaTime) {};

void GameLayer::OnRender() {
  auto& renderer = application_.GetRenderer();

  if (const auto r = renderer.BeginFrame({0}, {0}); !r) {
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
  std::println("Key released w/ code {0}", event.Keycode);
};

void GameLayer::OnKeyPressed(const engine::events::KeyPressedEvent& event) {
  std::println("Key pressed w/ code {0}", event.Keycode);
};

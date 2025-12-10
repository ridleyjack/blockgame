#include "GameLayer.hpp"

#include "Engine/Application.hpp"
#include "Engine/Graphics/Mesh.hpp"
#include "Engine/Graphics/Vulkan/Renderer.hpp"
#include "../../Engine/Src/Engine/Events/Events.hpp"

#include <print>

namespace gfx = engine::graphics::vulkan;

const std::vector<engine::graphics::Vertex> vertices = {
    {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}}, // 0
    {{0.5f, -0.5f, -0.5f},  {0.0f, 1.0f, 0.0f}}, // 1
    {{0.5f, 0.5f, -0.5f},   {0.0f, 0.0f, 1.0f}}, // 2
    {{-0.5f, 0.5f, -0.5f},  {1.0f, 1.0f, 0.0f}}, // 3

    {{-0.5f, -0.5f, 0.5f},  {1.0f, 0.0f, 1.0f}}, // 4
    {{0.5f, -0.5f, 0.5f},   {0.0f, 1.0f, 1.0f}}, // 5
    {{0.5f, 0.5f, 0.5f},    {1.0f, 0.5f, 0.0f}}, // 6
    {{-0.5f, 0.5f, 0.5f},   {0.5f, 0.0f, 1.0f}}, // 7
};

const std::vector<uint16_t> indices = {
    {
     4, 5, 6, 4, 6, 7, 1, 0, 3, 1, 3, 2, 0, 4, 7, 0, 7, 3, 5, 1, 2, 5, 2, 6, 3, 7, 6, 3, 6, 2, 0, 1, 5, 0, 5, 4,
     }
};

GameLayer::GameLayer(engine::Application& application) : application_(application) {
  myMesh_ = application.GetRenderer().CreateMesh({.Vertices = vertices, .Indices = indices});
}

void GameLayer::OnUpdate(float deltaTime) {};

void GameLayer::OnRender() {
  auto& renderer = application_.GetRenderer();

  const auto ctx = renderer.BeginFrame();
  if (!ctx && ctx.error() != gfx::RenderError::FrameOutOfDate) {
    std::println("Failed to begin rendering frame", 1);
    return;
  }

  renderer.Submit(myMesh_);

  if (const auto rv = renderer.EndFrame(*ctx); !rv) {
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

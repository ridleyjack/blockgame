#pragma once

#include "Camera.hpp"
#include "BlockHighlighter.hpp"
#include "Crosshair.hpp"
#include "TextureRegistry.hpp"

#include "Engine/Application.hpp"
#include "World/World.hpp"
#include "Engine/ILayer.hpp"
#include "Engine/Events/IEventHandler.hpp"

#include <optional>

namespace engine {
class Application;

namespace graphics {
struct Vertex;

namespace vulkan {
class Renderer;
}
} // namespace graphics

namespace events {
struct KeyPressedEvent;
struct KeyReleasedEvent;
} // namespace events
} // namespace engine

namespace gfx = engine::graphics;
namespace vlk = gfx::vulkan;

namespace evt = engine::events;

struct Movements {
  bool Forward{};
  bool Backward{};
  bool Left{};
  bool Right{};
};

struct KeyInput {
  Movements Movement{};

  bool Exit{};
};

class GameLayer : public engine::ILayer,
                  public evt::IKeyEventHandler,
                  public evt::IMouseEventHandler,
                  public evt::IWindowEventHandler {
public:
  explicit GameLayer(engine::Application& application, vlk::Renderer& renderer);

  void OnUpdate(float deltaTime) override;

  void OnRender() override;

  void OnKeyReleased(const engine::events::KeyReleasedEvent& event) override;
  void OnKeyPressed(const engine::events::KeyPressedEvent& event) override;

  void OnMouseMoved(const engine::events::MouseMovedEvent& event) override;
  void OnMouseButtonPressed(const engine::events::MouseButtonPressedEvent& event) override;

  void OnFramebufferResized(const engine::events::FramebufferResizedEvent& event) override;

private:
  static constexpr gfx::Color SkyColor{0.55f, 0.72f, 0.90f};

  engine::Application& application_;
  vlk::Renderer& renderer_;

  TextureRegistry textures_;

  Camera camera_{};
  KeyInput input_{};

  World world_;

  BlockHighlighter blockHighlighter_;
  Crosshair crosshair_;
  std::optional<math::Vec3Int> hoveredBlock_{};

  bool firstMouse_{true};
  float lastX_{}, lastY_{};

  void handleKeyInput(int keycode, bool state);
};

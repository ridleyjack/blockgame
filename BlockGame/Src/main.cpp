#include "GameLayer.hpp"

#include "Engine/Application.hpp"
#include "Engine/Graphics/Vulkan/Renderer.hpp"

namespace eng = engine;
namespace gfx = eng::graphics;
namespace vlk = gfx::vulkan;

int main() {

  const engine::ApplicationConfig config{};
  eng::Application app{config};
  vlk::Renderer renderer{app.GetWindow().GetHandle()};

  GameLayer layer{app, renderer};
  app.PushLayer(&layer);
  app.Run();

  return 0;
}

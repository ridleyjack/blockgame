#include "GameLayer.hpp"

#include "Engine/Application.hpp"
#include "Engine/GFX/Vulkan/VulkanRenderer.hpp"

int main() {

  engine::ApplicationConfig config{};
  engine::Application app{config};
  engine::gfx::vulkan::Renderer renderer{app.GetWindow().GetHandle()};
  renderer.Init();

  GameLayer layer{renderer};
  app.PushLayer(&layer);
  app.Run();

  return 0;
}

#include "GameLayer.hpp"

#include "Engine/Application.hpp"
#include "Engine/Renderer/VulkanRenderer.hpp"

int main() {

  Engine::ApplicationConfig config{};
  Engine::Application app{config};
  Engine::Renderer renderer{app.GetWindow().GetHandle()};
  renderer.Init();

  GameLayer layer{renderer};
  app.PushLayer(&layer);
  app.Run();

  return 0;
}

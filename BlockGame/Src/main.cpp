#include "GameLayer.hpp"

#include "Engine/Application.hpp"
#include "Engine/GFX/Vulkan/VulkanRenderer.hpp"

int main() {

  engine::ApplicationConfig config{};
  engine::Application app{config};

  GameLayer layer{app};
  app.PushLayer(&layer);
  app.Run();

  return 0;
}

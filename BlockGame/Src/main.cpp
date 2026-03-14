#include "GameLayer.hpp"

#include "Engine/Application.hpp"

int main() {

  const engine::ApplicationConfig config{};
  engine::Application app{config};

  GameLayer layer{app};
  app.PushLayer(&layer);
  app.Run();

  return 0;
}

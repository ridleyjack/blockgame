#pragma once

#include "ILayer.hpp"
#include "Window.hpp"

#include <memory>
#include <vector>

namespace engine {

namespace gfx::vulkan {
class Renderer;
}

struct ApplicationConfig {
  std::string Name{"Block Game"};
  WindowConfig Window{};
};

class Application {
public:
  explicit Application(const ApplicationConfig& config);
  ~Application();

  void Run();
  void Stop();

  void PushLayer(ILayer* layer);
  void RemoveLayer(const ILayer* layer);

  static float GetTime();

  Window& GetWindow() const;
  gfx::vulkan::Renderer& GetRenderer() const;

private:
  ApplicationConfig config_{};
  std::unique_ptr<Window> window_{};
  std::unique_ptr<gfx::vulkan::Renderer> renderer_{};

  std::vector<ILayer*> layerStack_{};

  bool running_{false};
};

} // namespace engine
#pragma once

#include "ILayer.hpp"
#include "Window.hpp"

#include <memory>
#include <vector>

namespace Engine {

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
  void RemoveLayer(ILayer* layer);

  static float GetTime();

  Window& GetWindow() const;

private:
  ApplicationConfig config_{};
  std::unique_ptr<Window> window_{};

  std::vector<ILayer*> layerStack_{};

  bool running_{false};
};

} // namespace Engine
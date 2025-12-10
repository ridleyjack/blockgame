#pragma once

#include "Window.hpp"
#include "Events/Events.hpp"
#include "Events/IEventRaiser.hpp"

#include <memory>
#include <vector>

namespace engine {

namespace graphics::vulkan {
class Renderer;
} // namespace graphics::vulkan

class ILayer;
class Window;

struct ApplicationConfig {
  std::string Name{"Block Game"};
  WindowConfig Window{};
};

class Application : public events::IEventRaiser {
public:
  explicit Application(const ApplicationConfig& config);
  ~Application() override;

  void Run();
  void Stop();

  void PushLayer(ILayer* layer);
  void RemoveLayer(const ILayer* layer);

  static float GetTime();

  Window& GetWindow() const;
  graphics::vulkan::Renderer& GetRenderer() const;

  void RaiseEvent(const events::Event& event) override;

private:
  ApplicationConfig config_{};
  std::unique_ptr<Window> window_{};
  std::unique_ptr<graphics::vulkan::Renderer> renderer_{};

  std::vector<ILayer*> layerStack_{};

  bool running_{false};
};

} // namespace engine
#pragma once

#include "Window.hpp"
#include "Events/Events.hpp"
#include "Events/IEventRaiser.hpp"

#include <memory>
#include <vector>

namespace engine {

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

  void RaiseEvent(const events::Event& event) override;

private:
  ApplicationConfig config_{};
  std::unique_ptr<Window> window_{};
  std::vector<ILayer*> layerStack_{};

  bool running_{false};
};

} // namespace engine
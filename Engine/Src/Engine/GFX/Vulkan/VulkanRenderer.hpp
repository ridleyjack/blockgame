#pragma once

#include <memory>

struct GLFWwindow;

namespace Engine {

class Context;
class TileGroup;

class Renderer {
public:
  explicit Renderer(GLFWwindow* window);
  ~Renderer();

  Renderer(const Renderer&) = delete;
  Renderer& operator=(const Renderer&) = delete;

  void Init();
  void DrawFrame();

  bool ShouldClose() const;
  void WaitUntilIdle() const;

private:
  uint32_t currentFrame_{};

  std::unique_ptr<Context> context_{};
  std::unique_ptr<TileGroup> tileGroup_{};
};

} // namespace Engine
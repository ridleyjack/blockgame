#pragma once
#include <string>

namespace engine::Assets {

class ImageLoader {
public:
  ImageLoader() = default;
  ~ImageLoader();

  void Load(const std::string& path);

  const unsigned char* Data() const;
  int Width() const;
  int Height() const;
  int Channels() const;

private:
  unsigned char* pixels_{nullptr};
  int width_{}, height_{}, channels_{};
};
} // namespace Engine::Assets
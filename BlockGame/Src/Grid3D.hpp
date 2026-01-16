#pragma once

#include <vector>

template <typename T> class Grid3D {
public:
  Grid3D(const std::size_t depth, const std::size_t height, const std::size_t width, const T& value)
      : depth_(depth), height_(height), width_(width), data_(depth_ * height_ * width_, value) {}

  T& operator[](const std::size_t z, const std::size_t y, const std::size_t x) noexcept {
    assert(z < depth_ && y < height_ && x < width_);
    return data_[index_(z, y, x)];
  }

  const T& operator[](const std::size_t z, const std::size_t y, const std::size_t x) const noexcept {
    assert(z < depth_ && y < height_ && x < width_);
    return data_[index_(z, y, x)];
  }

  std::size_t Depth() const noexcept {
    return depth_;
  }
  std::size_t Height() const noexcept {
    return height_;
  }
  std::size_t Width() const noexcept {
    return width_;
  }

private:
  std::size_t depth_{};
  std::size_t height_{};
  std::size_t width_{};

  std::vector<T> data_{};

  inline std::size_t index_(const std::size_t z, const std::size_t y, const std::size_t x) const noexcept {
    return z * height_ * width_ + y * width_ + x;
  }
};

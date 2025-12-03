#pragma once
#include <cstdint>
#include <optional>
#include <vector>
#include <cassert>

namespace engine::core::containers {
template <typename T> class HandlePool {
public:
  HandlePool() = default;

  explicit HandlePool(uint32_t reservedSize) {
    pool_.reserve(reservedSize);
  }

  HandlePool(const HandlePool&) = delete;
  HandlePool& operator=(const HandlePool&) = delete;

  void Reserve(uint32_t size) {
    pool_.reserve(size);
  }

  template <typename... Args> uint32_t Create(Args&&... args) {
    if (freeIndices_.empty()) {
      pool_.emplace_back(std::in_place, std::forward<Args>(args)...);
      return pool_.size() - 1;
    }

    uint32_t index = freeIndices_.back();
    freeIndices_.pop_back();
    pool_[index].emplace(std::forward<Args>(args)...);
    return index;
  }

  void Delete(uint32_t index) noexcept {
    assert(index < pool_.size());
    if (!pool_[index].has_value()) {
      return;
    }
    pool_[index].reset();
    freeIndices_.push_back(index);
  }

  T& Get(uint32_t index) noexcept {
    assert(index < pool_.size());
    return *pool_[index];
  }

  const T& Get(uint32_t index) const noexcept {
    assert(index < pool_.size());
    return *pool_[index];
  }

  void Clear() noexcept {
    pool_.clear();
    freeIndices_.clear();
  }

private:
  std::vector<std::optional<T>> pool_;
  std::vector<uint32_t> freeIndices_;
};
} // namespace engine::core::containers

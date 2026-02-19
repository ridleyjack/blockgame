#pragma once
#include <condition_variable>
#include <mutex>
#include <optional>
#include <queue>

template <typename T> class ThreadSafeQueue {
public:
  void Push(T value) {
    {
      std::lock_guard lock(mutex_);
      queue_.push(std::move(value));
    }
    cv_.notify_one();
  }

  std::optional<T> TryPop() {
    std::lock_guard lock(mutex_);

    if (queue_.empty())
      return {};

    T value = std::move(queue_.front());
    queue_.pop();
    return value;
  }

  std::optional<T> WaitPop(std::atomic<bool>& stopFlag) {
    std::unique_lock lock(mutex_);
    cv_.wait(lock, [&] { return stopFlag.load() || !queue_.empty(); });

    if (stopFlag && queue_.empty())
      return {};

    T value = std::move(queue_.front());
    queue_.pop();
    return value;
  }

  void NotifyAll() {
    cv_.notify_all();
  }

private:
  std::queue<T> queue_{};
  std::mutex mutex_{};
  std::condition_variable cv_{};
};
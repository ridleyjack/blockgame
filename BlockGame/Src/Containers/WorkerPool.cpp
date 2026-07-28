#include "WorkerPool.hpp"

#include <cstdint>
#include <thread>

WorkerPool::WorkerPool() {
  std::uint32_t workerCount = std::thread::hardware_concurrency();
  workerCount = workerCount < 2 ? 1 : workerCount - 1;

  workers_.reserve(workerCount);
  for (std::uint32_t i = 0; i < workerCount; i++) {
    workers_.emplace_back([this] {
      while (true) {
        std::function<void()> task;
        {
          std::unique_lock lock(mutex_);
          cv_.wait(lock, [this] { return stopping_ || !tasks_.empty(); });

          if (stopping_ && tasks_.empty())
            return;

          task = std::move(tasks_.front());
          tasks_.pop();
        }

        task();
      }
    });
  }
}

WorkerPool::~WorkerPool() {
  Cancel();
}

bool WorkerPool::Enqueue(std::function<void()> task) {
  {
    std::lock_guard lock(mutex_);
    if (stopping_)
      return false;

    tasks_.push(std::move(task));
  }
  cv_.notify_one();
  return true;
}

void WorkerPool::Drain() {
  {
    std::lock_guard lock(mutex_);
    stopping_ = true;
  }
  cv_.notify_all();

  for (auto& worker : workers_) {
    if (worker.joinable())
      worker.join();
  }
}

void WorkerPool::Cancel() {
  {
    std::lock_guard lock(mutex_);
    stopping_ = true;
    tasks_ = std::queue<std::function<void()>>{};
  }
  cv_.notify_all();

  for (auto& worker : workers_) {
    if (worker.joinable())
      worker.join();
  }
}

#pragma once

#include <print>
#include <iostream>
#include <string_view>

[[noreturn]] inline void Fatal(const std::string_view message) noexcept {
  try {
    std::println(stderr, "Fatal error: {}", message);
  } catch (...) {
    std::fputs("Critical: Fatal error occurred, and logging failed.\n", stderr);
  }
  std::fflush(stderr);
  std::abort();
}
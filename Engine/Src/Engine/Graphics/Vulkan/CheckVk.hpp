#pragma once

#include "Engine/Fatal.hpp"

#include <vulkan/vulkan.h>

#include <format>
#include <string_view>

inline void CheckVk(const VkResult result, const std::string_view label) noexcept {
  if (result == VK_SUCCESS)
    return;

  try {
    const std::string message = std::format("{} failed with VkResult {}", label, static_cast<int>(result));
    Fatal(message);
  } catch (...) {
    std::fputs("Fatal Vulkan error occurred, but string formatting failed.\n", stderr);
    std::abort();
  }
}

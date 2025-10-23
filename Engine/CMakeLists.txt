# Engine/CMakeLists.txt
add_library(Engine
        vendor/stb/stb_image.cpp
        vendor/stb/stb_image.h
        Src/Engine/Application.cpp
        Src/Engine/Window.cpp
        Src/Engine/Renderer/TileGroup.cpp
        Src/Engine/Renderer/TileGroup.hpp
        Src/Engine/Renderer/VulkanCommand.cpp
        Src/Engine/Renderer/VulkanCommand.hpp
        Src/Engine/Renderer/VulkanContext.cpp
        Src/Engine/Renderer/VulkanContext.hpp
        Src/Engine/Renderer/VulkanDevice.cpp
        Src/Engine/Renderer/VulkanDevice.hpp
        Src/Engine/Renderer/VulkanPipeline.cpp
        Src/Engine/Renderer/VulkanPipeline.hpp
        Src/Engine/Renderer/VulkanRenderer.cpp
        Src/Engine/Renderer/VulkanRenderer.hpp
        Src/Engine/Renderer/VulkanSwapChain.cpp
        Src/Engine/Renderer/VulkanSwapChain.hpp
        Src/Engine/Renderer/VulkanSync.cpp
        Src/Engine/Renderer/VulkanSync.hpp
        Src/Engine/Assets/File.cpp
        Src/Engine/Assets/File.hpp
        Src/Engine/Assets/ImageLoader.cpp
        Src/Engine/Assets/ImageLoader.hpp
        Src/Engine/Renderer/VulkanPipelineLibrary.cpp
        Src/Engine/Renderer/VulkanPipelineLibrary.hpp
)

target_compile_features(Engine PUBLIC cxx_std_23)

find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)

target_link_libraries(Engine
        PUBLIC Vulkan::Vulkan
        PRIVATE glfw
)

target_include_directories(Engine
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/Src
        "vendor/stb"
)


# Engine/CMakeLists.txt

find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)

add_library(Engine
        vendor/stb/stb_image.cpp
        vendor/stb/stb_image.h
        Src/Engine/Application.cpp
        Src/Engine/Window.cpp
        Src/Engine/Assets/File.cpp
        Src/Engine/Assets/File.hpp
        Src/Engine/Assets/ImageLoader.cpp
        Src/Engine/Assets/ImageLoader.hpp
)
target_include_directories(Engine
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/Src
        ${CMAKE_CURRENT_SOURCE_DIR}/vendor/stb
)

target_link_libraries(Engine
        PUBLIC Vulkan::Vulkan
        PRIVATE glfw
)


target_compile_features(Engine PUBLIC cxx_std_23)


add_library(Engine_GFX
        Src/Engine/GFX/Vulkan/VulkanCommand.cpp
        Src/Engine/GFX/Vulkan/VulkanCommand.hpp
        Src/Engine/GFX/Vulkan/VulkanContext.cpp
        Src/Engine/GFX/Vulkan/VulkanContext.hpp
        Src/Engine/GFX/Vulkan/VulkanDevice.cpp
        Src/Engine/GFX/Vulkan/VulkanDevice.hpp
        Src/Engine/GFX/Vulkan/VulkanPipeline.cpp
        Src/Engine/GFX/Vulkan/VulkanPipeline.hpp
        Src/Engine/GFX/Vulkan/VulkanRenderer.cpp
        Src/Engine/GFX/Vulkan/VulkanRenderer.hpp
        Src/Engine/GFX/Vulkan/VulkanSwapChain.cpp
        Src/Engine/GFX/Vulkan/VulkanSwapChain.hpp
        Src/Engine/GFX/Vulkan/VulkanSync.cpp
        Src/Engine/GFX/Vulkan/VulkanSync.hpp
        Src/Engine/GFX/Vulkan/VulkanPipelineCache.cpp
        Src/Engine/GFX/Vulkan/VulkanPipelineCache.hpp
        Src/Engine/GFX/Mesh.hpp
        Src/Engine/GFX/RenderObject.cpp
        Src/Engine/GFX/RenderObject.hpp
)

target_include_directories(Engine_GFX
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/Src
)

target_link_libraries(Engine_GFX
        PUBLIC
        Engine
        Vulkan::Vulkan
        PRIVATE
        glfw
)

target_compile_features(Engine_GFX PUBLIC cxx_std_23)



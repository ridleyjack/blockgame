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
        Src/Engine/Core/Containers/HandlePool.hpp
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


add_library(Engine_Graphics
        Src/Engine/Graphics/Vulkan/Command.cpp
        Src/Engine/Graphics/Vulkan/Command.hpp
        Src/Engine/Graphics/Vulkan/Context.cpp
        Src/Engine/Graphics/Vulkan/Context.hpp
        Src/Engine/Graphics/Vulkan/Device.cpp
        Src/Engine/Graphics/Vulkan/Device.hpp
        Src/Engine/Graphics/Vulkan/Pipeline.cpp
        Src/Engine/Graphics/Vulkan/Pipeline.hpp
        Src/Engine/Graphics/Vulkan/Renderer.cpp
        Src/Engine/Graphics/Vulkan/Renderer.hpp
        Src/Engine/Graphics/Vulkan/SwapChain.cpp
        Src/Engine/Graphics/Vulkan/SwapChain.hpp
        Src/Engine/Graphics/Vulkan/Sync.cpp
        Src/Engine/Graphics/Vulkan/Sync.hpp
        Src/Engine/Graphics/Vulkan/PipelineCache.cpp
        Src/Engine/Graphics/Vulkan/PipelineCache.hpp
        Src/Engine/Graphics/Mesh.hpp
        Src/Engine/Graphics/RenderObject.cpp
        Src/Engine/Graphics/RenderObject.hpp
        Src/Engine/Graphics/Handles.hpp
        Src/Engine/Graphics/PipelineCreateInfo.hpp
        Src/Engine/Graphics/MeshCreateInfo.hpp
        Src/Engine/Graphics/MeshCreateInfo.hpp
        Src/Engine/Graphics/MeshCreateInfo.hpp
        Src/Engine/Graphics/Vulkan/RenderPass.cpp
        Src/Engine/Graphics/Vulkan/RenderPass.hpp
        Src/Engine/Graphics/Vulkan/FramebufferSet.cpp
        Src/Engine/Graphics/Vulkan/FramebufferSet.hpp
)

target_include_directories(Engine_Graphics
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/Src
)

target_link_libraries(Engine_Graphics
        PUBLIC
        Engine
        Vulkan::Vulkan
        PRIVATE
        glfw
)

target_compile_features(Engine_Graphics PUBLIC cxx_std_23)



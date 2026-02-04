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
        Src/Engine/Containers/HandlePool.hpp
        Src/Engine/Events/IEventHandler.hpp
        Src/Engine/Events/IEventRaiser.hpp
        Src/Engine/Events/Events.hpp
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
        Src/Engine/Graphics/Handles.hpp
        Src/Engine/Graphics/PipelineCreateInfo.hpp
        Src/Engine/Graphics/Mesh.hpp
        Src/Engine/Graphics/Vulkan/RenderPassCache.cpp
        Src/Engine/Graphics/Vulkan/RenderPassCache.hpp
        Src/Engine/Graphics/Vulkan/FramebufferSet.cpp
        Src/Engine/Graphics/Vulkan/FramebufferSet.hpp
        Src/Engine/Graphics/Vulkan/MeshGPU.hpp
        Src/Engine/Graphics/Vulkan/UniformBuffer.hpp
        Src/Engine/Graphics/Vulkan/DescriptorAllocator.cpp
        Src/Engine/Graphics/Vulkan/DescriptorAllocator.hpp
        Src/Engine/Graphics/Vulkan/Config.hpp
        Src/Engine/Graphics/Vulkan/MeshGPU.cpp
        Src/Engine/Graphics/Vulkan/TextureAllocator.cpp
        Src/Engine/Graphics/Vulkan/TextureAllocator.hpp
        Src/Engine/Graphics/Vulkan/MeshAllocator.cpp
        Src/Engine/Graphics/Vulkan/MeshAllocator.hpp
        Src/Engine/Graphics/Vulkan/ImageUtil.cpp
        Src/Engine/Graphics/Vulkan/ImageUtil.hpp
        Src/Engine/Graphics/CameraMatrices.hpp
        Src/Engine/Graphics/TextureArrayBuilder.cpp
        Src/Engine/Graphics/TextureArrayBuilder.hpp
        Src/Engine/Graphics/TextureArrayInfo.hpp
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



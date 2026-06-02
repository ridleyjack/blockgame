add_executable(Blockgame
        Src/main.cpp
        Src/GameLayer.cpp
        Src/GameLayer.hpp
        Src/Camera.cpp
        Src/Camera.hpp
        Src/Containers/Grid3D.hpp
        Src/Containers/ThreadSafeQueue.hpp
        Src/World/ChunkMesher.cpp
        Src/World/ChunkMesher.hpp
        Src/BlockRegistry.cpp
        Src/BlockRegistry.hpp
        Src/TextureRegistry.cpp
        Src/TextureRegistry.hpp
        Src/World/ChunkStreamer.cpp
        Src/World/ChunkStreamer.hpp
        Src/World/WorldGenerator.cpp
        Src/World/WorldGenerator.hpp
        Src/World/World.cpp
        Src/World/World.hpp
        Src/BlockTypes.hpp
        Src/World/Chunk.hpp
        Src/World/WorldStore.cpp
        Src/World/WorldStore.hpp
        Src/Highlight.cpp
        Src/Highlight.hpp
        Src/RenderItem.hpp
)

target_include_directories(Blockgame
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Src
)

target_link_libraries(Blockgame
        PRIVATE
        Engine
        Engine_Graphics
        glfw
)

# Textures
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/Textures
        DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

# Shaders
find_program(GLSLC glslc REQUIRED)

set(SHADER_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Shaders")
set(SHADER_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/Shaders")

set(SHADERS
        terrain.vert
        terrain.frag
        highlight.vert
        highlight.frag
)

set(SPIRV_OUTPUTS)

foreach (shader IN LISTS SHADERS)
    set(input "${SHADER_SOURCE_DIR}/${shader}")
    set(output "${SHADER_OUTPUT_DIR}/${shader}.spv")

    add_custom_command(
            OUTPUT "${output}"
            COMMAND "${CMAKE_COMMAND}" -E make_directory "${SHADER_OUTPUT_DIR}"
            COMMAND "${GLSLC}" "${input}" -o "${output}"
            DEPENDS "${input}"
            VERBATIM
            COMMENT "Compiling shader ${shader}"
    )

    list(APPEND SPIRV_OUTPUTS "${output}")
endforeach ()

add_custom_target(CompileShaders DEPENDS ${SPIRV_OUTPUTS})

add_dependencies(Blockgame CompileShaders)

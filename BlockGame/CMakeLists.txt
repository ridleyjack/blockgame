add_executable(Blockgame
        Src/main.cpp
        Src/GameLayer.cpp
        Src/GameLayer.hpp
)

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/Textures
        DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

# Copy shaders to the build directory.
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/Shaders
        DESTINATION ${CMAKE_CURRENT_BINARY_DIR})


target_compile_features(Blockgame PRIVATE cxx_std_23)

target_link_libraries(Blockgame
        PRIVATE Engine
)
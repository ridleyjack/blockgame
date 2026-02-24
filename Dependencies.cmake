include_guard(GLOBAL)
include(FetchContent)

# GLFW
FetchContent_Declare(
        glfw3
        DOWNLOAD_EXTRACT_TIMESTAMP OFF
        URL https://github.com/glfw/glfw/releases/download/3.4/glfw-3.4.zip
)
FetchContent_MakeAvailable(glfw3)

# GLM
FetchContent_Declare(
        glm
        DOWNLOAD_EXTRACT_TIMESTAMP OFF
        URL https://github.com/g-truc/glm/archive/refs/tags/1.0.1.zip
)
FetchContent_MakeAvailable(glm)

# GTest
if (ENABLE_TESTS)
    FetchContent_Declare(
            googletest
            URL https://github.com/google/googletest/archive/refs/tags/v1.17.0.zip
    )

    # Prevent overriding MSVC runtime settings (important on Windows)
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

    FetchContent_MakeAvailable(googletest)
endif ()
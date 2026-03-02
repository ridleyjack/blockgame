add_executable(
        engine_tests
        Memory/SparseBufferTest.cpp
        Memory/RingBufferTest.cpp
)

target_link_libraries(
        engine_tests
        PRIVATE
        Engine
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(engine_tests)

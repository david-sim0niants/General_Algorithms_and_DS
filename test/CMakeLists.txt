include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

set(TEST_TARGETS huffman_coding hash_table kmp_pattern_search union_find red_black_tree bloom_filter)
set(TEST_SRC_FILES ${TEST_TARGETS})
list(TRANSFORM TEST_SRC_FILES APPEND .cpp)

set(TARGET_NAME test_all)
add_executable(${TARGET_NAME} ${TEST_SRC_FILES})
target_link_libraries(${TARGET_NAME} GTest::gtest_main ${TEST_TARGETS})

include(GoogleTest)
gtest_discover_tests(${TARGET_NAME})

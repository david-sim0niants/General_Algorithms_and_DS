set(TARGET_NAME huffman_coding)
add_library(${TARGET_NAME} INTERFACE)
target_sources(${TARGET_NAME} INTERFACE huffman_coding.cpp)
target_include_directories(${TARGET_NAME} INTERFACE ${MAIN_INCLUDE_DIR})

set(TARGET_NAME hash_table)
add_library(${TARGET_NAME} INTERFACE)
target_include_directories(${TARGET_NAME} INTERFACE ${MAIN_INCLUDE_DIR})

set(TARGET_NAME kmp_pattern_search)
add_library(${TARGET_NAME} INTERFACE)
target_sources(${TARGET_NAME} INTERFACE kmp_pattern_search.cpp)
target_include_directories(${TARGET_NAME} INTERFACE ${MAIN_INCLUDE_DIR})

set(TARGET_NAME union_find)
add_library(${TARGET_NAME} INTERFACE)
target_include_directories(${TARGET_NAME} INTERFACE ${MAIN_INCLUDE_DIR})

set(TARGET_NAME red_black_tree)
add_library(${TARGET_NAME} INTERFACE)
target_include_directories(${TARGET_NAME} INTERFACE ${MAIN_INCLUDE_DIR})

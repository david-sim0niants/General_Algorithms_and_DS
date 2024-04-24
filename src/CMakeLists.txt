set(TARGET_NAME huffman_coding)
add_library(${TARGET_NAME} INTERFACE)
target_sources(${TARGET_NAME} INTERFACE huffman_coding.cpp)
target_include_directories(${TARGET_NAME} INTERFACE ${MAIN_INCLUDE_DIR})

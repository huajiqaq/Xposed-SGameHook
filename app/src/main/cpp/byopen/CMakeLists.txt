cmake_minimum_required(VERSION 3.4.1)

add_library(byopen STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/byopen_android.c
)

include_directories(
  ./include
)

target_include_directories(byopen PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

set_target_properties(byopen PROPERTIES
    C_STANDARD 99
)
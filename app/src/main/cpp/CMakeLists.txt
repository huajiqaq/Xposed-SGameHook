cmake_minimum_required(VERSION 3.22.1)
project("sgame_hook")


set(DOBBY_GENERATE_SHARED OFF CACHE BOOL "" FORCE)
if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(DOBBY_DEBUG OFF CACHE BOOL "" FORCE)
endif()

add_subdirectory(dobby)
add_subdirectory(byopen)
add_library(${CMAKE_PROJECT_NAME} SHARED
    sgame_hook.cpp
    atexit.cpp
)
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/dobby/include
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/byopen/include
)

find_library(log-lib log)
find_library(dl-lib dl)

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE
    dobby
    byopen
    ${log-lib}
    ${dl-lib}
)
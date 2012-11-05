project(ncextract)

cmake_minimum_required(VERSION 2.8)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH}
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake"
)

set(NCEXTRACT_SRC_DIR src)

add_subdirectory(${NCEXTRACT_SRC_DIR})


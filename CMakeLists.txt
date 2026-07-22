cmake_minimum_required(VERSION 3.10.0)
project(Kyra VERSION 0.1.0 LANGUAGES C CXX)

file(
    GLOB_RECURSE SOURCES
    src/*.cpp
    src/tools/*.cpp
    src/sony_libs/*.cpp
    src/sys/*.cpp
    src/sys/loader/*.cpp
    
)

add_executable(Kyra
    ${SOURCES}
)


target_include_directories(Kyra PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/sys
    ${CMAKE_CURRENT_SOURCE_DIR}/sys/loader
    
    ${CMAKE_CURRENT_SOURCE_DIR}/tools
    ${CMAKE_CURRENT_SOURCE_DIR}/sony_libs
    ${CMAKE_CURRENT_SOURCE_DIR}/external
)
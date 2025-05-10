
cmake_minimum_required(VERSION 3.28)

set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)
add_compile_options(-stdlib=libc++)
add_link_options(-stdlib=libc++)

set(CMAKE_BUILD_TYPE Debug)

project(poller)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON )

file(GLOB MOD_POLLER_SRC  modules/poller/*.cpp)
add_library(poller)
target_sources(poller PRIVATE ${MOD_POLLER_SRC})

add_subdirectory(test)

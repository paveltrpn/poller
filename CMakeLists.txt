
cmake_minimum_required(VERSION 3.28)

set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)
add_compile_options(-stdlib=libc++)
add_link_options(-stdlib=libc++)

set(CMAKE_BUILD_TYPE Debug)

project(poller)

include_directories("modules/poller")
include_directories("modules/io")

if (DEFINED ENV{NLOHMANN_JSON_DIR} AND NOT "$ENV{NLOHMANN_JSON_DIR}" STREQUAL "")
    message( "NLOHMANN_JSON_DIR is set to $ENV{NLOHMANN_JSON_DIR}")
    include_directories($ENV{NLOHMANN_JSON_DIR}/include)
else()
    message(SEND_ERROR "NLOHMANN_JSON_DIR is not set")
endif()

if (DEFINED ENV{LIBUV_DIR} AND NOT "$ENV{LIBUV_DIR}" STREQUAL "")
    message("LIBUV_DIR is set to $ENV{LIBUV_DIR}")
    include_directories($ENV{LIBUV_DIR}/include)
    link_directories($ENV{LIBUV_DIR}/build)
else()
    message(SEND_ERROR "LIBUV_DIR is not set")
endif()

if (DEFINED ENV{LIBCURL_DIR} AND NOT "$ENV{LIBCURL_DIR}" STREQUAL "")
    message("LIBCURL_DIR is set to $ENV{LIBCURL_DIR}")
    include_directories($ENV{LIBCURL_DIR}/include)
    link_directories($ENV{LIBCURL_DIR}/lib)
else()
    message(SEND_ERROR "LIBCURL_DIR is not set")
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON )

file(GLOB MOD_POLLER_SRC  modules/poller/*.cpp)
add_library(poller)
target_sources(poller PRIVATE ${MOD_POLLER_SRC})

file(GLOB MOD_IO_SRC  modules/io/*.cpp)
add_library(io)
target_sources(io PRIVATE ${MOD_IO_SRC})

add_subdirectory(test)

add_subdirectory(test/callback)
target_link_libraries(callback poller curl)

add_subdirectory(test/coro)
target_link_libraries(coro poller curl)

add_subdirectory(test/httpbin)
target_link_libraries(httpbin poller curl)

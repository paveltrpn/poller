
cmake_minimum_required(VERSION 3.28)

set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_CXX_FLAGS -stdlib=libc++)

#add_link_options(-stdlib=libc++)

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

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON )

file(GLOB MOD_POLLER_SRC  modules/poller/*.cpp modules/poller/*.h)
add_library(poller)
target_sources(poller PRIVATE ${MOD_POLLER_SRC})

# Module container
# ================================
file(GLOB MOD_CONTAINER_SRC modules/container/*.cpp)
add_library(container)
target_sources(container
  PUBLIC
    FILE_SET CXX_MODULES FILES
    ${MOD_CONTAINER_SRC}
)

# Module io
# ================================
file(GLOB MOD_IO_SRC modules/io/*.cpp)
add_library(io)
target_sources(io
  PUBLIC
    FILE_SET CXX_MODULES FILES
    ${MOD_IO_SRC}
)
target_link_libraries(io container)

# Module server
# ================================
file(GLOB MOD_SERVER_SRC modules/server/*.cpp)
add_library(server)
target_sources(server
  PUBLIC
    FILE_SET CXX_MODULES FILES
    ${MOD_SERVER_SRC}
)

# Module raw
# ================================
file(GLOB MOD_RAW_SRC modules/raw/*.cpp)
add_library(raw)
target_sources(raw
  PUBLIC
    FILE_SET CXX_MODULES FILES
    ${MOD_RAW_SRC}
)

# Module basio
# ================================
file(GLOB MOD_BASIO_SRC modules/basio/*.cpp)
add_library(basio)
target_sources(basio
  PUBLIC
    FILE_SET CXX_MODULES FILES
    ${MOD_BASIO_SRC}
)

file(GLOB MOD_SYNC_SRC  modules/sync/*.cpp modules/sync/*.h)
add_library(sync)
target_sources(sync PRIVATE ${MOD_SYNC_SRC})

add_subdirectory(test)

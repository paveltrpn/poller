cmake_minimum_required(VERSION 3.28)

# When specifying an out-of-tree source a binary directory must be explicitly specified.
#add_subdirectory(../third_party/googletest ../build)

enable_testing()

add_executable(io_file
    io/io_file.cpp
)
target_link_libraries(io_file uv io)

add_executable(io_timer
    io/io_timer.cpp
)
target_link_libraries(io_timer uv io)

add_executable(coro
    coro/main.cpp
)
target_link_libraries(coro poller curl)

add_executable(socket_test
    raw/socket.cpp
)
target_link_libraries(socket_test raw)

cmake_minimum_required(VERSION 3.28)

# When specifying an out-of-tree source a binary directory must be explicitly specified.
#add_subdirectory(../third_party/googletest ../build)

enable_testing()

add_executable(request
    request/request.cpp
)
target_link_libraries(request poller)

add_executable(io_reader
    io/io_reader.cpp
)
target_link_libraries(io_reader uv io)

add_executable(io_writer
    io/io_writer.cpp
)
target_link_libraries(io_writer uv io)

add_executable(io_timer
    io/io_timer.cpp
)
target_link_libraries(io_timer uv io)

add_executable(io_test
    io/io_test.cpp
)
target_link_libraries(io_test uv io)

add_executable(httpbin
    httpbin/httpbin.cpp
    httpbin/httpbin.h
    httpbin/main.cpp
)
target_link_libraries(httpbin poller curl)

add_executable(coro
    coro/main.cpp
)
target_link_libraries(coro poller curl)

add_executable(callback
    callback/main.cpp
)
target_link_libraries(callback poller curl)

add_executable(server_test
    server/main.cpp
)
target_link_libraries(server_test uv server)

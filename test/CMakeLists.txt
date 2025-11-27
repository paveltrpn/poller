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

add_executable(http_client
    http_client/http.cpp
)
target_link_libraries(http_client poller curl)

add_executable(postman)
target_sources(postman PUBLIC http_client/postman.cpp)
target_sources(postman PUBLIC FILE_SET CXX_MODULES FILES http_client/postman.cppm)
target_link_libraries(postman poller curl)

add_executable(httpbin)
target_sources(httpbin PUBLIC http_client/httpbin.cpp)
target_sources(httpbin PUBLIC FILE_SET CXX_MODULES FILES http_client/httpbin.cppm)
target_link_libraries(httpbin poller curl)

add_executable(gstatic)
target_sources(gstatic PUBLIC http_client/gstatic.cpp)
target_sources(gstatic PUBLIC FILE_SET CXX_MODULES FILES http_client/gstatic.cppm)
target_link_libraries(gstatic poller curl)

#add_executable(coindesk)
#target_sources(coindesk PUBLIC http_client/coindesk.cpp)
#target_sources(coindesk PUBLIC FILE_SET CXX_MODULES FILES http_client/coindesk.cppm)
#target_link_libraries(coindesk poller curl)

add_executable(socket_test
    raw/socket.cpp
)
target_link_libraries(socket_test raw)

add_executable(pingpong
    coro/pingpong.cpp
)
target_link_libraries(pingpong coro)

add_executable(async_manual_reset_event
    coro/async_manual_reset_event.cpp
)
target_link_libraries(async_manual_reset_event coro)

add_executable(generator
    coro/generator.cpp
)
target_link_libraries(generator coro)

cmake_minimum_required(VERSION 3.28)

# When specifying an out-of-tree source a binary directory must be explicitly specified.
#add_subdirectory(../third_party/googletest ../build)

enable_testing()

add_executable(postman)
target_sources(postman PUBLIC http_client/postman.cpp)
target_sources(postman PUBLIC FILE_SET CXX_MODULES FILES http_client/postman.cppm)
target_link_libraries(postman poller curl)

add_executable(httpbin)
target_sources(httpbin PUBLIC http_client/httpbin.cpp)
target_sources(httpbin PUBLIC FILE_SET CXX_MODULES FILES http_client/httpbin.cppm)
target_link_libraries(httpbin poller curl)

add_executable(socket_test
    raw/socket.cpp
)
target_link_libraries(socket_test raw)


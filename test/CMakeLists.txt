cmake_minimum_required(VERSION 3.28)

enable_testing()

add_executable(postman)
target_sources(postman PUBLIC http_client/postman.cpp)
target_sources(postman PUBLIC FILE_SET CXX_MODULES FILES http_client/postman.cppm)
target_link_libraries(postman poller curl)

add_executable(httpbin)
target_sources(httpbin PUBLIC http_client/httpbin.cpp)
target_sources(httpbin PUBLIC FILE_SET CXX_MODULES FILES http_client/httpbin.cppm)
target_link_libraries(httpbin poller curl)

add_executable(uvtcp
    uvtcp/uvtcp.cpp
)
target_link_libraries(uvtcp uv)

add_executable(timer
    io/timer.cpp
)
target_link_libraries(timer uv io)

add_executable(fileio
    io/fileio.cpp
)
target_link_libraries(fileio uv io)




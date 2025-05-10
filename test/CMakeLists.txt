cmake_minimum_required(VERSION 3.28)

# When specifying an out-of-tree source a binary directory must be explicitly specified.
#add_subdirectory(../third_party/googletest ../build)

enable_testing()

add_executable(request
    request/request.cpp
)

target_link_libraries(request poller)

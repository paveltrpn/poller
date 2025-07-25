Poller
========

Dependencies
=============

Project depends on:
libcurl 
libuv 
nlohmann-json

All this libraries available in source codes and easy to build 
and install. CMake script assumes that build to be done manually and corresponding envvars will
be provided. See below...

libcurl:
TODO...
$ apt install libpsl-dev

libuv:
$ git clone https://github.com/libuv/libuv
$ cd libuv
$ mkdir build && cd build
$ cmake .. && cmake --install . --prefix=/{some}/{path}

nlohmann-json:
$ cs /{some}/{path}
$ git clone https://github.com/nlohmann/json

Set envvars
============

export LIBCURL_DIR=/{some}/{path}/curl
export LIBUV_DIR=/{some}/{path}/libuv
export NLOHMANN_JSON_DIR/{some}/{path}/json

Build
========

Compiler with C++26 standart support rquired. By default compiler set to clang and libc++ standart
library (because of better modern standart support). Build type set to "Debug" by default (-DCMAKE_BUILD_TYPE to redefine).

$ mkdir build
$ cd build
$ cmake ..
$ cmake --build .

You may redefine compiler and standart library by:

$ cmake .. -DCMAKE_CXX_FLAGS={-stdlib=libstdc++ | -stdlib=libc++} -DCMAKE_CXX_COMPILER={$(which clang++) | $(which g++)}
$ cmake --build . 


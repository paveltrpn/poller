Poller
========

Dependencies
=============

Curl:
-------------
build deps:
$ apt install libpsl-dev

Set envvars
============

export LIBCURL_DIR=/{some}/{path}/curl
export LIBUV_DIR=/{some}/{path}/libuv

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


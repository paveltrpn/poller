# Poller

This is a library that wraps a CURL and libuv with support of C++20 coroutines.  

## About

CURL part of this project is a simple HTTP client library leveraging C++20 coroutines and the libuv part provides a asychronous timer, disk and network operations (by now is only timer and very simple file open operation:).  

### Building Dependencies

All dependencies are available as source code and should be built manually:

**libcurl:**
```bash
$ apt install libpsl-dev
$ git clone https://github.com/curl/curl
$ mkdir build && cd build
$ cmake .. 
$ cmake --build . --parallel {N}
$ cmake --install . --prefix=/{some}/{path}
```

**libuv:**
```bash
$ git clone https://github.com/libuv/libuv
$ cd libuv
$ mkdir build && cd build
$ cmake .. 
$ cmake --build . --parallel {N}
$ cmake --install . --prefix=/{some}/{path}
```

**nlohmann-json:**
```bash
$ cd {some}/{path}
$ git clone https://github.com/nlohmann/json
```

## Build

### Requirements

- Compiler with C++26 support (clang++ recommended)
- Fresh CMake with support of C++20 modules

### Configuration

Set environment variables pointing to your dependency installations:

```bash
export LIBCURL_DIR={some}/{path}/curl
export LIBUV_DIR={some}/{path}/libuv
export NLOHMANN_JSON_DIR={some}/{path}/json
```

### Compilation  
As usual with CMake based projects  

```bash
$ mkdir build
$ cd build
$ cmake ..
$ cmake --build .
```

### Compiler Configuration

By default, the project uses `clang++` with `libc++` for better C++26 support.  

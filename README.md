# Poller

This is a library that wraps a CURL and libuv with support of C++20 coroutines.  

## About

CURL part of this project is a simple HTTP client library leveraging C++20 coroutines and the libuv part is provides a asychronous timer, disk and network operations (by now is only timer:).  

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
- CMake 4.2 or higher

### Configuration

Set environment variables pointing to your dependency installations:

```bash
export LIBCURL_DIR={some}/{path}/curl
export LIBUV_DIR={some}/{path}/libuv
export NLOHMANN_JSON_DIR={some}/{path}/json
```

### Compilation

```bash
$ mkdir build
$ cd build
$ cmake ..
$ cmake --build .
```

### Compiler Configuration

By default, the project uses `clang++` with `libc++` for better C++26 support. You can override:

```bash
$ cmake .. -DCMAKE_CXX_FLAGS=-stdlib=libstdc++ -DCMAKE_CXX_COMPILER=$(which g++)
$ cmake .. -DCMAKE_CXX_FLAGS=-stdlib=libc++ -DCMAKE_CXX_COMPILER=$(which clang++)
```

Build type can be changed via `-DCMAKE_BUILD_TYPE`:

```bash
$ cmake .. -DCMAKE_BUILD_TYPE=Release
```

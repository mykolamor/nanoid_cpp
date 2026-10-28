# nanoid-cpp
[![Build status](https://github.com/mykolamor/nanoid_cpp/actions/workflows/cmake-multi-platform.yml/badge.svg?branch=master)](https://github.com/mykolamor/nanoid_cpp/actions/workflows/cmake-multi-platform.yml)
[![License](https://img.shields.io/badge/license-MIT%20License-blue.svg)](LICENSE)

A tiny, URL-friendly, unique string ID generator for C++, implementation of [ai's](https://github.com/ai) [nanoid](https://github.com/ai/nanoid)!

**Safe.** By default it uses the operating system's cryptographically secure random source: `getrandom` on Linux, `arc4random_buf` on macOS, BSD and Android, `BCryptGenRandom` on Windows. You can plug in a source of your choice (OpenSSL, Crypto++, Botan, LibCrypto, ...) using the [API](#custom-random-bytes-generator).

**Thread-safe.** The default generator can be used from any number of threads at once. See [Thread Safety](#thread-safety) for custom generators.

**Compact.** It uses more symbols than UUID (`A-Za-z0-9_-`)
and has the same number of unique options in just 22 symbols instead of 36.

**Fast.** Nanoid is as fast as UUID but can be used in URLs.

## Install

nanoid needs CMake 3.10 or newer. Whichever way you add it, link the `nanoid::nanoid` target:

```cmake
target_link_libraries(your_target PRIVATE nanoid::nanoid)
```

**Installed package:**

```sh
cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build && cmake --install build
```

```cmake
find_package(nanoid 2 CONFIG REQUIRED)
```

**Subproject or git submodule:**

```cmake
add_subdirectory(external/nanoid_cpp)
```

**FetchContent** (CMake 3.14+):

```cmake
include(FetchContent)
FetchContent_Declare(nanoid GIT_REPOSITORY https://github.com/mykolamor/nanoid_cpp GIT_TAG master)
FetchContent_MakeAvailable(nanoid)
```

**CPM.cmake:**

```cmake
CPMAddPackage(NAME nanoid GITHUB_REPOSITORY mykolamor/nanoid_cpp GIT_TAG master)
```

As a subproject, nanoid's tests, examples and install rules are off by default.
Set `NANOID_INSTALL=ON` if your project installs targets that link nanoid.

## Usage

### Normal

The default method uses URL-friendly symbols (`A-Za-z0-9_-`) and returns an ID
with 21 characters (to have a collision probability similar to UUID v4).

```cpp
std::string id = nanoid::generate();
//id := "Uakgb_J5m9g-0JDMbcJqLJ"
```

Symbols `-,.()` are not encoded in the URL. If used at the end of a link
they could be identified as a punctuation symbol.

If you want to reduce ID length (and increase collisions probability),
you can pass the size as an argument:

```cpp
std::string id = nanoid::generate(10);
//id := "IRFa-VaY2b"
```

### Async

The async method runs `generate` on a new thread.

```cpp
auto id_fut = nanoid::generate_async();
//unrelated code here
std::string id = id_fut.get();
//id := "Uakgb_J5m9g-0JDMbcJqLJ"
```

### Custom Alphabet or Length

If you want to change the ID's alphabet or length
you can pass alphabet and size.

```cpp
std::string id = nanoid::generate("1234567890abcdef", 10);
//id := "4f90d13a42"
```

The alphabet must contain 1 to 256 symbols.
Multi-byte UTF-8 characters can't be used.
Repeated symbols are allowed but make those symbols more likely.

### Custom Random Bytes Generator

You can replace the default generator in two ways:

- Derive from `nanoid::crypto_random_base` and implement `next_bytes`. See [examples/openssl](examples/openssl/openssl.cpp).
- Wrap a standard random engine with `nanoid::engine_random<G>`. See [examples/mt19937](examples/mt19937/mt19937.cpp).

For instance, to use a seed-based generator:

```cpp
nanoid::engine_random<std::mt19937> random(10);
auto id = nanoid::generate(random, "abcdef", 10);
//id := "bffffedeef"
```

`engine_random<G>` takes only uniformly random bytes from each output.
The engine has to produce at least 8 random bits per call.
Seed-based engines like `std::mt19937` are **not** cryptographically secure:
their output [can be predicted](https://github.com/altf4/untwister) after observing enough IDs.
Use them only where IDs don't have to be unguessable, e.g. reproducible IDs in tests.

### Thread Safety

The default generator is thread-safe: `generate()` and `generate_async()` can be called
from any number of threads at once.

When you pass your own generator, thread safety is up to that generator.
`nanoid::engine_random<G>` is **not** thread-safe, because standard engines keep internal state.
Give each thread its own instance (e.g. with `thread_local`), or guard a shared one with a mutex.
Don't share one instance between concurrent `generate` calls.

### Random Byte Buffering

On Linux, the default generator buffers random bytes per thread, which is faster on average, but some calls are slower when the buffer refills.
For more uniform call times, build with `-DNANOID_RANDOM_BUFFER=OFF` (or define `NANOID_DISABLE_RANDOM_BUFFER` without CMake).

### Custom Namespace

To put the API in another namespace than `nanoid`, set `-DNANOID_NAMESPACE=your_namespace`.
CMake passes it on to everything that links `nanoid::nanoid`.
Without CMake, define `NANOID_NAMESPACE` identically for nanoid and all code using it, or linking fails.

## Credits

- [ai](https://github.com/ai) - [nanoid](https://github.com/ai/nanoid)
- [codeyu](https://github.com/codeyu) - [nanoid-net](https://github.com/codeyu/nanoid-net/)

## License

The MIT License (MIT). Please see [License File](LICENSE) for more information.

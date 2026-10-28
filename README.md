# nanoid-cpp
[![Build status](https://github.com/mykolamor/nanoid_cpp/actions/workflows/cmake-multi-platform.yml/badge.svg?branch=master)](https://github.com/mykolamor/nanoid_cpp/actions/workflows/cmake-multi-platform.yml)
[![License](https://img.shields.io/badge/license-MIT%20License-blue.svg)](LICENSE)

A tiny, URL-friendly, unique string ID generator for C++, implementation of [ai's](https://github.com/ai) [nanoid](https://github.com/ai/nanoid)!

**Safe.** By default it uses the operating system's cryptographically secure random source: `getrandom` on Linux, `arc4random_buf` on macOS, BSD and Android, `BCryptGenRandom` on Windows. You can plug in a source of your choice (OpenSSL, Crypto++, Botan, LibCrypto, ...) using the [API](#custom-random-bytes-generator).

**Thread-safe.** The default generator can be used from any number of threads at once. See [Thread Safety](#thread-safety) for custom generators.

**Compact.** It uses more symbols than UUID (`A-Za-z0-9_-`)
and has the same number of unique options in just 21 symbols instead of 36.

**Fast.** An ID with default settings takes about 0.1 µs on Linux (measured on a Ryzen 9 5950X).

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
find_package(nanoid 4 CONFIG REQUIRED)
```

**Subproject or git submodule:**

```cmake
add_subdirectory(external/nanoid_cpp)
```

**FetchContent** (CMake 3.14+):

```cmake
include(FetchContent)
FetchContent_Declare(nanoid GIT_REPOSITORY https://github.com/mykolamor/nanoid_cpp GIT_TAG 4.0.1)
FetchContent_MakeAvailable(nanoid)
```

**CPM.cmake:**

```cmake
CPMAddPackage(NAME nanoid GITHUB_REPOSITORY mykolamor/nanoid_cpp GIT_TAG 4.0.1)
```

**vcpkg:** the port is in this repository, so point vcpkg at its `ports` folder
(`--overlay-ports=<nanoid_cpp>/ports`, or `VCPKG_OVERLAY_PORTS`) and depend on `nanoid-cpp`.
Then use `find_package(nanoid CONFIG REQUIRED)` as above.

As a subproject, nanoid's examples, command-line tool and install rules are off by default.
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

To generate many IDs with the same alphabet, create a generator once.
It checks the alphabet when constructed:

```cpp
nanoid::custom_alphabet generate_id("1234567890abcdef", 10);
std::string id = generate_id();        // 10 symbols
std::string short_id = generate_id(5); // 5 symbols
```

The default alphabet is available as `nanoid::url_alphabet`.

### Custom Random Bytes Generator

You can replace the default generator in two ways:

- Derive from `nanoid::crypto_random_base` and implement `next_bytes`. See [examples/openssl](examples/openssl/openssl.cpp).
- Wrap a standard random engine with `nanoid::engine_random<G>`. See [examples/mt19937](examples/mt19937/mt19937.cpp).

For instance, to use a seed-based generator:

```cpp
nanoid::engine_random<std::mt19937> random(10);
auto id = nanoid::generate(random, "abcdef", 10);
//id := "dddffadeab"
```

`engine_random<G>` takes only uniformly random bytes from each output.
The engine has to produce at least 8 random bits per call.
Seed-based engines like `std::mt19937` are **not** cryptographically secure:
their output [can be predicted](https://github.com/altf4/untwister) after observing enough IDs.
Use them only where IDs don't have to be unguessable, e.g. reproducible IDs in tests.

### Thread Safety

The default generator is thread-safe: `generate()` can be called from any number of threads at once.

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

### Command Line

nanoid also builds a `nanoid` command-line tool (`NANOID_BUILD_CLI`, on when nanoid is built on its own):

```sh
$ nanoid
Uakgb_J5m9g-0JDMbcJqLJ
$ nanoid --size 10 --alphabet abc
aabaccbbab
```

Run `nanoid --help` for all options.

## Tests

The tests are off by default. They use Catch2, which is downloaded if it isn't installed:

```sh
cmake -B build -DNANOID_BUILD_TESTS=ON && cmake --build build && cd build && ctest
```

## Credits

- [ai](https://github.com/ai) - [nanoid](https://github.com/ai/nanoid)
- [codeyu](https://github.com/codeyu) - [nanoid-net](https://github.com/codeyu/nanoid-net/)

## License

The MIT License (MIT). Please see [License File](LICENSE) for more information.

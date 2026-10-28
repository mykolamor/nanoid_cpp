cmake_minimum_required(VERSION 3.10...3.31)

file(READ "${CMAKE_CURRENT_SOURCE_DIR}/inc/nanoid/settings.h" NANOID_SETTINGS_H)
foreach(part MAJOR MINOR PATCH)
    string(REGEX MATCH "#define NANOID_VERSION_${part} ([0-9]+)" _ "${NANOID_SETTINGS_H}")
    set(NANOID_VERSION_${part} ${CMAKE_MATCH_1})
endforeach()

project(nanoid_cpp
    VERSION ${NANOID_VERSION_MAJOR}.${NANOID_VERSION_MINOR}.${NANOID_VERSION_PATCH}
    LANGUAGES CXX
    )

if (CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    set(NANOID_TOP_LEVEL ON)
else()
    set(NANOID_TOP_LEVEL OFF)
endif()

option(NANOID_BUILD_TESTS "Build nanoid unit tests" ${NANOID_TOP_LEVEL})
option(NANOID_BUILD_EXAMPLES "Build nanoid usage examples" ${NANOID_TOP_LEVEL})
option(NANOID_INSTALL "Generate nanoid install rules" ${NANOID_TOP_LEVEL})
option(NANOID_RANDOM_BUFFER "Buffer OS random bytes per thread on Linux. Faster on average, occasional slow refills" ON)
set(NANOID_NAMESPACE "nanoid" CACHE STRING "C++ namespace of nanoid's API")

include(GNUInstallDirs)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_library(nanoid
    src/nanoid/crypto_random.cpp
    src/nanoid/nanoid.cpp
    )
add_library(nanoid::nanoid ALIAS nanoid)
target_include_directories(nanoid PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    )
target_compile_features(nanoid PUBLIC cxx_std_11)
target_link_libraries(nanoid PRIVATE Threads::Threads)
if (WIN32)
    # BCryptGenRandom
    target_link_libraries(nanoid PRIVATE bcrypt)
endif()
if (NOT NANOID_RANDOM_BUFFER)
    target_compile_definitions(nanoid PRIVATE NANOID_DISABLE_RANDOM_BUFFER)
endif()
if (NOT NANOID_NAMESPACE STREQUAL "nanoid")
    # PUBLIC, so code linking nanoid::nanoid sees the same namespace, also via find_package.
    target_compile_definitions(nanoid PUBLIC NANOID_NAMESPACE=${NANOID_NAMESPACE})
endif()
target_compile_options(nanoid PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -pedantic>
    )
set_target_properties(nanoid PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    # Export all symbols for shared builds on Windows.
    WINDOWS_EXPORT_ALL_SYMBOLS ON
    )

#############################################
# Examples
if (NANOID_BUILD_EXAMPLES AND NOT NANOID_NAMESPACE STREQUAL "nanoid")
    # The examples are written against nanoid:: for readability.
    message(STATUS "Skipping examples: they require NANOID_NAMESPACE=nanoid")
elseif (NANOID_BUILD_EXAMPLES)
    add_subdirectory(examples)
    message(STATUS "Added examples to build")
endif()

#############################################
# Install
if (NANOID_INSTALL)
    include(CMakePackageConfigHelpers)
    set(NANOID_CMAKE_DIR ${CMAKE_INSTALL_LIBDIR}/cmake/nanoid)

    install(DIRECTORY inc/nanoid DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
    install(TARGETS nanoid EXPORT nanoid-targets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        )
    install(EXPORT nanoid-targets
        NAMESPACE nanoid::
        DESTINATION ${NANOID_CMAKE_DIR}
        )

    # Config files for find_package()
    configure_package_config_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/Config.cmake.in
        "${CMAKE_CURRENT_BINARY_DIR}/nanoid-config.cmake"
        INSTALL_DESTINATION ${NANOID_CMAKE_DIR}
        )
    write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/nanoid-config-version.cmake"
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
        )
    install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/nanoid-config.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/nanoid-config-version.cmake"
        DESTINATION ${NANOID_CMAKE_DIR}
        )
endif()

#############################################
# Unit tests
if (NANOID_BUILD_TESTS)
    find_package(Catch2 3 QUIET)
    if (Catch2_FOUND)
        message(STATUS "Using installed Catch2 ${Catch2_VERSION} for tests")
    else()
        if (CMAKE_VERSION VERSION_LESS 3.16)
            message(FATAL_ERROR "nanoid's tests need Catch2 3 installed, or CMake 3.16+ to download it. "
                "Set NANOID_BUILD_TESTS=OFF to build without tests.")
        endif()
        include(FetchContent)
        FetchContent_Declare(Catch2
            URL https://github.com/catchorg/Catch2/archive/refs/tags/v3.16.1.tar.gz
            URL_HASH SHA256=d16bd1b8b2364918bd472cb216394acb93bde6784890df0d91bccfb3f28d777d
            )
        FetchContent_MakeAvailable(Catch2)
        message(STATUS "Downloaded Catch2 3.16.1 for tests")
    endif()

    enable_testing()
    add_executable(nanoid_tests tests/unit_tests.cpp)
    # Catch2 v3 needs C++14. The library itself stays C++11.
    target_compile_features(nanoid_tests PRIVATE cxx_std_14)

    target_link_libraries(nanoid_tests
        nanoid::nanoid
        Catch2::Catch2WithMain
        Threads::Threads
        )

    add_test(NAME nanoid_tests COMMAND nanoid_tests)
    message(STATUS "Added tests to build")
endif()

cmake_minimum_required(VERSION 3.5)
project (nanoid_cpp
    VERSION 2.1.0
    )

set(CMAKE_CXX_STANDARD 11)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

option(BUILD_TESTS "Build nanoid unit tests" ON)
option(BUILD_EXAMPLES "Build nanoid usage examples" ON)
option(NANOID_RANDOM_BUFFER "Buffer OS random bytes per thread on Linux. Faster on average, occasional slow refills" ON)

add_subdirectory(vendor/catch2)

file(GLOB_RECURSE NANOID_SRC_FILES src/*.cpp)
file(GLOB_RECURSE NANOID_INC_FILES inc/*.h)

add_library(nanoid ${NANOID_SRC_FILES})
target_include_directories(nanoid PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc>
  $<INSTALL_INTERFACE:inc>)
target_link_libraries(nanoid Threads::Threads)
if (WIN32)
    # BCryptGenRandom
    target_link_libraries(nanoid bcrypt)
endif()
if (NOT NANOID_RANDOM_BUFFER)
    target_compile_definitions(nanoid PRIVATE NANOID_DISABLE_RANDOM_BUFFER)
endif()
#target_compile_options(nanoid PRIVATE
#  $<$<CXX_COMPILER_ID:MSVC>:/W4>
#  $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -pedantic>
#)

#############################################
# Examples
if (BUILD_EXAMPLES)
    add_subdirectory(examples)
    message(STATUS "Added examples to build")
endif()

#############################################
# Install
install(DIRECTORY inc/nanoid DESTINATION include)
install(TARGETS nanoid EXPORT nanoid-targets)
install(EXPORT nanoid-targets DESTINATION lib/cmake/nanoid)

# # Config files for find_package()
include(CMakePackageConfigHelpers)
configure_package_config_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/Config.cmake.in
  "${CMAKE_CURRENT_BINARY_DIR}/nanoid-config.cmake"
  INSTALL_DESTINATION lib/cmake/nanoid)
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/nanoid-version.cmake"
  VERSION "2.1.0"
  COMPATIBILITY AnyNewerVersion)
install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/nanoid-config.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/nanoid-version.cmake"
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/FindNanoid.cmake"
  DESTINATION lib/cmake/nanoid)
export(EXPORT nanoid-targets
  FILE "${CMAKE_CURRENT_BINARY_DIR}/cmake/nanoid-targets.cmake")


#############################################
# Unit tests
if (BUILD_TESTS)
    enable_testing()
    add_executable(nanoid_tests tests/unit_tests.cpp)

    target_link_libraries(nanoid_tests
            nanoid
            Catch2::Test
            )

    add_test(test_all nanoid_tests)
    message(STATUS "Added tests to build")
endif()

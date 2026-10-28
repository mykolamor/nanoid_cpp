# Builds against an installed nanoid package, to check that find_package(nanoid) works.
cmake_minimum_required(VERSION 3.10)
project(nanoid_package_test CXX)

find_package(nanoid CONFIG REQUIRED)

add_executable(package_test main.cpp)
target_link_libraries(package_test nanoid::nanoid)

enable_testing()
add_test(NAME package_test COMMAND package_test)

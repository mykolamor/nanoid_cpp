set(EXAMPLE_NAME example_mt19937)
add_executable(${EXAMPLE_NAME} mt19937.cpp)
target_link_libraries(${EXAMPLE_NAME} nanoid::nanoid)
message(STATUS "Building ${EXAMPLE_NAME}")

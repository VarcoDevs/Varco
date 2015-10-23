cmake_minimum_required (VERSION 3.1)
project (varco)
set (CMAKE_CXX_STANDARD 11)

set (SRCS src/init.cpp)

add_subdirectory (deps/skia/cmake deps/skia)

add_executable (varco ${SRCS})
target_link_libraries (varco skia)
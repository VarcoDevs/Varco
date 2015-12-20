cmake_minimum_required (VERSION 3.1)
project (varco)
set (CMAKE_CXX_STANDARD 11)

set (SRCS src/init.cpp)
include_directories (src)

# Window handling sources
set (WINDOWHANDLING_SRCS 
  src/WindowHandling/MainWindow.cpp
  src/WindowHandling/MainWindow.hpp)
set (SRCS ${SRCS} ${WINDOWHANDLING_SRCS})
source_group (WindowHandling FILES ${WINDOWHANDLING_SRCS})

# Configure Skia's CMakeLists.txt file found in deps/skia/cmake, build it into
# deps/skia
add_subdirectory (deps/skia/cmake deps/skia)

add_executable (varco ${SRCS})
add_dependencies (varco skia)
target_link_libraries (varco skia)
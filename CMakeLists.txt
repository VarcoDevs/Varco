cmake_minimum_required (VERSION 3.1)
project (varco)
set (CMAKE_CXX_STANDARD 11)

set (SRCS src/init.cpp)

# Window handling sources
set (WINDOWHANDLING_SRCS 
  src/WindowHandling/MainWindow.cpp
  src/WindowHandling/MainWindow.hpp)
set (SRCS ${SRCS} ${WINDOWHANDLING_SRCS})
source_group (WindowHandling FILES ${WINDOWHANDLING_SRCS})

# DEBUG - Configure resource dir
configure_file(src/Debug/stubs.h.in src/Debug/stubs.h)

# Debug stuff
set (SRCS ${SRCS} ${CMAKE_BINARY_DIR}/src/Debug/stubs.h)
source_group (Debug FILES ${CMAKE_BINARY_DIR}/src/Debug/stubs.h)

# Configure Skia's CMakeLists.txt file found in deps/skia/cmake, build it into
# deps/skia
add_subdirectory (deps/skia/cmake deps/skia)

add_executable (varco ${SRCS})
target_include_directories (varco PUBLIC src) # Add /src as include directory
target_include_directories (varco PUBLIC ${CMAKE_BINARY_DIR}/src) # Add build/src as include directory
target_link_libraries (varco skia) # Add skia dependency
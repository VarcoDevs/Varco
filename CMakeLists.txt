cmake_minimum_required (VERSION 3.1)
project (varco)
set (CMAKE_CXX_STANDARD 14)

set (SRCS src/init.cpp)
set (INCLUDES src)
set (LIBS)

find_package (OpenGL REQUIRED)
list (APPEND LIBS ${OPENGL_LIBRARIES})

list (APPEND INCLUDES 
                        deps/skia/include/android
                        deps/skia/include/animator
                        deps/skia/include/c
                        deps/skia/include/codec
                        deps/skia/include/config
                        deps/skia/include/core
                        deps/skia/include/device
                        deps/skia/include/effects
                        deps/skia/include/gpu
                        deps/skia/include/images
                        deps/skia/include/pathops
                        deps/skia/include/pipe
                        deps/skia/include/ports
                        deps/skia/include/private
                        deps/skia/include/svg
                        deps/skia/include/utils
                        deps/skia/include/views
                        deps/skia/include/xml)
                        
# ---------------- Platform specific code ------------------- #
if (WIN32)
    set (WINDOWHANDLING_SRCS 
            src/WindowHandling/BaseOSWindow_Win.cpp
            src/WindowHandling/BaseOSWindow_Win.hpp
            src/WindowHandling/MainWindow.cpp
            src/WindowHandling/MainWindow.hpp)
    list (APPEND LIBS
            optimized;${CMAKE_SOURCE_DIR}/deps/skia/lib/win64/skia.lib;
            debug;${CMAKE_SOURCE_DIR}/deps/skia/lib/win64/skiad.lib;)
endif()
# ----------------------------------------------------------- #

list (APPEND SRCS ${WINDOWHANDLING_SRCS})
source_group (WindowHandling FILES ${WINDOWHANDLING_SRCS})

add_executable (varco ${SRCS})
target_include_directories (varco PUBLIC ${INCLUDES})
target_link_libraries (varco ${LIBS})

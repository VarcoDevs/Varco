cmake_minimum_required (VERSION 3.1)
project (varco)
set (CMAKE_CXX_STANDARD 14)

set (SRCS src/init.cpp)
set (INCLUDES src)
set (LIBS)

find_package (OpenGL REQUIRED)
list (APPEND LIBS ${OPENGL_LIBRARIES})

########################################### Skia dependency ########################################### 
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

# 7zip is preferable
set (CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

if (WIN32)
    set (WINDOWHANDLING_SRCS 
            src/WindowHandling/BaseOSWindow_Win.cpp
            src/WindowHandling/BaseOSWindow_Win.hpp
            src/WindowHandling/MainWindow.cpp
            src/WindowHandling/MainWindow.hpp)
    list (APPEND LIBS
            optimized;${CMAKE_SOURCE_DIR}/deps/skia/lib/win64/skia.lib;
            debug;${CMAKE_SOURCE_DIR}/deps/skia/lib/win64/skiad.lib;)
    
    # Decompress binaries if needed    
    if (NOT EXISTS ${CMAKE_SOURCE_DIR}/deps/skia/lib/win64/skia.lib)
      message (STATUS "Decompressing Win64 Skia binaries...")
      # 7zip is preferable, but as a fallback use cmake built-in 7zip module
      find_package (SevenZip)
      if (NOT SEVENZIP_FOUND)
        execute_process(
            COMMAND ${CMAKE_COMMAND} -E tar xzf ${CMAKE_SOURCE_DIR}/deps/skia/lib/win64/libs.7z
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/deps/skia/lib/win64 # Folder where to decompress stuff
        )
      else() # Use 7Zip
        execute_process(
            COMMAND ${SEVENZIP_BINARY} x ${CMAKE_SOURCE_DIR}/deps/skia/lib/win64/libs.7z
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/deps/skia/lib/win64 # Folder where to decompress stuff
        )
      endif()
    endif()    
endif()
########################################### Skia dependency ########################################### 

list (APPEND SRCS ${WINDOWHANDLING_SRCS})
source_group (WindowHandling FILES ${WINDOWHANDLING_SRCS})

add_executable (varco ${SRCS})
target_include_directories (varco PUBLIC ${INCLUDES})
target_link_libraries (varco ${LIBS})
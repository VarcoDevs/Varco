cmake_minimum_required (VERSION 3.1)
project (varco)
set (CMAKE_CXX_STANDARD 14)

if (NOT CMAKE_BUILD_TYPE)
  set (CMAKE_BUILD_TYPE Release)
endif ()

set (SRCS src/init.cpp)
set (INCLUDES src)
set (LIBS)

find_package (OpenGL REQUIRED)
list (APPEND LIBS ${OPENGL_LIBRARIES})

########################################### Zlib dependency ###########################################

add_subdirectory(deps/zlib)

set(ZLIB_INCLUDE_DIRS  "${CMAKE_SOURCE_DIR}/deps/zlib;${CMAKE_BINARY_DIR}/deps/zlib")
set(ZLIB_LIBRARIES "zlibstatic")
set(ZLIB_FOUND true)

######################################## End Zlib dependency ##########################################

########################################### Skia dependency ###########################################

# No gpu log calls in debug
set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DGR_GL_LOG_CALLS=0 -DGR_GL_CHECK_ERROR=0")

set (BUILD_SHARED_LIBS FALSE) # This causes skia to be built as a static lib
add_subdirectory (deps/skia/cmake)

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
            deps/skia/src/gpu
            deps/skia/include/images
            deps/skia/include/pathops
            deps/skia/include/pipe
            deps/skia/include/ports
            deps/skia/include/private
            deps/skia/include/svg
            deps/skia/include/utils
            deps/skia/include/views
            deps/skia/include/xml)

if (WIN32)
  set (WINDOWHANDLING_SRCS 
            src/WindowHandling/BaseOSWindow_Win.cpp
            src/WindowHandling/BaseOSWindow_Win.hpp
            src/WindowHandling/MainWindow.cpp
            src/WindowHandling/MainWindow.hpp)
endif()

if (UNIX)
  set (WINDOWHANDLING_SRCS
            src/WindowHandling/BaseOSWindow_Linux.cpp
            src/WindowHandling/BaseOSWindow_Linux.hpp
            src/WindowHandling/MainWindow.cpp
            src/WindowHandling/MainWindow.hpp)
endif()
########################################## End Skia dependency ##########################################

list (APPEND SRCS ${WINDOWHANDLING_SRCS})
source_group (WindowHandling FILES ${WINDOWHANDLING_SRCS})

set (CONTROL_SRCS
            src/Control/DocumentManager.cpp
            src/Control/DocumentManager.hpp)
list (APPEND SRCS ${CONTROL_SRCS})
source_group (Control FILES ${CONTROL_SRCS})

set (UI_SRCS
            src/UI/UIElement.cpp
            src/UI/UIElement.hpp
            src/UI/TabBar/TabBar.cpp
            src/UI/TabBar/TabBar.hpp
            src/UI/ScrollBar/ScrollBar.cpp
            src/UI/ScrollBar/ScrollBar.hpp
            src/UI/CodeView/CodeView.cpp
            src/UI/CodeView/CodeView.hpp)
list (APPEND SRCS ${UI_SRCS})
source_group (UI FILES ${UI_SRCS})

set (UTILS_SRCS
            src/Utils/Commons.hpp
            src/Utils/Utils.hpp
            src/Utils/VKeyCodes.hpp
            src/Utils/Concurrent.hpp
            src/Utils/Interpolators.hpp)
if (WIN32)
  list (APPEND UTILS_SRCS src/Utils/WGL.hpp
                          src/Utils/WGL.cpp)
endif()
list (APPEND SRCS ${UTILS_SRCS})
source_group (Utils FILES ${UTILS_SRCS})

set (DOCUMENT_SRCS
            src/Document/Document.cpp
            src/Document/Document.hpp)
list (APPEND SRCS ${DOCUMENT_SRCS})
source_group (Document FILES ${DOCUMENT_SRCS})

set (LEXERS_SRCS
            src/Lexers/Lexer.hpp
            src/Lexers/Lexer.cpp
            src/Lexers/CPPLexer.hpp
            src/Lexers/CPPLexer.cpp)
list (APPEND SRCS ${LEXERS_SRCS})
source_group (Lexers FILES ${LEXERS_SRCS})

# Configure test data directory path
set (TESTDATA_SIMPLEFILE "${CMAKE_SOURCE_DIR}/TestData/SimpleFile.cpp")
set (TESTDATA_BASICBLOCKFILE "${CMAKE_SOURCE_DIR}/TestData/BasicBlock.cpp")
configure_file (${CMAKE_SOURCE_DIR}/TestData/config_template.hpp ${CMAKE_BINARY_DIR}/Configuration/config.hpp)
list (APPEND INCLUDES ${CMAKE_BINARY_DIR}/Configuration)

# Main Varco project definition and its dependency projects
add_executable (varco ${SRCS})
target_include_directories (varco PUBLIC ${INCLUDES})
target_link_libraries (varco skia)

if (UNIX)
  #X11 specifies a different default order for RGBA samples (BGRA)
  target_compile_definitions(varco PRIVATE -DSK_SAMPLES_FOR_X)
endif()

if (UNIX) # Additional unix libraries required

  #X11
  find_package ( X11 REQUIRED )
  if (NOT X11_FOUND)
    message (ERROR "Required X11 package not found")
  else()
    include_directories (${X11_INCLUDE_DIR})
    target_link_libraries (varco ${X11_LIBRARIES})
  endif()
  
  set_target_properties (varco PROPERTIES COMPILE_FLAGS -pthread LINK_FLAGS -pthread)
	
  #Freetype
  find_package (Freetype REQUIRED)
	if(NOT FREETYPE_FOUND)
	  message (ERROR "Required Freetype package not found")
	else()
	  target_link_libraries (varco ${FREETYPE_LIBRARIES})
	endif()	
		
endif()

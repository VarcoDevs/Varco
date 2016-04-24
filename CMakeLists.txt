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

########################################### Skia dependency ###########################################

# This shields us against Skia compile-time macros used in headers
set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DSK_DEBUG")
# No gpu log calls in debug
set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DGR_GL_LOG_CALLS=0 -DGR_GL_CHECK_ERROR=0")

# 7zip is used to decompress during configure the necessary dependencies
set (CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Decompress binaries if needed
if (NOT EXISTS ${CMAKE_SOURCE_DIR}/deps/skia/libs/win64/skia.lib)
  message (STATUS "Decompressing Win64 Skia binaries...")

  # Important: dependencies MUST be available
  if (NOT EXISTS ${CMAKE_SOURCE_DIR}/deps/skia.7z)
    message (FATAL_ERROR "[ERROR] 'deps/skia.7z' unavailable - please download it from https://github.com/VarcoDevs/Varco/releases")
  endif()


  # 7zip is preferable, but as a fallback use cmake built-in 7zip module
  find_package (SevenZip)
  if (NOT SEVENZIP_FOUND)
    message (STATUS "[FALLBACK] CMake will be used to decompress dependencies")
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E tar xzf ${CMAKE_SOURCE_DIR}/deps/skia.7z
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/deps # Folder where to decompress stuff
    )
  else() # Use 7Zip
    execute_process(
        COMMAND ${SEVENZIP_BINARY} x ${CMAKE_SOURCE_DIR}/deps/skia.7z
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/deps # Folder where to decompress stuff
    )
  endif()
endif()

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

if (WIN32)
  set (WINDOWHANDLING_SRCS 
          src/WindowHandling/BaseOSWindow_Win.cpp
          src/WindowHandling/BaseOSWindow_Win.hpp
          src/WindowHandling/MainWindow.cpp
          src/WindowHandling/MainWindow.hpp)
  list (APPEND LIBS
          optimized;${CMAKE_SOURCE_DIR}/deps/skia/libs/win64/skia.lib;
          debug;${CMAKE_SOURCE_DIR}/deps/skia/libs/win64/skiad.lib;)

  if (NOT EXISTS ${CMAKE_SOURCE_DIR}/deps/skia/libs/win64/skia.lib)
    message (FATAL_ERROR "[ERROR] Could not decompress dependencies - either install 7zip or unpack them manually")
  endif()
  # Copy debug information for skia into the debug build directory
  message(STATUS "Copying debug information to build directory.." )
  file(COPY ${CMAKE_SOURCE_DIR}/deps/skia/libs/win64/skiad.pdb DESTINATION ${CMAKE_BINARY_DIR}/Debug)
  file(RENAME ${CMAKE_BINARY_DIR}/Debug/skiad.pdb ${CMAKE_BINARY_DIR}/Debug/skia.pdb)
endif()
if (UNIX)
  set (WINDOWHANDLING_SRCS
          src/WindowHandling/BaseOSWindow_Linux.cpp
          src/WindowHandling/BaseOSWindow_Linux.hpp
          src/WindowHandling/MainWindow.cpp
          src/WindowHandling/MainWindow.hpp)
  list (APPEND LIBS
          optimized;${CMAKE_SOURCE_DIR}/deps/skia/libs/linux/libskia.a;
          debug;${CMAKE_SOURCE_DIR}/deps/skia/libs/linux/libskiad.a;)

  if (NOT EXISTS ${CMAKE_SOURCE_DIR}/deps/skia/libs/linux/libskia.a)
    message (FATAL_ERROR "[ERROR] Could not decompress dependencies - either install 7zip or unpack them manually")
  endif()
endif()
########################################## End Skia dependency ##########################################

list (APPEND SRCS ${WINDOWHANDLING_SRCS})
source_group (WindowHandling FILES ${WINDOWHANDLING_SRCS})

set (UI_SRCS
			    src/UI/UICtrlBase.cpp
			    src/UI/UICtrlBase.hpp
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
            src/Utils/Concurrent.hpp)
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

# Configure test data directory path
set (TESTDATA_SIMPLEFILE "${CMAKE_SOURCE_DIR}/TestData/SimpleFile.cpp")
configure_file (${CMAKE_SOURCE_DIR}/TestData/config_template.hpp ${CMAKE_BINARY_DIR}/Configuration/config.hpp)
list (APPEND INCLUDES ${CMAKE_BINARY_DIR}/Configuration)

add_executable (varco ${SRCS})
target_include_directories (varco PUBLIC ${INCLUDES})
target_link_libraries (varco ${LIBS})
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
  
  #FontConfig
  find_package (Fontconfig REQUIRED)
	if(NOT FONTCONFIG_FOUND)
	  message (ERROR "Required FontConfig package not found")	
	else()
	  target_link_libraries (varco ${FONTCONFIG_LIBRARIES})
	endif()	
	
	#Freetype
  find_package (Freetype REQUIRED)
	if(NOT FREETYPE_FOUND)
	  message (ERROR "Required Freetype package not found")
	else()
	  target_link_libraries (varco ${FREETYPE_LIBRARIES})
	endif()	
		
endif()

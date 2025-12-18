cmake_minimum_required(VERSION 3.10)
project(video_vcam_plugin CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find VLC plugins headers
# On macOS, if installed via Homebrew, they might be in /opt/homebrew/include/vlc/plugins
# Or we might need to point to the VLC source tree.
# For now, we try standard locations or PKG_CONFIG.

find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
    pkg_check_modules(VLC_PLUGIN vlc-plugin)
endif()

# Fallback: User can provide VLC_INCLUDE_DIR
if(NOT VLC_PLUGIN_FOUND)
    if(NOT VLC_INCLUDE_DIR)
        # Check local restored headers
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/vlc_sdk/include/vlc_plugin.h")
            set(VLC_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/vlc_sdk/include")
        # Check if VLC.app has headers (Best for ABI match on macOS)
        elseif(EXISTS "/Applications/VLC.app/Contents/MacOS/include/vlc/vlc_common.h")
            message(STATUS "Using VLC headers from /Applications/VLC.app")
            set(VLC_INCLUDE_DIR "/Applications/VLC.app/Contents/MacOS/include")
        else()
             message(WARNING "VLC headers not found. Please set VLC_INCLUDE_DIR.")
        endif()
    endif()
endif()

if(VLC_INCLUDE_DIR)
    include_directories(${VLC_INCLUDE_DIR})
    include_directories(${VLC_INCLUDE_DIR}/plugins)
else()
     # Final fallback
     include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
endif()

# VLC plugins usually need these definitions
add_definitions(-D__PLUGIN__ -D_FILE_OFFSET_BITS=64 -D_REENTRANT -D_THREAD_SAFE)
# VLC 3 macOS build expects a versioned entry symbol like: vlc_entry__3_0_0f
# (see libvlccore.dylib strings). So we build with MODULE_NAME=3_0_0f to match.
add_definitions(-DMODULE_NAME=3_0_0f)
add_definitions(-DMODULE_STRING="video_vcam")
add_definitions(-Drestrict=__restrict)

# macOS specific: VLC plugins are dynamic libraries (modules)
add_library(video_vcam_plugin MODULE src/vcam_plugin.cpp)

# Link against NDI
# Using the one found in DAL plugin as it is reliable on this system

# Remove the "lib" prefix automatically added by CMake, because VLC plugins usually have specific naming.
# But user requested "libvideo_vcam_plugin.dylib".
# CMake defaults: lib<name>.dylib on Mac.
# So "libvideo_vcam_plugin.dylib" is actually the default output for library name "video_vcam_plugin".
# However, modules usually don't have the 'lib' prefix in some VLC contexts, but on macOS they often do or it doesn't matter.
# We will keep the default 'lib' prefix to match the user request "libvideo_vcam_plugin.dylib".

# Link against NDI
# Using the one found in DAL plugin as it is reliable on this system
find_library(NDI_LIB ndi PATHS "/Library/CoreMediaIO/Plug-Ins/DAL/NDIVideoOut.plugin/Contents/Frameworks" NO_DEFAULT_PATH)
if (NOT NDI_LIB)
    message(WARNING "libndi.dylib not found in DAL plugin path. Trying standard locations.")
    find_library(NDI_LIB ndi)
endif()

if (NDI_LIB)
    message(STATUS "Found NDI Library: ${NDI_LIB}")
    target_link_libraries(video_vcam_plugin ${NDI_LIB})
else()
    message(FATAL_ERROR "Could not find libndi.dylib. Please install NDI Tools.")
endif()

set_target_properties(video_vcam_plugin PROPERTIES
    PREFIX "lib"
    SUFFIX ".dylib"
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

if(APPLE)
    # Let VLC resolve symbols when dlopening the plugin.
    set_target_properties(video_vcam_plugin PROPERTIES
        LINK_FLAGS "-undefined dynamic_lookup"
    )
endif()


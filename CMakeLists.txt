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
        message(WARNING "VLC plugin headers not found. Please set VLC_INCLUDE_DIR or install vlc-plugin headers.")
        # Attempt to guess typical Mac locations (e.g. if installed via brew)
        set(POSSIBLE_VLC_PATHS
            "/opt/homebrew/include"
            "/usr/local/include"
            "/Applications/VLC.app/Contents/MacOS/include"
        )
        find_path(VLC_INCLUDE_DIR vlc_common.h PATHS ${POSSIBLE_VLC_PATHS} PATH_SUFFIXES vlc/plugins vlc)
    endif()
endif()

if(VLC_INCLUDE_DIR)
    include_directories(${VLC_INCLUDE_DIR})
    include_directories(${VLC_INCLUDE_DIR}/plugins)
else()
    # If still not found, we assume they might be in 'include' in this project (if user copies them)
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
# Use Objective-C++ so we can interact with Cocoa menus.
add_library(video_vcam_plugin MODULE src/vcam_plugin.mm)

# Remove the "lib" prefix automatically added by CMake, because VLC plugins usually have specific naming.
# But user requested "libvideo_vcam_plugin.dylib".
# CMake defaults: lib<name>.dylib on Mac.
# So "libvideo_vcam_plugin.dylib" is actually the default output for library name "video_vcam_plugin".
# However, modules usually don't have the 'lib' prefix in some VLC contexts, but on macOS they often do or it doesn't matter.
# We will keep the default 'lib' prefix to match the user request "libvideo_vcam_plugin.dylib".

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

    # Link Cocoa for menu integration.
    find_library(COCOA_LIBRARY Cocoa)
    if(COCOA_LIBRARY)
        target_link_libraries(video_vcam_plugin PRIVATE "${COCOA_LIBRARY}")
    else()
        message(WARNING "Cocoa framework not found; menu integration will fail to link.")
    endif()
endif()

